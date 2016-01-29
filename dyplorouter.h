#ifndef DYPLOROUTER_H
#define DYPLOROUTER_H

#include <QObject>
#include <QMap>

#include "filterprogrammer.h"
#include "dyplocontext.h"

// responsible for setting up the routes of the activated filters in Dyplo.
class DyploRouter
{
public:
    explicit DyploRouter(DyploContext& context, FilterProgrammer& programmer);

    enum Demo
    {
        UndefinedDemo,
        VideoDemo,
        AudioDemo,
        MandelbrotDemo
    };

    void RouteFilter(FilterProgrammer::Filters filter);
    void UnrouteFilter(FilterProgrammer::Filters filter);

    // get the output node of a specific demo
    int GetDemoOutputNode(Demo demo);

    // get the input node of a specific demo
    int GetDemoInputNode(Demo demo);

private:
    Demo getDemoByFilter(FilterProgrammer::Filters filter);
    FilterProgrammer::Filters getFilterRoutedBefore(FilterProgrammer::Filters filter);
    FilterProgrammer::Filters getFilterRoutedAfter(FilterProgrammer::Filters filter);

    typedef QList<FilterProgrammer::Filters> TFilterList;

    // ordered list of the routed filters
    QMap<Demo, TFilterList> iRoutedFilters;

    typedef QMap<Demo, TFilterList> TDemoFilterMap;
    TDemoFilterMap filtersPerDemo;

    DyploContext& dyploContext;
    FilterProgrammer& filterProgrammer;
};

#endif // DYPLOROUTER_H
