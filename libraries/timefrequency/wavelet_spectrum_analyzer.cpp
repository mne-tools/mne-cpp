//#include "wavelet_spectrum_analyzer.h"
//#include <algorithm>

//using namespace TIMEFREQUENCYLIB;

//std::vector<float> linspace(float from, float to, int n)
//{
//	if		(n <= 0) return { };
//	else if (n == 1) return { from };
//	else
//	{
//		std::vector<float> result(n);

//		float dv = (to - from) / (n - 1);
//		for (size_t i = 0; i < result.size(); ++i)
//			result[i] = from + i * dv;

//		return result;
//	}
//}


//__forceinline float mag_sqr(const std::complex<float>& z)
//{
//	return z.real() * z.real() + z.imag() * z.imag();
//}

//__forceinline float fractional(float x)
//{
//	return x - (int)x;
//}




//wavelet_spectrum_analyzer::wavelet_spectrum_analyzer(wavelet_analyzer_settings set)
//{
//	_set			= set;
//	_filters		.resize(_set.freq_count);
//	_superlets		.resize(_set.freq_count);
//	_conv_buffer	.resize(_set.input_size);
//	_pooling_buffer	.resize(_set.input_size);
//	_frequencies	= linspace(_set.freq_low, _set.freq_high, _set.freq_count);
//	_orders			= linspace(_set.resolution_low, _set.resolution_high, _set.freq_count);

//	if (!_set.fractional)
//		std::transform(_orders.begin(), _orders.end(), _orders.begin(), std::roundf);

//	for (size_t i_freq = 0; i_freq < _set.freq_count; ++i_freq)
//	{
//		float center_freq	= _frequencies[i_freq];
//		int n_wavelets		= int(std::ceilf(_orders[i_freq]));

//		for (size_t i_wave = 0; i_wave < n_wavelets; ++i_wave)
//		{
//			float ncyc = _set.multiplicative
//				? (i_wave + 1) * _set.wavelet_cycles
//				: _set.wavelet_cycles + i_wave;

//			_superlets[i_freq]	.emplace_back(center_freq, ncyc, _set.sampling_rate);
//			_filters[i_freq]	.push_back(new convolver(_set.input_size, _superlets[i_freq].back().size()));
//			_filters[i_freq]	.back()->assign_kernel(_superlets[i_freq].back().data(), _superlets[i_freq].back().size());
//		}
//	}
//}

//wavelet_spectrum_analyzer::~wavelet_spectrum_analyzer()
//{
//	for (auto& fb : _filters)
//	{
//		for (auto& conv : fb)
//			delete conv;
//		fb.clear();
//	}

//	_filters		.clear();
//	_superlets		.clear();
//	_pooling_buffer	.clear();
//	_frequencies	.clear();
//	_orders			.clear();
//	_conv_buffer	.clear();
//	_set			= wavelet_analyzer_settings();
//}



//void wavelet_spectrum_analyzer::analyze(float* input, float* output)
//{
//	size_t input_size = _set.input_size;

//	for (size_t i_freq = 0; i_freq < _superlets.size(); ++i_freq)
//	{
//		// initialize the pooling buffer
//		std::fill(_pooling_buffer.begin(), _pooling_buffer.end(), 1.0);

//		if (_superlets[i_freq].size() > 1)
//		{
//			// superresolution
//			size_t n_wavelets	= size_t(std::floorf(_orders[i_freq]));
//			double r_factor		= 1.0 / n_wavelets;

//			for (size_t i_wave = 0; i_wave < n_wavelets; ++i_wave)
//			{
//				// perform R2C convolution
//				_filters[i_freq][i_wave]->conv_r2c_same(input, _conv_buffer.data());

//				// pool with exponent = 1
//				for (size_t i = 0; i < input_size; ++i)
//				{
//					_pooling_buffer[i] *= 2.0 * mag_sqr(_conv_buffer[i]);
//				}
//			}

//			// determine if fractional superlet should be used
//			if (fractional(_orders[i_freq]) != 0			&&	// order is fractional
//				_set.fractional								&&	// we are allowed to use it
//				_superlets[i_freq].size() == n_wavelets + 1	)	// we really have a wavelet available for it
//			{
//				double exponent = fractional(_orders[i_freq]);
//				r_factor		= 1.0 / (n_wavelets + exponent);

//				// perform convolution with the last wavelet
//				_filters[i_freq].back()->conv_r2c_same(input, _conv_buffer.data());

//				// pool with fractional exponent
//				for (size_t i = 0; i < input_size; ++i)
//				{
//					_pooling_buffer[i] *= std::pow(2.0 * mag_sqr(_conv_buffer[i]), exponent);
//				}
//			}

//			// perform geometric mean and save to output buffer
//			for (size_t i = 0; i < input_size; ++i)
//			{
//				output[i] += float(std::pow(_pooling_buffer[i], r_factor));
//			}
//		}
//		else
//		{
//			// standard CWT
//			_filters[i_freq].front()->conv_r2c_same(input, _conv_buffer.data());

//			for (size_t i = 0; i < input_size; ++i)
//				output[i] += float(2.0 * mag_sqr(_conv_buffer[i]));
//		}

//		output += input_size;
//	}
//}



