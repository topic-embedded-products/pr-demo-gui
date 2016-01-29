#ifndef FILTERPROGRAMMER_H
#define FILTERPROGRAMMER_H

#include <QList>
#include <QMap>
#include <QSet>
#include "dyplocontext.h"
#include "types.h"

// responsible for programming filters in available PR regions
class FilterProgrammer
{
public:
    explicit FilterProgrammer(DyploContext& context);

    // returns if the programming succeeded.
    // output parameter indicates which region was used to program succesfully and how long it took.
    bool ProgramFilter(EFilters filter, EPrRegion& programmedInRegion, quint32& programmedInMs);

    // returns if the disabling succeeded.
    // output parameter indicates which region was disabled.
    bool DisableFilter(EFilters filter, EPrRegion& disabledRegion);


    bool IsFilterProgrammed(EFilters filter);
    EPrRegion GetProgramRegion(EFilters filter);

signals:

public slots:

private:
    QString getPartialFilename(EFilters filter);

    typedef QSet<EPrRegion> TPrRegionSet;
    typedef QMap<EFilters, TPrRegionSet> TPartialPrRegionListMap;
    typedef QMap<EFilters, EPrRegion> TPartialPrRegionMap;
    typedef QMap<EFilters, QString> TPartialFilenameMap;

    DyploContext& dyploContext;

    // Filter mapped to filename
    TPartialFilenameMap partialFileNames;

    // List of regions for which a filter is available
    TPartialPrRegionListMap partialsForRegion;

    // PR regions on system
    TPrRegionSet prRegions;

    // What is programmed and what is available:
    TPartialPrRegionMap programmedPartials;
};

#endif // FILTERPROGRAMMER_H
