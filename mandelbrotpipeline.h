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

class MandelbrotWorker
{
protected:
    dyplo::HardwareConfig *node;
    dyplo::HardwareFifo *to_logic;
public:
    MandelbrotWorker(DyploContext *dyplo);
    ~MandelbrotWorker();
    int getNodeIndex() const;
    void request(const void* data, int count);
};

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
    double x;
    double y;
    double z;
    MandelbrotImage rendered_image[2];
    int current_scanline;
    int current_image;

    void writeConfig(int outgoing_index, int start_scanline, int number_of_lines, unsigned short extra_line_bits);
    void zoomFrame();
    void requestNext(int outgoing_index, int lines);
};

#endif // MANDELBROTPIPELINE_H
