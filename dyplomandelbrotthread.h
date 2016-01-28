#ifndef DyploMandelThread_H
#define DyploMandelThread_H

#include <QMutex>
#include <QSize>
#include <QThread>
#include <QWaitCondition>

class QImage;

class DyploMandelbrotThread : public QThread
{
    Q_OBJECT

public:
    DyploMandelbrotThread(QObject *parent = 0);
    ~DyploMandelbrotThread();

    void grabNextFrame();

signals:
    void renderedImage(const QImage &image);

protected:
    virtual void run();

private:
    QMutex mutex;
    QWaitCondition condition;
    bool abort;
    bool continueGrabbing;
};

#endif // DyploMandelThread_H

