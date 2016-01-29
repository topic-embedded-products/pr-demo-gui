#ifndef DyploVideoThread_H
#define DyploVideoThread_H

#include <QMutex>
#include <QSize>
#include <QThread>
#include <QWaitCondition>

class QImage;

class DyploVideoThread : public QThread
{
    Q_OBJECT

public:
    DyploVideoThread(QObject* parent = 0);
    ~DyploVideoThread();

    void grabNextFrame();

public slots:
    void startRendering(int dyploOutputNodeId);
    void stopRendering();

signals:
    void renderedImage(const QImage& image);

protected:
    virtual void run();

private:
    QMutex mutex;
    QWaitCondition condition;
    bool abort;
    bool continueGrabbing;
    int dyploOutputNodeId;
};

#endif // DyploVideoThread_H

