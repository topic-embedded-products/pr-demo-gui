#ifndef MANDELBROTPIPELINE_H
#define MANDELBROTPIPELINE_H

#include <QObject>
#include "dyploresources.h"

class QImage;
class QSocketNotifier;
class DyploContext;

namespace dyplo {
class HardwareFifo;
class HardwareDMAFifo;
class HardwareConfig;
}

class MandelbrotPipeline : public QObject
{
    Q_OBJECT
public:
    explicit MandelbrotPipeline(QObject *parent = 0);
    virtual ~MandelbrotPipeline();

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
    QSocketNotifier* fromLogicNotifier;
    dyplo::HardwareDMAFifo *from_logic;
    dyplo::HardwareFifo *to_logic;
    dyplo::HardwareConfig *node;
    double x;
    double y;
    double z;
};

#endif // MANDELBROTPIPELINE_H
