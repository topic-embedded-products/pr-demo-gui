#ifndef VIDEOPIPELINE_H
#define VIDEOPIPELINE_H

#include <QObject>
#include <QImage>
#include "video-capture.h"
#include "dyploresources.h"

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

    int activate(DyploContext* dyplo, int width, int height, bool hardwareYUV, bool filterContrast, bool filterGray, bool filterThd);
    void deactivate();

    void enumDyploResources(DyploNodeResourceList& list);

    QSize getVideoSize() const { return QSize(settings.width, settings.height); }

signals:
    void renderedImage(const QImage &image);
    void setActive(bool active);

private slots:
    void frameAvailableSoft(int socket);
    void frameAvailableHard(int socket);
    void frameAvailableDyplo(int socket);

protected:
    int openIOCamera(DyploContext *dyplo, int width, int height, bool filterContrast, bool filterGray, bool filterThd);
    int openCaptureDevice(int width, int height);
    void deactivate_impl();
    void allocYUVbuffer();

    void update_buffer_sizes();
    void update_rgb_settings(int width, int height);

    VideoCapture capture;
    QSocketNotifier* captureNotifier;
    QSocketNotifier* fromLogicNotifier;
    unsigned char* rgb_buffer;

    dyplo::HardwareDMAFifo *to_logic;
    dyplo::HardwareDMAFifo *from_logic;
    dyplo::HardwareConfig *yuv2rgb;
    dyplo::HardwareConfig *filterContrast;
    dyplo::HardwareConfig *filterTreshold;
    dyplo::HardwareConfig *filterGrayscale;
    dyplo::HardwareConfig *ioCamera;

    unsigned int software_flags;
    unsigned int* yuv_buffer;
    VideoCaptureSettings settings;
    unsigned int yuv_size;
    unsigned int rgb_size;

    unsigned int crop_top;
    unsigned int crop_left;
    unsigned int crop_height;
    unsigned int crop_width;
    unsigned int crop_offset; /* in bytes */

    enum QImage::Format outputformat;
};

#endif // VIDEOPIPELINE_H
