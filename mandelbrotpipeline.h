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
};

class MandelbrotWorker
{
protected:
    dyplo::HardwareConfig *node;
    dyplo::HardwareFifo *to_logic;
public:
    MandelbrotWorker():
        node(NULL), to_logic(NULL)
    {}
    void activate(DyploContext *dyplo);
    void deactivate();
    bool isActive() const { return node != NULL; }
    int getNodeIndex() const;
    void request(const void* data, int count);
};

typedef std::vector<MandelbrotWorker> WorkerList;

class MandelbrotIncoming : public QObject
{
    Q_OBJECT
protected:
    MandelbrotPipeline *pipeline;
    QSocketNotifier* fromLogicNotifier;
    dyplo::HardwareDMAFifo *from_logic;
    unsigned int video_blocksize;
public:
    MandelbrotIncoming(MandelbrotPipeline *parent);
    void activate(DyploContext *dyplo, int blocksize, int node_index);
    void deactivate();
private slots:
    void dataAvailable(int socket);
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

    /* Called from MandelbrotIncoming */
    void dataAvailable(const uchar *data, unsigned int bytes_used);

signals:
    void renderedImage(const QImage &image);
    void setActive(bool active);

protected:
    int video_width;
    int video_height;
    int video_lines_per_block;
    MandelbrotIncoming incoming;
    MandelbrotWorker outgoing;
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
