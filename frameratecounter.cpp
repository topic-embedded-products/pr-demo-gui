#include "frameratecounter.h"
#include <QDateTime>

FrameRateCounter::FrameRateCounter():
    firstStamp(QDateTime::currentMSecsSinceEpoch()),
    count(0)
{
    lastStamp = firstStamp;
}

void FrameRateCounter::frame()
{
    ++count;
    lastStamp = QDateTime::currentMSecsSinceEpoch();
    qint64 diff = lastStamp - firstStamp;
    if (diff > 1000) {
        emit frameRate(count, diff);
        firstStamp = lastStamp;
        count = 0;
    }
}
