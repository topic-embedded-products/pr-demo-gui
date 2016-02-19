#ifndef AUDIOPIPELINE_H
#define AUDIOPIPELINE_H

#include <QObject>

class MicrophoneCaptureThread;

class AudioPipeline : public QObject
{
    Q_OBJECT
public:
    explicit AudioPipeline(QObject *parent = 0);
    virtual ~AudioPipeline();

    void start();
    void stop();

signals:
    void capturedAudio(short* buf, unsigned int len);
    void setActive(bool active);

public slots:
    void receivedAudio(short* buf, unsigned int len);
private slots:
    void captureThreadFinished();

protected:
    MicrophoneCaptureThread *capture;
};

#endif // AUDIOPIPELINE_H
