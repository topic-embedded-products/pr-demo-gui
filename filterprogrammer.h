#ifndef FILTERPROGRAMMER_H
#define FILTERPROGRAMMER_H

#include <QList>
#include <QMap>
#include <QSet>
#include "dyplocontext.h"

// responsible for programming filters in available PR regions
class FilterProgrammer
{
public:
    explicit FilterProgrammer(DyploContext& context);

    enum PrRegion
    {
        PrRegion1 = 0,
        PrRegion2 = 1,
        PrRegion3 = 2,
        PrRegion4 = 3,
        PrRegion5 = 4,
        PrRegion6 = 5,
        PrRegion7 = 6
    };

    enum Filters {
        FilterUndefined,
        FilterVideoGrayscale,
        FilterVideoLaplacian,
        FilterAudioLowpass,
        FilterAudioHighpass,
        FilterAudioFFT,
        FilterMandelbrot
    };

    // returns if the programming succeeded.
    // output parameter indicates which region was used to program succesfully.
    bool ProgramFilter(Filters filter, PrRegion& programmedInRegion);

    // returns if the disabling succeeded.
    // output parameter indicates which region was disabled.
    bool DisableFilter(Filters filter, PrRegion& disabledRegion);


    bool IsFilterProgrammed(Filters filter);
    PrRegion GetProgramRegion(Filters filter);

signals:

public slots:

private:
    QString getPartialFilename(Filters filter);

    typedef QSet<PrRegion> TPrRegionSet;
    typedef QMap<Filters, TPrRegionSet> TPartialPrRegionListMap;
    typedef QMap<Filters, PrRegion> TPartialPrRegionMap;
    typedef QMap<Filters, QString> TPartialFilenameMap;

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
