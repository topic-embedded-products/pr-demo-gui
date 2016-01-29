#include "filterprogrammer.h"

FilterProgrammer::FilterProgrammer(DyploContext& context) :
    dyploContext(context)
{
    prRegions.insert(PrRegion1);
    prRegions.insert(PrRegion2);
    prRegions.insert(PrRegion3);
    prRegions.insert(PrRegion4);
    prRegions.insert(PrRegion5);
    prRegions.insert(PrRegion6);
    prRegions.insert(PrRegion7);

    partialsForRegion[FilterVideoGrayscale].insert(PrRegion1);
    partialsForRegion[FilterVideoGrayscale].insert(PrRegion2);
    partialsForRegion[FilterVideoGrayscale].insert(PrRegion3);
    partialsForRegion[FilterVideoGrayscale].insert(PrRegion4);
    partialsForRegion[FilterVideoGrayscale].insert(PrRegion5);
    partialsForRegion[FilterVideoGrayscale].insert(PrRegion6);
    partialsForRegion[FilterVideoGrayscale].insert(PrRegion7);

    partialsForRegion[FilterVideoLaplacian].insert(PrRegion1);
    partialsForRegion[FilterVideoLaplacian].insert(PrRegion2);
    partialsForRegion[FilterVideoLaplacian].insert(PrRegion3);
    partialsForRegion[FilterVideoLaplacian].insert(PrRegion4);
    partialsForRegion[FilterVideoLaplacian].insert(PrRegion5);
    partialsForRegion[FilterVideoLaplacian].insert(PrRegion6);
    partialsForRegion[FilterVideoLaplacian].insert(PrRegion7);

    partialsForRegion[FilterAudioLowpass].insert(PrRegion1);
    partialsForRegion[FilterAudioLowpass].insert(PrRegion2);
    partialsForRegion[FilterAudioLowpass].insert(PrRegion3);
    partialsForRegion[FilterAudioLowpass].insert(PrRegion4);
    partialsForRegion[FilterAudioLowpass].insert(PrRegion5);
    partialsForRegion[FilterAudioLowpass].insert(PrRegion6);
    partialsForRegion[FilterAudioLowpass].insert(PrRegion7);

    partialsForRegion[FilterAudioHighpass].insert(PrRegion1);
    partialsForRegion[FilterAudioHighpass].insert(PrRegion2);
    partialsForRegion[FilterAudioHighpass].insert(PrRegion3);
    partialsForRegion[FilterAudioHighpass].insert(PrRegion4);
    partialsForRegion[FilterAudioHighpass].insert(PrRegion5);
    partialsForRegion[FilterAudioHighpass].insert(PrRegion6);
    partialsForRegion[FilterAudioHighpass].insert(PrRegion7);

    partialsForRegion[FilterAudioFFT].insert(PrRegion1);
    partialsForRegion[FilterAudioFFT].insert(PrRegion2);
    partialsForRegion[FilterAudioFFT].insert(PrRegion3);
    partialsForRegion[FilterAudioFFT].insert(PrRegion4);
    partialsForRegion[FilterAudioFFT].insert(PrRegion5);
    partialsForRegion[FilterAudioFFT].insert(PrRegion6);
    partialsForRegion[FilterAudioFFT].insert(PrRegion7);

    partialsForRegion[FilterMandelbrot].insert(PrRegion1);
    partialsForRegion[FilterMandelbrot].insert(PrRegion2);
    partialsForRegion[FilterMandelbrot].insert(PrRegion3);
    partialsForRegion[FilterMandelbrot].insert(PrRegion4);
    partialsForRegion[FilterMandelbrot].insert(PrRegion5);
    partialsForRegion[FilterMandelbrot].insert(PrRegion6);
    partialsForRegion[FilterMandelbrot].insert(PrRegion7);

    // TEMPORARY NAMES:
    partialFileNames[FilterVideoGrayscale] = "video_grayscale_node";
    partialFileNames[FilterVideoLaplacian] = "video_laplacian_node";
    partialFileNames[FilterAudioFFT] = "audio_fft_node";
    partialFileNames[FilterAudioHighpass] = "audio_high_pass_node";
    partialFileNames[FilterAudioLowpass] = "audio_low_pass_node";
    partialFileNames[FilterMandelbrot] = "mandelbrot_node";
}

bool FilterProgrammer::ProgramFilter(Filters filter,
                                     PrRegion& programmedInRegion)
{
   bool programmed = false;

   Q_ASSERT(partialsForRegion.contains(filter));

   // find available node
   TPrRegionSet::iterator it;

   for (it = prRegions.begin(); it != prRegions.end() && !programmed; ++it)
   {
       const PrRegion& region = *it;
       // check if PR region is already programmed
       if (!programmedPartials.values().contains(region))
       {
           // TODO: program partial using Dyplo API
           QString partialFileName = getPartialFilename(filter);

           programmed = true;

           programmedPartials[filter] = region;
           programmedInRegion = region;
       }
   }

   return programmed;
}

bool FilterProgrammer::DisableFilter(FilterProgrammer::Filters filter,
                                     FilterProgrammer::PrRegion& disabledRegion)
{
    bool disabled = false;

    if (programmedPartials.contains(filter))
    {
        PrRegion& region = programmedPartials[filter];

        // TODO: disable partial using Dyplo API

        disabledRegion = region;
        disabled = true;

        programmedPartials.remove(filter);
    }

    return disabled;
}

bool FilterProgrammer::IsFilterProgrammed(FilterProgrammer::Filters filter)
{
    return programmedPartials.contains(filter);
}

FilterProgrammer::PrRegion FilterProgrammer::GetProgramRegion(FilterProgrammer::Filters filter)
{
    Q_ASSERT(IsFilterProgrammed(filter));
    return programmedPartials[filter];
}

QString FilterProgrammer::getPartialFilename(FilterProgrammer::Filters filter)
{
    Q_ASSERT(partialFileNames.contains(filter));
    return partialFileNames[filter];
}
