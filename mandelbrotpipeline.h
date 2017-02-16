#ifndef MANDELBROTPIPELINE_H
#define MANDELBROTPIPELINE_H

#include <QObject>
#include <QImage>
#include "dyploresources.h"
#include <vector>

/* Forward declarations */
class QSocketNotifier;
class DyploContext;
class MandelbrotPipeline;

namespace dyplo {
class HardwareFifo;
class HardwareDMAFifo;
class HardwareConfig;
}

struct MandelbrotImage {
    QImage image;
    int lines_remaining;

    void initialize(int width, int height);
};

struct MandelbrotRequest;

class MandelbrotWorker;
typedef std::vector<MandelbrotWorker *> MandelbrotWorkerList;

class MandelbrotIncoming : public QObject
{
    Q_OBJECT
protected:
    MandelbrotPipeline *pipeline;
    QSocketNotifier* fromLogicNotifier;
    dyplo::HardwareDMAFifo *from_logic;
    unsigned int video_blocksize;
public:
    MandelbrotIncoming(MandelbrotPipeline *parent, DyploContext *dyplo, int blocksize, int node_index);
    ~MandelbrotIncoming();
private slots:
    void dataAvailable(int socket);
};

typedef std::vector<MandelbrotIncoming *> MandelbrotIncomingList;

typedef std::vector<dyplo::HardwareConfig *> HardwareConfigList;

#define MANDELBROT_RENDER_IMAGES    4

class MandelbrotPipeline : public QObject
{
    Q_OBJECT
public:
    explicit MandelbrotPipeline(QObject *parent = 0);
    virtual ~MandelbrotPipeline();

    bool setSize(int width, int height);
    int activate(DyploContext* dyplo, int max_nodes);

    /* Go to this location on the next frame. */
    void setCoordinates(double _next_x, double _next_y);
    void resetZoom();

    void enumDyploResources(DyploNodeInfoList& list);

    /* Called from MandelbrotIncoming */
    void dataAvailable(const uchar *data, unsigned int bytes_used);

    double getX() const { return x; }
    double getY() const { return y; }
    double getZ() const { return z; }

public slots:
    void deactivate();

signals:
    void renderedImage(const QImage &image);
    void setActive(bool active);

protected:
    int video_width;
    int video_height;
    int video_lines_per_block;
    MandelbrotIncomingList incoming;
    MandelbrotWorkerList outgoing;
    HardwareConfigList mux;
    double x; /* Center X */
    double y; /* Center Y */
    double z; /* zoom factor, value of one pixel */
    long long fixed_z; /* z in fixed-point */
    long long fixed_left_x; /* X starting point in fixed-point */
    double next_x;
    double next_y;
    double next_z;
    MandelbrotImage rendered_image[MANDELBROT_RENDER_IMAGES];
    int current_scanline;
    int current_image;
    bool next_xy_valid;
    bool next_z_reset;

    void deactivate_impl();
    void zoomFrame();
    void requestNext(unsigned short worker_index);
};

#endif // MANDELBROTPIPELINE_H
