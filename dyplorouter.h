#ifndef DYPLOROUTER_H
#define DYPLOROUTER_H

#include <QObject>
#include <QMap>

#include "filterprogrammer.h"
#include "dyplocontext.h"
#include "types.h"

// responsible for setting up the routes of the activated filters in Dyplo.
class DyploRouter
{
public:
    explicit DyploRouter(DyploContext& context, FilterProgrammer& programmer);

    void RouteFilter(EFilters filter);
    void UnrouteFilter(EFilters filter);

    // get the output node of a specific demo
    int GetDemoOutputNode(EDemo demo);

    // get the input node of a specific demo
    int GetDemoInputNode(EDemo demo);

private:
    EDemo getDemoByFilter(EFilters filter);
    EFilters getFilterRoutedBefore(EFilters filter);
    EFilters getFilterRoutedAfter(EFilters filter);

    typedef QList<EFilters> TFilterList;

    // ordered list of the routed filters
    QMap<EDemo, TFilterList> iRoutedFilters;

    typedef QMap<EDemo, TFilterList> TDemoFilterMap;
    TDemoFilterMap filtersPerDemo;

    DyploContext& dyploContext;
    FilterProgrammer& filterProgrammer;
};

#endif // DYPLOROUTER_H
