#include "fourierfilter.h"
#include "yaffut.h"

#define BENCHMARK_TIME_US 100000

static const float PI = 3.1415926535897932384626f;

template <class T> static void gen_signal(unsigned N, T* out, float ampl_sin, float ampl_cos)
{
	for (unsigned i = 0; i < N; ++i)
	{
		out[i] = static_cast<T>(
			ampl_sin * sin(i * 2 * PI * 8 / N) +
			ampl_cos * cos(i * 2 * PI * 17 / N));
	}
}

static int imported_wisdom = fftwf_import_system_wisdom();

#define FAIL_EX(index, expect, actual) \
	{\
		std::ostringstream msg; \
                msg << "Mismatch at " << index << " expected: " << expect << " actual: " << actual; \
		FAIL(msg.str()); \
	}

struct FFT {};

TEST(FFT, spectrum_calculation)
{
	const unsigned int N = 1024;
        const unsigned int S = (N/2)+1;
	const float factor = 2.0f / N;
	const float delta = 0.5f;

        FourierFilter fourier(N, S);
        for (int repeat = 5; repeat != 0; --repeat)
	{
                short input_data[N];
		gen_signal(N, &input_data[0], 2000.0f * repeat, 3000.0f * (6-repeat)); // Create input
		float* spectrum = fourier.getSpectrum(&input_data[0]);

                for (int i = 0; i < S; i++)
                {
                    std::cout << std::endl << i << ": " << spectrum[i];
                }
				
		// Test spectrum calculation
		for (unsigned int i = 0; i < 8; ++i)
		{
			float v = spectrum[i] * factor;
			if (!((v < delta) && (v > -delta)))
				FAIL_EX(i, 0.0f, v);
		}
		for (unsigned int i = 9; i < 17; ++i)
		{
			float v = spectrum[i] * factor;
			if (!((v < delta) && (v > -delta)))
				FAIL_EX(i, 0.0f, v);
		}
		for (unsigned int i = 18; i < S; ++i)
		{
			float v = spectrum[i] * factor;
			if (!((v < delta) && (v > -delta)))
				FAIL_EX(i, 0.0f, v);
                }
		{
			float expect = 2000.0f * repeat;
			float v = spectrum[8] * factor;
			if (abs(v - expect) > delta)
				EQUAL(expect, v);
		}
		{
			float expect = 3000.0f * (6-repeat);
			float v = spectrum[17] * factor;
			if (abs(v - expect) > delta)
				EQUAL(expect, v);
                }
	}
}
