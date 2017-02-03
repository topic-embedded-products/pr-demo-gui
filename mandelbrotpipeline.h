#ifndef MANDELBROTPIPELINE_H
#define MANDELBROTPIPELINE_H

#include <QObject>
#include <QImage>
#include "dyploresources.h"

class QImage;
class QSocketNotifier;
class DyploContext;

namespace dyplo {
class HardwareFifo;
class HardwareDMAFifo;
class HardwareConfig;
}

struct MandelbrotImage {
    QImage image;
    int lines_remaining;
};

class MandelbrotPipeline : public QObject
{
    Q_OBJECT
public:
    explicit MandelbrotPipeline(QObject *parent = 0);
    virtual ~MandelbrotPipeline();

    bool setSize(int width, int height);
    int activate(DyploContext* dyplo);
    void deactivate();

    void enumDyploResources(DyploNodeInfoList& list);

signals:
    void renderedImage(const QImage &image);
    void setActive(bool active);

private slots:
    void frameAvailableDyplo(int socket);

public slots:

protected:
    int video_width;
    int video_height;
    int video_lines_per_block;
    unsigned int video_blocksize;
    QSocketNotifier* fromLogicNotifier;
    dyplo::HardwareDMAFifo *from_logic;
    dyplo::HardwareFifo *to_logic;
    dyplo::HardwareConfig *node;
    double x;
    double y;
    double z;
    MandelbrotImage currentImage;
    int current_scanline;

    void writeConfig(int start_scanline, int number_of_lines);
    void zoomFrame();
    void requestNext(int lines);
};

#endif // MANDELBROTPIPELINE_H
