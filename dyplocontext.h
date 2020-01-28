#ifndef DYPLOCONTEXT_H
#define DYPLOCONTEXT_H

#include <QObject>
#include "dyplo/hardware.hpp"
#include "dyplonodeinfo.h"

class DyploContext: public QObject
{
    Q_OBJECT
public:
    DyploContext();
    ~DyploContext();

    dyplo::HardwareContext& GetHardwareContext() { return hardwareCtx; }
    dyplo::HardwareControl& GetHardwareControl() const { return *hwControl; }

    /* Note: For "create" functions, the caller becomes the owner of the
     * newly created object (factory pattern). Throw exception on failure. */
    dyplo::HardwareConfig* createConfig(const char* name);
    dyplo::HardwareDMAFifo* createDMAFifo(int access);

    /* Hardware layout */
    QVector<DyploNodeInfo> nodeInfo;
    unsigned int num_dma_nodes;

signals:
    void programmedPartial(int node, const char* name, unsigned int size, unsigned int microseconds);

private:
    DyploContext(DyploContext const&);      // Don't Implement
    void operator=(DyploContext const&);    // Don't implement

    dyplo::HardwareContext hardwareCtx;
    dyplo::HardwareControl* hwControl;
};

#endif // DYPLOCONTEXT_H
