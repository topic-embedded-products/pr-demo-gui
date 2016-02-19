#include "microphonecapturethread.h"

#include <stdio.h>
#include <stdlib.h>
#include <alsa/asoundlib.h>
#include <QDebug>

const unsigned int MicrophoneCaptureThread::CAPTURE_SIZE = 2048;

MicrophoneCaptureThread::MicrophoneCaptureThread(QObject* parent) :
    QThread(parent),
    iCapturing(false),
    iContinueCapturing(false)
{
}

MicrophoneCaptureThread::~MicrophoneCaptureThread()
{
    stopCapturing();
}

void MicrophoneCaptureThread::startCapturing(QString audioDeviceName)
{
    iAudioDeviceName = audioDeviceName;
    iCapturing = true;
    start(NormalPriority);
}

void MicrophoneCaptureThread::stopCapturing()
{
    {
        QMutexLocker lock(&iMutex);
        iCapturing = false;
        iContinueCapturing = true;
        iCondition.wakeAll();
    }
    wait();
}

void MicrophoneCaptureThread::continueCapturing()
{
    QMutexLocker locker(&iMutex);
    iContinueCapturing = true;
    iCondition.wakeOne();
}

void MicrophoneCaptureThread::run()
{
    int err;
    snd_pcm_t* capture_handle;

    if ((err = snd_pcm_open (&capture_handle, iAudioDeviceName.toStdString().c_str(), SND_PCM_STREAM_CAPTURE, 0)) < 0) {
        fprintf (stderr, "cannot open audio device %s (%s)\n",
             iAudioDeviceName.toStdString().c_str(),
             snd_strerror(err));
        return;
    }

    captureloop(capture_handle);

    snd_pcm_close(capture_handle);
}

void MicrophoneCaptureThread::captureloop(void *_capture_handle)
{
    int err;
    snd_pcm_t* capture_handle = (snd_pcm_t*)_capture_handle;
    snd_pcm_hw_params_t* hw_params;
    short buf[CAPTURE_SIZE];

    if ((err = snd_pcm_hw_params_malloc(&hw_params)) < 0) {
        fprintf (stderr, "cannot allocate hardware parameter structure (%s)\n",
             snd_strerror (err));
        return;
    }

    if ((err = snd_pcm_hw_params_any(capture_handle, hw_params)) < 0) {
        fprintf (stderr, "cannot initialize hardware parameter structure (%s)\n",
             snd_strerror (err));
        return;
    }

    if ((err = snd_pcm_hw_params_set_access(capture_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
        fprintf (stderr, "cannot set access type (%s)\n",
             snd_strerror (err));
        return;
    }

    if ((err = snd_pcm_hw_params_set_format(capture_handle, hw_params, SND_PCM_FORMAT_S16_LE)) < 0) {
        fprintf (stderr, "cannot set sample format (%s)\n",
             snd_strerror (err));
        return;
    }

    unsigned int sampleRate = 44100;
    if ((err = snd_pcm_hw_params_set_rate_near(capture_handle, hw_params, &sampleRate, 0)) < 0) {
        fprintf (stderr, "cannot set sample rate (%s)\n",
             snd_strerror (err));
        return;
    }

    if ((err = snd_pcm_hw_params_set_channels(capture_handle, hw_params, 1)) < 0) {
        fprintf (stderr, "cannot set channel count (%s)\n",
             snd_strerror (err));
        return;
    }

    if ((err = snd_pcm_hw_params (capture_handle, hw_params)) < 0) {
        fprintf (stderr, "cannot set parameters (%s)\n",
             snd_strerror (err));
        return;
    }

    snd_pcm_hw_params_free(hw_params);

    if ((err = snd_pcm_prepare (capture_handle)) < 0) {
        fprintf (stderr, "cannot prepare audio interface for use (%s)\n",
             snd_strerror (err));
        return;
    }

    while (iCapturing)
    {
        if ((err = snd_pcm_readi(capture_handle, buf, CAPTURE_SIZE)) != CAPTURE_SIZE) {
            fprintf (stderr, "read from audio interface failed (%s)\n",
                 snd_strerror (err));
            return;
        }
        else
        {
            emit capturedAudio(buf, CAPTURE_SIZE);

            {
                QMutexLocker lock(&iMutex);
                if (!iContinueCapturing && iCapturing)
                    iCondition.wait(&iMutex);
                iContinueCapturing = false;
            }
        }
    }
}
