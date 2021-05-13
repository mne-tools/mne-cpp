#include "wavelet_spectrum_analyzer.h"

using namespace TIMEFREQUENCYLIB;

#define P_INPUT_DATA		0
#define P_SAMPLING_RATE		1
#define P_FREQ_INTERVAL		2
#define P_FREQ_COUNT		3
#define P_CYCLE_COUNT		4
#define P_SUPERRESOLUTION	5
#define P_MULTIPLICATIVE	6
#define P_FRACTIONAL		7


bool is_integer(double x)
{
	return x - (long long)x == 0;
}

int mxGetSize(mxArray* arr)
{
	return arr
		? (int)(mxGetM(arr) * mxGetN(arr))
		: 0;
}


void validate_params(int nrhs, mxArray** prhs)
{
	// check params
	if (nrhs < 6)
		mexErrMsgTxt("Parameter count not met! faslt expects at least 6 parameters! See below...\n \
		1. input_data				- Scalar Matrix		- Input buffers (row major) - each row is a trial\n \
		2. sampling_rate			- Scalar Number		- The sampling frequency in Hz\n \
		3. frequency_interval		- Scalar Vector		- tuple (vector of size 2) containing the lowest and highest frequency\n \
		4. frequency_count			- Scalar Number 	- the number of frequency bins in the interval\n \
		5. cycle_count				- Scalar Number		- the number of cycles of the shortest wavelets\n \
		6. superresolution_order	- Scalar Vector		- tuple (vector of size 2) containing the lowest and the highest superresolution orders\n \
		7. multiplicative			- Scalar Number		- 0 to use additive superresolution, multiplicative otherwise (default: true)\n \
		8. fractional				- Scalar Number		- 0 to use integral ASLT, uses fractional otherwise (default: true)");
	if (nrhs > 8)
		mexErrMsgTxt("Parameter count exceeded! faslt expects at most 8 parameters! See below...\n \
		1. input_data				- Scalar Matrix		- Input buffers (row major) - each row is a trial\n \
		2. sampling_rate			- Scalar Number		- The sampling frequency in Hz\n \
		3. frequency_interval		- Scalar Vector		- tuple (vector of size 2) containing the lowest and highest frequency\n \
		4. frequency_count			- Scalar Number 	- the number of frequency bins in the interval\n \
		5. cycle_count				- Scalar Number		- the number of cycles of the shortest wavelets\n \
		6. superresolution_order	- Scalar Vector		- tuple (vector of size 2) containing the lowest and the highest superresolution orders\n \
		7. multiplicative			- Scalar Number		- 0 to use additive superresolution, multiplicative otherwise (default: true)\n \
		8. fractional				- Scalar Number		- 0 to use integral ASLT, uses fractional otherwise (default: true)");

	// check data
	if (!mxIsDouble(prhs[P_INPUT_DATA]))
		mexErrMsgTxt("input_data needs to be a real-valued (double) matrix");

	// check input size
	if (!mxGetN(prhs[P_INPUT_DATA]) == 1)
		mexErrMsgTxt("input_data is a column vector. faslt only accepts row vectors.");

	// check FS
	if (mxGetScalar(prhs[P_SAMPLING_RATE]) <= 0.0)
		mexErrMsgTxt("sampling_rate needs to be a positive non-zero scalar representing the sampling frequency of the input in Hz");

	// check freq interval
	if (!mxIsDouble(prhs[P_FREQ_INTERVAL]) || mxGetSize(prhs[P_FREQ_INTERVAL]) != 2)
		mexErrMsgTxt("frequency_interval needs to be a tuple containing the lower and upper frequency bounds in Hz");
	else
	{
		double lower	= mxGetPr(prhs[P_FREQ_INTERVAL])[0];
		double upper	= mxGetPr(prhs[P_FREQ_INTERVAL])[1];
		double fs		= mxGetScalar(prhs[P_SAMPLING_RATE]);

		if (lower <= 0 || lower >= fs / 2 ||
			upper <= 0 || upper >= fs / 2)
			mexErrMsgTxt("frequency interval must not include DC (0) and Nyquist (sampling_rate / 2) frequencies");
	}

	// check frequency count
	if (!is_integer(mxGetScalar(prhs[P_FREQ_COUNT])) || mxGetScalar(prhs[P_FREQ_COUNT]) <= 0)
		mexErrMsgTxt("frequency_count needs to be a positive non-zero integer");

	// check cycle count
	if (mxGetScalar(prhs[P_CYCLE_COUNT]) <= 0.0)
		mexErrMsgTxt("cycle_count needs to be a positive non-zero scalar representing the number of cycles of the shortest wavelets");

	// check resolution
	if (!mxIsDouble(prhs[P_SUPERRESOLUTION]) || mxGetSize(prhs[P_SUPERRESOLUTION]) != 2)
		mexErrMsgTxt("superresolution needs to be a tuple containing the lower and upper frequency bounds in Hz");
	else
	{
		if (mxGetPr(prhs[P_SUPERRESOLUTION])[0] <= 0 || mxGetPr(prhs[P_SUPERRESOLUTION])[1] <= 0)
			mexErrMsgTxt("frequency interval must not include DC (0) and Nyquist (sampling_rate / 2) frequencies");
	}

	// check multiplicative
	if (nrhs > P_MULTIPLICATIVE && mxGetSize(prhs[P_MULTIPLICATIVE]) != 1)
		mexErrMsgTxt("multiplicative needs to be a scalar (0 - false, !0 otherwise)");

	// check fractional
	if (nrhs > P_FRACTIONAL && mxGetSize(prhs[P_FRACTIONAL]) != 1)
		mexErrMsgTxt("fractional needs to be a scalar (0 - false, !0 otherwise)");
}



void print_settings(wavelet_analyzer_settings& set)
{
	mexPrintf("wavelet_analyzer_settings:\n\
	sampling_rate:		%f\n\
	freq_low:			%f\n\
	freq_high:			%f\n\
	freq_count:			%d\n\
	input_size:			%d\n\
	wavelet_cycles:		%f\n\
	resolution_low:		%f\n\
	resolution_high:	%f\n\
	multiplicative:		%s\n\
	fractional:			%s\n",
		set.sampling_rate,
		set.freq_low,
		set.freq_high,
		set.freq_count,
		set.input_size,
		set.wavelet_cycles,
		set.resolution_low,
		set.resolution_high,
		set.multiplicative	? "true" : "false",
		set.fractional		? "true" : "false");
}


void __declspec(dllexport) mexFunction(int nlhs, mxArray** plhs, int nrhs, mxArray** prhs)
{
	//	Parameter Name			- Parameter Type							- Description
	//
	//  input_data				- Scalar Matrix								- Input buffers (row major) - each row is a trial
	//	sampling_rate			- Scalar Number								- The sampling frequency in Hz
	//	frequency_interval		- Scalar Vector								- tuple (vector of size 2) containing the lowest and highest frequency
	//	frequency_count			- Integer Number							- the number of frequency bins in the interval
	//	cycle_count				- Scalar Number								- the number of cycles of the shortest wavelets
	//	superresolution_order	- Scalar Vector								- tuple containing the lowest and the highest superresolution orders
	//	multiplicative			- Scalar Number								- 0 to use additive superresolution, multiplicative otherwise (default: true)
	//	fractional				- Scalar Number								- 0 to use integral ASLT, uses fractional otherwise (default: true)
	//
	//Return values
	//	S						- Scalar Matrix								- A matrix of size length(frequencies) x size(input_data, 2)

	validate_params(nrhs, prhs);

	// read parameters
	wavelet_analyzer_settings set;

	// input dimensions
	int input_count		= (int)mxGetM(prhs[P_INPUT_DATA]);
	set.input_size		= (int)mxGetN(prhs[P_INPUT_DATA]);

	// sampling rate
	set.sampling_rate	= (float)mxGetScalar(prhs[P_SAMPLING_RATE]);

	// frequencies
	set.freq_low		= (float)mxGetPr(prhs[P_FREQ_INTERVAL])[0];
	set.freq_high		= (float)mxGetPr(prhs[P_FREQ_INTERVAL])[1];
	set.freq_count		= (int)mxGetScalar(prhs[P_FREQ_COUNT]);

	// res order
	set.wavelet_cycles	= (float)mxGetScalar(prhs[P_CYCLE_COUNT]);
	set.resolution_low	= (float)mxGetPr(prhs[P_SUPERRESOLUTION])[0];
	set.resolution_high = (float)mxGetPr(prhs[P_SUPERRESOLUTION])[1];

	// flags
	set.multiplicative	= nrhs > P_MULTIPLICATIVE	? (bool)mxGetScalar(prhs[P_MULTIPLICATIVE]) : true;
	set.fractional		= nrhs > P_FRACTIONAL		? (bool)mxGetScalar(prhs[P_FRACTIONAL])		: true;

	// create output
	nlhs	= 1;
	plhs[0] = mxCreateDoubleMatrix(set.freq_count, set.input_size, mxREAL);

	// alloc temporary buffers (for float conversion and IO)
	float *flt_input_data	= static_cast<float*>(mxCalloc(set.input_size, sizeof(float)));
	float *flt_result		= static_cast<float*>(mxCalloc(size_t(set.freq_count) * set.input_size, sizeof(float)));
	double* in_ptr			= mxGetPr(prhs[P_INPUT_DATA]);
	double*	out_ptr			= mxGetPr(plhs[0]);

	// check buffers
	if (!flt_input_data || !flt_result)
	{
		mexErrMsgTxt("Internal error!");
		return;
	}

	// create analyzer
	wavelet_spectrum_analyzer anl(set);

	// perform analysis
	for (int i = 0; i < input_count; ++i)
	{
		for (int j = 0; j < set.input_size; ++j)
			flt_input_data[j] = (float)in_ptr[i + j * input_count];

		anl.analyze(flt_input_data, flt_result);
	}

	// normalize and copy output
	double idiv = 1.0 / input_count;
	for (int i = 0; i < set.freq_count; ++i)
	{
		for (int j = 0; j < set.input_size; ++j)
		{
			out_ptr[i + j * set.freq_count] = flt_result[i * set.input_size + j] * idiv;
		}
	}

	mxFree(flt_input_data);
	mxFree(flt_result);
}






