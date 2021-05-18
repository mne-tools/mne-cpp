#pragma once
#include <vector>
#include <complex>
#include <cmath>

namespace TIMEFREQUENCYLIB {

static float morlet_SD_factor	= 2.5f;
static float morlet_SD_spread	= 6;
static float pi_f				= 3.14159265359f;

class morlet
{
public:
	morlet(float center_freq, float cycle_count, float sampling_rate)
	{
		float SD		= (cycle_count / 2) * (1 / center_freq) / morlet_SD_factor;
		int wl_size		= int(2 * std::floor(std::round(SD * sampling_rate * morlet_SD_spread) / 2) + 1);
		int half_size	= wl_size / 2;

		_data.resize(wl_size);
		float integral;
		std::vector<float> gauss(std::move(gausswin(wl_size, morlet_SD_spread / 2, integral)));

		float igsum	= 1.f / integral;
		float isd	= 1.f / sampling_rate;

		for (int i = 0; i < wl_size; ++i)
		{
			float t = (i - half_size) * isd;
			_data[i] = gauss[i] * expj(2 * pi_f * center_freq * t) * igsum;
		}
	}


	int size() { return int(_data.size()); }

	std::complex<float>* data() { return _data.data(); }


protected:
	static std::vector<float> gausswin(int n, float alpha, float& integral)
	{
		int halfsize	= n / 2;
		float idiv		= alpha / halfsize;

		std::vector<float> result(n);
		double acc = 0;
		for (int i = 0; i < n; ++i)
		{
			float t		= (i - halfsize) * idiv;
			result[i]	= std::exp(-(t * t) * 0.5f);
			acc			+= result[i];
		}

		integral = float(acc);
		return result;
	}

	static std::complex<float> expj(float arg)
	{
		return std::complex<float>(std::cos(arg), std::sin(arg));
	}



	std::vector<std::complex<float>> _data;


};

}
