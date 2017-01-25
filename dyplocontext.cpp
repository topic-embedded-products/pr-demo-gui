#include "dyplocontext.h"
#include <errno.h>
#include <QElapsedTimer>
#include <QDebug>

DyploContext::DyploContext()
{
    try
    {
        hwControl = new dyplo::HardwareControl(hardwareCtx);
    }
    catch (const std::exception& ex)
    {
        qCritical() << "ERROR: Cannot initialize Dyplo:\n" << QString(ex.what());
    }
}

DyploContext::~DyploContext()
{
    delete hwControl;
}

dyplo::HardwareConfig *DyploContext::createConfig(const char *name)
{
    if (!hwControl)
        throw dyplo::IOException(name, ENODEV);
    unsigned int candidates = hardwareCtx.getAvailablePartitions(name);
    if (candidates == 0)
        throw dyplo::IOException(name, ENODEV);
    int id = 0;
    while (candidates)
    {
        if ((candidates & 0x01) != 0)
        {
            int handle = hardwareCtx.openConfig(id, O_RDWR);
            if (handle == -1)
            {
                if (errno != EBUSY) /* Non existent? Bail out, last node */
                    throw dyplo::IOException(name);
            }
            else
            {
                hwControl->disableNode(id);
                std::string filename = hardwareCtx.findPartition(name, id);
                unsigned int size;
                QElapsedTimer myTimer; /* TODO: More accurate */
                myTimer.start();
                {
                    dyplo::HardwareProgrammer programmer(hardwareCtx, *hwControl);
                    size = programmer.fromFile(filename.c_str());
                }
                unsigned int elapsed = myTimer.nsecsElapsed() / 1000;
                emit programmedPartial(id, name, size, elapsed);
                return new dyplo::HardwareConfig(handle);
            }
        }
        ++id;
        candidates >>= 1;
    }
    throw dyplo::IOException(name, ENODEV);
}

dyplo::HardwareDMAFifo *DyploContext::createDMAFifo(int access)
{
    return new dyplo::HardwareDMAFifo(hardwareCtx.openAvailableDMA(access));
}
