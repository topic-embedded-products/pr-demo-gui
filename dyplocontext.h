#ifndef DYPLOCONTEXT_H
#define DYPLOCONTEXT_H

#include <QObject>
#include "dyplo/hardware.hpp"

class DyploContext
{
public:
    // singleton
    static DyploContext& getInstance()
    {
        static DyploContext instance;
        return instance;
    }

    ~DyploContext();

    dyplo::HardwareContext& GetHardwareContext();
    dyplo::HardwareControl& GetHardwareControl();

private:

    DyploContext();                         // Constructor? (the {} brackets) are needed here.
    DyploContext(DyploContext const&);      // Don't Implement
    void operator=(DyploContext const&);    // Don't implement

    dyplo::HardwareContext* hardwareCtx;
    dyplo::HardwareControl* hwControl;

};

#endif // DYPLOCONTEXT_H
