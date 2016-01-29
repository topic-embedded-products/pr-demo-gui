#include "dyplorouter.h"
#include <QDebug>

DyploRouter::DyploRouter(DyploContext& context,
                         FilterProgrammer& programmer) :
    dyploContext(context),
    filterProgrammer(programmer)
{
    filtersPerDemo[DemoAudio].append(FilterAudioFFT);
    filtersPerDemo[DemoAudio].append(FilterAudioLowpass);
    filtersPerDemo[DemoAudio].append(FilterAudioHighpass);

    filtersPerDemo[DemoVideo].append(FilterVideoGrayscale);
    filtersPerDemo[DemoVideo].append(FilterVideoLaplacian);

    filtersPerDemo[DemoMandelbrot].append(FilterMandelbrot);
}

void DyploRouter::RouteFilter(EFilters filter)
{
    // get demo
    EDemo targetDemo = getDemoByFilter(filter);

    // get the PR region that the new filter is programmed on
    EPrRegion newRegion = filterProgrammer.GetProgramRegion(filter);

    TFilterList& routedFilters = iRoutedFilters[targetDemo];

    // get programmed filters per demo
    if (!routedFilters.empty())
    {
        // get the last programmed filter
        EFilters lastFilter = routedFilters.back();

        // get the PR region that the last filter is programmed on
        EPrRegion lastRegion = filterProgrammer.GetProgramRegion(lastFilter);

        try
        {
            // TODO: test and implement fully
            // route the filter that was created last to the new region
            //DyploContext::getInstance().GetHardwareControl().routeAddSingle(lastRegion, 0, newRegion, 0);
            // route new filter to output
            //DyploContext::getInstance().GetHardwareControl().routeAddSingle(newRegion, 0, GetDemoOutputNode(targetDemo), 0);
            iRoutedFilters[targetDemo].push_back(filter);
        }
        catch (const std::exception& ex)
        {
            qCritical() << "ERROR:\n" << QString(ex.what());
        }
    }
    else
    {
        try
        {
            // TODO: test and implement fully
            // route input for the demo to the new filter
            //DyploContext::getInstance().GetHardwareControl().routeAddSingle(GetDemoInputNode(targetDemo), 0, newRegion, 0);
            // route new filter to output
            //DyploContext::getInstance().GetHardwareControl().routeAddSingle(newRegion, 0, GetDemoOutputNode(targetDemo), 0);

            // register routed filter
            iRoutedFilters[targetDemo].push_back(filter);
        }
        catch (const std::exception& ex)
        {
            qCritical() << "ERROR:\n" << QString(ex.what());
        }
    }
}

void DyploRouter::UnrouteFilter(EFilters filter)
{
    try
    {
        // get demo
        EDemo targetDemo = getDemoByFilter(filter);

        // if there is a previous filter or a next filter, it needs to be rerouted
        EFilters beforeFilter = getFilterRoutedBefore(filter);
        EFilters afterFilter = getFilterRoutedAfter(filter);

        if (beforeFilter != FilterUndefined)
        {
            if (afterFilter != FilterUndefined)
            {
                // route beforefilter to afterfilter
                // TODO: test and implement fully
                //DyploContext::getInstance().GetHardwareControl().routeAddSingle(beforeFilter, 0, afterFilter, 0);
            }
            else
            {
                // route beforeFilter to Output node
                // TODO: test and implement fully
                //DyploContext::getInstance().GetHardwareControl().routeAddSingle(beforeFilter, 0, GetDemoOutputNode(targetDemo), 0);
            }
        } else if (afterFilter != FilterUndefined)
        {
            // route Input node to afterfilter
            // TODO: test and implement fully
            //DyploContext::getInstance().GetHardwareControl().routeAddSingle(GetDemoInputNode(targetDemo), 0, afterFilter, 0);
        }

        // remove unrouted filter from list
        iRoutedFilters[targetDemo].removeAll(filter);
    }
    catch (const std::exception& ex)
    {
        qCritical() << "ERROR:\n" << QString(ex.what());
    }
}

int DyploRouter::GetDemoOutputNode(EDemo demo)
{
    // TEMPORARY VALUES:
    switch (demo)
    {
        case DemoVideo:
            return 0;
        case DemoAudio:
            return 1;
        case DemoMandelbrot:
            return 2;
        default:
            break;
    }

    return -1;
}

int DyploRouter::GetDemoInputNode(EDemo demo)
{
    // TEMPORARY VALUES:
    switch (demo)
    {
        case DemoVideo:
            return 0;
        case DemoAudio:
            return 1;
        case DemoMandelbrot:
            return 2;
        default:
            break;
    }

    return -1;
}

EDemo DyploRouter::getDemoByFilter(EFilters filter)
{
    for (TDemoFilterMap::iterator it = filtersPerDemo.begin(); it != filtersPerDemo.end(); ++it)
    {
        TFilterList& filterList = *it;

        if (filterList.contains(filter))
        {
            return it.key();
        }
    }

    return DemoUndefined;
}

EFilters DyploRouter::getFilterRoutedBefore(EFilters filter)
{
    EDemo targetDemo = getDemoByFilter(filter);
    const TFilterList& routedFilters = iRoutedFilters[targetDemo];

    EFilters beforeFilter = FilterUndefined;

    for (TFilterList::const_iterator it = routedFilters.begin(); it != routedFilters.end(); it++)
    {
        if (*it == filter)
        {
            return beforeFilter;
        }

        beforeFilter = *it;
    }

    return beforeFilter;
}

EFilters DyploRouter::getFilterRoutedAfter(EFilters filter)
{
    EDemo targetDemo = getDemoByFilter(filter);
    const TFilterList& routedFilters = iRoutedFilters[targetDemo];

    bool foundFilter = false;

    for (TFilterList::const_iterator it = routedFilters.begin(); it != routedFilters.end(); it++)
    {
        if (foundFilter)
        {
            return *it;
        }

        if (*it == filter)
        {
            foundFilter = true;
        }
    }

    return FilterUndefined;
}
