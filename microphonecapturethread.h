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

public slots:
    void startCapturing(QString audioDeviceName);
    void stopCapturing();
    void continueCapturing();

signals:
    void capturedAudio(short* buf, unsigned int len);

protected:
    virtual void run();

private:
    QString iAudioDeviceName;
    bool iCapturing;
    bool iContinueCapturing;

    QMutex iMutex;
    QWaitCondition iCondition;
};

#endif // MICROPHONECAPTURE_H
