#include "dyplorouter.h"
#include <QDebug>

DyploRouter::DyploRouter(DyploContext& context,
                         FilterProgrammer& programmer) :
    dyploContext(context),
    filterProgrammer(programmer)
{
    filtersPerDemo[AudioDemo].append(FilterProgrammer::FilterAudioFFT);
    filtersPerDemo[AudioDemo].append(FilterProgrammer::FilterAudioLowpass);
    filtersPerDemo[AudioDemo].append(FilterProgrammer::FilterAudioHighpass);

    filtersPerDemo[VideoDemo].append(FilterProgrammer::FilterAudioFFT);
    filtersPerDemo[VideoDemo].append(FilterProgrammer::FilterAudioLowpass);

    filtersPerDemo[MandelbrotDemo].append(FilterProgrammer::FilterMandelbrot);
}

void DyploRouter::RouteFilter(FilterProgrammer::Filters filter)
{
    // get demo
    Demo targetDemo = getDemoByFilter(filter);

    // get the PR region that the new filter is programmed on
    FilterProgrammer::PrRegion newRegion = filterProgrammer.GetProgramRegion(filter);

    TFilterList& routedFilters = iRoutedFilters[targetDemo];

    // get programmed filters per demo
    if (!routedFilters.empty())
    {
        // get the last programmed filter
        FilterProgrammer::Filters lastFilter = routedFilters.back();

        // get the PR region that the last filter is programmed on
        FilterProgrammer::PrRegion lastRegion = filterProgrammer.GetProgramRegion(lastFilter);

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

void DyploRouter::UnrouteFilter(FilterProgrammer::Filters filter)
{
    try
    {
        // get demo
        Demo targetDemo = getDemoByFilter(filter);
        const TFilterList& routedFilters = iRoutedFilters[targetDemo];

        FilterProgrammer::PrRegion filterRegion = filterProgrammer.GetProgramRegion(filter);

        // if there is a previous filter or a next filter, reroute it
        FilterProgrammer::Filters beforeFilter = getFilterRoutedBefore(filter);
        FilterProgrammer::Filters afterFilter = getFilterRoutedAfter(filter);

        if (beforeFilter != FilterProgrammer::FilterUndefined)
        {
            if (afterFilter != FilterProgrammer::FilterUndefined)
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
        } else if (afterFilter != FilterProgrammer::FilterUndefined)
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

int DyploRouter::GetDemoOutputNode(DyploRouter::Demo demo)
{
    // TEMPORARY VALUES:
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

int DyploRouter::GetDemoInputNode(DyploRouter::Demo demo)
{
    // TEMPORARY VALUES:
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

DyploRouter::Demo DyploRouter::getDemoByFilter(FilterProgrammer::Filters filter)
{
    for (TDemoFilterMap::iterator it = filtersPerDemo.begin(); it != filtersPerDemo.end(); ++it)
    {
        TFilterList& filterList = *it;

        if (filterList.contains(filter))
        {
            return it.key();
        }
    }

    return UndefinedDemo;
}

FilterProgrammer::Filters DyploRouter::getFilterRoutedBefore(FilterProgrammer::Filters filter)
{
    Demo targetDemo = getDemoByFilter(filter);
    const TFilterList& routedFilters = iRoutedFilters[targetDemo];

    FilterProgrammer::Filters beforeFilter = FilterProgrammer::FilterUndefined;

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

FilterProgrammer::Filters DyploRouter::getFilterRoutedAfter(FilterProgrammer::Filters filter)
{
    Demo targetDemo = getDemoByFilter(filter);
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

    return FilterProgrammer::FilterUndefined;
}
