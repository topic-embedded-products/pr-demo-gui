#ifndef FOURIERFILTER_H
#define FOURIERFILTER_H

#include "fftw3.h"

class FourierFilter
{
public:
    FourierFilter(int inputSize, int spectrumSize);
    ~FourierFilter();

    // audio will be sampled as 'short', 16-bit.
    // previously used result is invalidated when you call this function again.
    float* getSpectrum(short* data);

private:
    int count_decline(int block_size,
                      int decline);

    int iInputSize;
    int iFourierBlockSize;
    int iSpectrumSize;
    int iDecline;

    float*          iFourierInput;
    fftwf_complex*  iFourierOutput;
    fftwf_plan      iPlan;
    float*          iResultBuffer;
};

#endif // FOURIERFILTER_H
