#include "audiopipeline.h"
#include "microphonecapturethread.h"

AudioPipeline::AudioPipeline(QObject *parent) : QObject(parent),
    capture(NULL)
{
}

AudioPipeline::~AudioPipeline()
{
    stop();
}

void AudioPipeline::start()
{
    if (!capture)
    {
        capture = new MicrophoneCaptureThread();
        connect(capture, SIGNAL(capturedAudio(short*,uint)), this, SLOT(receivedAudio(short*,uint)));
        connect(capture, SIGNAL(finished()), this, SLOT(captureThreadFinished()));
        capture->startCapturing("default");
        emit setActive(true);
    }
}

void AudioPipeline::stop()
{
    MicrophoneCaptureThread *c = capture;
    if (c)
    {
        capture = NULL; /* Make sure the var is null before triggering callbacks */
        delete c;
    }
}

void AudioPipeline::receivedAudio(short *buf, unsigned int len)
{
    if (capture)
    {
        emit capturedAudio(buf, len);
        // 'buf' has been used to generate the spectrum, continue capturing
        capture->continueCapturing();
    }
}

void AudioPipeline::captureThreadFinished()
{
    emit setActive(false);
}
