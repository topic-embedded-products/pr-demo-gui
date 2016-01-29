#ifndef DyploMandelThread_H
#define DyploMandelThread_H

#include <QMutex>
#include <QSize>
#include <QThread>
#include <QWaitCondition>
#include "dyplocontext.h"

class QImage;

class DyploMandelbrotThread : public QThread
{
    Q_OBJECT

public:
    DyploMandelbrotThread(QObject* parent = 0);
    ~DyploMandelbrotThread();

    void grabNextFrame();
    void startRendering(int dyploOutputNodeId);
    void stopRendering();

signals:
    void renderedImage(const QImage &image);

protected:
    virtual void run();

private:
    QMutex mutex;
    QWaitCondition condition;
    bool abort;
    bool continueGrabbing;
    int dyploOutputNodeId;
};

#endif // DyploMandelThread_H

