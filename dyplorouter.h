#ifndef DYPLOROUTER_H
#define DYPLOROUTER_H

#include <QObject>

#include "filterprogrammer.h"
#include "dyplocontext.h"

// responsible for setting up the routes of the activated filters in Dyplo.
class DyploRouter
{
public:
    explicit DyploRouter(DyploContext& context, FilterProgrammer& programmer);

    enum Demo
    {
        VideoDemo,
        AudioDemo,
        MandelbrotDemo
    };

    void RouteFilter(FilterProgrammer::Filters filter);
    void UnrouteFilter(FilterProgrammer::Filters filter);

    // get the output node of a specific demo
    int GetDemoOutputNode(Demo demo);

private:
    // ordered list of the routed filters
    QList<FilterProgrammer::Filters> routedFilters;

    DyploContext& dyploContext;
    FilterProgrammer& filterProgrammer;
};

#endif // DYPLOROUTER_H
