#include "dyplorouter.h"

DyploRouter::DyploRouter(DyploContext& context,
                         FilterProgrammer& programmer) :
    dyploContext(context),
    filterProgrammer(programmer)
{

}

void DyploRouter::RouteFilter(FilterProgrammer::Filters filter)
{
    (void)filter;
    // TODO
}

void DyploRouter::UnrouteFilter(FilterProgrammer::Filters filter)
{
    (void)filter;
    // TODO
}

int DyploRouter::GetDemoOutputNode(DyploRouter::Demo demo)
{
    switch (demo)
    {
        case VideoDemo:
            return 0;
        case AudioDemo:
            return 1;
        case MandelbrotDemo:
            return 2;
        default:
            break;
    }

    return -1;
}
