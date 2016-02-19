#ifndef MICROPHONECAPTURE_H
#define MICROPHONECAPTURE_H

#include <QObject>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>

class MicrophoneCaptureThread : public QThread
{
    Q_OBJECT

public:
    MicrophoneCaptureThread(QObject* parent = 0);
    virtual ~MicrophoneCaptureThread();

    // by default, capture 4096 bytes (2048 x 16bits)
    static const unsigned int CAPTURE_SIZE;

    void startCapturing(QString audioDeviceName);
    void continueCapturing();

signals:
    void capturedAudio(short* buf, unsigned int len);

protected:
    void stopCapturing();
    virtual void run();
    void captureloop(void* capture_handle);

private:
    QString iAudioDeviceName;
    QMutex iMutex;
    QWaitCondition iCondition;
    bool iCapturing;
    bool iContinueCapturing;
};

#endif // MICROPHONECAPTURE_H
