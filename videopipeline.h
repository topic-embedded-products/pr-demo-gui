#ifndef VIDEOPIPELINE_H
#define VIDEOPIPELINE_H

#include <QObject>
#include "video-capture.h"
#include "dyploresources.h"

class QImage;
class QSocketNotifier;
class DyploContext;

namespace dyplo {
class HardwareDMAFifo;
class HardwareConfig;
}

class VideoPipeline: public QObject
{
    Q_OBJECT

public:
    VideoPipeline();
    ~VideoPipeline();

    int activate(DyploContext* dyplo, bool hardwareYUV);
    void deactivate();

    void enumDyploResources(DyploNodeInfoList& list);
signals:
    void renderedImage(const QImage &image);
    void setActive(bool active);

private slots:
    void frameAvailableSoft(int socket);
    void frameAvailableHard(int socket);
    void frameAvailableDyplo(int socket);

protected:
    VideoCapture capture;
    QSocketNotifier* captureNotifier;
    QSocketNotifier* fromLogicNotifier;
    unsigned char* rgb_buffer;

    dyplo::HardwareDMAFifo *to_logic;
    dyplo::HardwareDMAFifo *from_logic;
    dyplo::HardwareConfig *yuv2rgb;
};

#endif // VIDEOPIPELINE_H
