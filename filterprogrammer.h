#ifndef FILTERPROGRAMMER_H
#define FILTERPROGRAMMER_H

#include <QObject>
#include <QList>
#include <QMap>
#include <QSet>

class FilterProgrammer : public QObject
{
    Q_OBJECT
public:
    explicit FilterProgrammer(QObject *parent = 0);

    enum PrRegion
    {
        PrRegion1,
        PrRegion2,
        PrRegion3,
        PrRegion4,
        PrRegion5,
        PrRegion6,
        PrRegion7
    };

    enum Filters {
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

signals:

public slots:

private:
    QString getPartialFilename(Filters filter);

    typedef QSet<PrRegion> TPrRegionSet;
    typedef QMap<Filters, TPrRegionSet> TPartialPrRegionListMap;
    typedef QMap<Filters, PrRegion> TPartialPrRegionMap;
    typedef QMap<Filters, QString> TPartialFilenameMap;

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
