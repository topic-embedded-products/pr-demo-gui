#ifndef FRAMERATECOUNTER_H
#define FRAMERATECOUNTER_H

#include <QObject>

class FrameRateCounter : public QObject
{
    Q_OBJECT
public:
    FrameRateCounter();
    void frame();
signals:
    void frameRate(unsigned int ms_per_frame);
protected:
    qint64 firstStamp;
    qint64 lastStamp;
    unsigned int count;
};

#endif // FRAMERATECOUNTER_H
