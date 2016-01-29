#pragma once

enum EPrRegion
{
    // value should match Dyplo Node ID
    PrRegion1 = 0,
    PrRegion2 = 1,
    PrRegion3 = 2,
    PrRegion4 = 3,
    PrRegion5 = 4,
    PrRegion6 = 5,
    PrRegion7 = 6
};

enum EFilters {
    FilterUndefined,
    FilterVideoGrayscale,
    FilterVideoLaplacian,
    FilterAudioLowpass,
    FilterAudioHighpass,
    FilterAudioFFT,
    FilterMandelbrot
};

enum EDemo
{
    DemoUndefined,
    DemoVideo,
    DemoAudio,
    DemoMandelbrot
};
