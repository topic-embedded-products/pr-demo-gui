#include "dyplocontext.h"
#include <QDebug>

DyploContext::DyploContext()
{
    try
    {
        hardwareCtx = new dyplo::HardwareContext();
        hwControl = new dyplo::HardwareControl(*hardwareCtx);

        // All bitstreams are partial streams.
        hardwareCtx->setProgramMode(true);
    }
    catch (const std::exception& ex)
    {
        qCritical() << "ERROR:\n" << QString(ex.what());
    }
}

DyploContext::~DyploContext()
{
    delete hwControl;
    delete hardwareCtx;
}

dyplo::HardwareContext& DyploContext::GetHardwareContext()
{
    return *hardwareCtx;
}

dyplo::HardwareControl& DyploContext::GetHardwareControl()
{
    return *hwControl;
}
