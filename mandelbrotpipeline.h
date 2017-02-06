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

    /* Called from MandelbrotIncoming */
    void dataAvailable(const uchar *data, unsigned int bytes_used);

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
    MandelbrotImage rendered_image[2];
    int current_scanline;
    int current_image;

    void zoomFrame();
    void requestNext(unsigned short worker_index);
};

#endif // MANDELBROTPIPELINE_H
