#include "fourierfilter.h"
#include <math.h>
#include <iostream>

FourierFilter::FourierFilter(int inputSize, int spectrumSize) :
    iInputSize(inputSize),
    iFourierBlockSize((iInputSize/2) + 1),
    iSpectrumSize(spectrumSize),
    iFourierInput(new float[iInputSize]),
    iFourierOutput(new fftwf_complex[iFourierBlockSize]),
    iPlan(fftwf_plan_dft_r2c_1d(iInputSize, iFourierInput, iFourierOutput, FFTW_ESTIMATE)),
    iResultBuffer(new float[iSpectrumSize])
{
    if (iSpectrumSize == iFourierBlockSize)
    {
        iDecline = 0;
    }
    else
    {
        iDecline = 1;
        while (count_decline(iFourierBlockSize, iDecline) < iSpectrumSize)
        {
            ++iDecline;
        }
    };
}

FourierFilter::~FourierFilter()
{
    fftwf_destroy_plan(iPlan);
    delete[] iFourierInput;
    delete[] iFourierOutput;
    delete[] iResultBuffer;
}

float* FourierFilter::getSpectrum(short* data)
{
    // convert short to float
    for (int i = 0; i < iInputSize; i++)
    {
        iFourierInput[i] = (float)data[i];
    }

    fftwf_execute(iPlan);

    if (iSpectrumSize == iFourierBlockSize)
    {
        // Special case: No change
        for (int i = 0; i < iFourierBlockSize; ++i)
            iResultBuffer[i] = cabsf(iFourierOutput[i]);
    }
    else
    {
        unsigned int block = iFourierBlockSize;
        unsigned int where = iFourierBlockSize;
        unsigned int index = iSpectrumSize;
        while (where)
        {
            unsigned int step = block/iDecline;
            if (step <= 1) break; // Done with the multi-factor ones
            where -= step;
            float sum = cabsf(iFourierOutput[where]);
            for (unsigned int i = where+1; i<block; ++i)
                sum += cabsf(iFourierOutput[i]);
            --index;
            iResultBuffer[index] = sum;
            block = where;
        }
        for (unsigned int i = 0; i < where; ++i)
            iResultBuffer[i] = cabsf(iFourierOutput[i]);
    }
    return iResultBuffer;
}

int FourierFilter::count_decline(int block_size, int decline)
{
    int result = 0;
    int sum = 0;
    int block = block_size;
    while (sum < block_size)
    {
        int step = block/decline;
        if (step < 1) step = 1;
        sum += step;
        block -= step;
        ++result;
    }
    return result;
}
