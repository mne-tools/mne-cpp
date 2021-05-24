#pragma once
//#include "morlet.h"
//#include "convolver.h"


//namespace TIMEFREQUENCYLIB {

//struct wavelet_analyzer_settings
//{
//	float	sampling_rate;
//	float	freq_low;
//	float	freq_high;
//	int		freq_count;
//	int		input_size;
//	float	wavelet_cycles;
//	float	resolution_low;
//	float	resolution_high;
//	bool	multiplicative;
//	bool	fractional;
//};


//class wavelet_spectrum_analyzer
//{
//	using filter_bank	= std::vector<convolver*>;
//    using superlet		= std::vector<morlet>;

//public:
//	wavelet_spectrum_analyzer(wavelet_analyzer_settings set);
//	~wavelet_spectrum_analyzer();

//	void analyze(float* input, float* output);

//protected:
//	wavelet_analyzer_settings			_set;
//	std::vector<filter_bank>			_filters;
//	std::vector<superlet>				_superlets;
//	std::vector<std::complex<float>>	_conv_buffer;
//	std::vector<double>					_pooling_buffer;
//	std::vector<float>					_frequencies;
//	std::vector<float>					_orders;

//};

//}

