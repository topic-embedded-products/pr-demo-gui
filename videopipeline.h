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

    int activate(DyploContext* dyplo, bool hardwareYUV, bool filterContrast, bool filterGray, bool filterThd);
    void deactivate();

    void enumDyploResources(DyploNodeResourceList& list);
signals:
    void renderedImage(const QImage &image);
    void setActive(bool active);

private slots:
    void frameAvailableSoft(int socket);
    void frameAvailableHard(int socket);
    void frameAvailableDyplo(int socket);

protected:
    void deactivate_impl();
    void allocYUVbuffer();

    VideoCapture capture;
    QSocketNotifier* captureNotifier;
    QSocketNotifier* fromLogicNotifier;
    unsigned char* rgb_buffer;

    dyplo::HardwareDMAFifo *to_logic;
    dyplo::HardwareDMAFifo *from_logic;
    dyplo::HardwareConfig *yuv2rgb;
    dyplo::HardwareConfig *filter1;
    dyplo::HardwareConfig *filterTreshold;
    dyplo::HardwareConfig *yuvfilter1;

    unsigned int software_flags;
    unsigned int* yuv_buffer;
};

#endif // VIDEOPIPELINE_H
