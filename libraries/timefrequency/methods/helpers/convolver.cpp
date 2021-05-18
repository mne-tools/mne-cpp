#include "convolver.h"
#include "fftw.h"

#include <mutex>
#include <algorithm>

using namespace TIMEFREQUENCYLIB;

std::mutex fftw_mutex;


int nextpow2(int x)
{
	if (x < 0)
		return 0;
	--x;
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;
	return x + 1;
}


__forceinline std::complex<float> conj(const std::complex<float>& z)
{
	return std::complex<float>(z.real(), -z.imag());
}




void convolver::initialize(int signal_size, int kernel_size)
{
	_signal_size	= signal_size;
	_kernel_size	= kernel_size;
	_method			= method::fft_convolution;
	_shape			= shape::same;
	_operation		= operation::convolution;
	_convol_size	= _signal_size + _kernel_size - 1;


	switch (_method)
	{
	case method::fft_convolution:
		_segment_size	= 0;
		_segment_count	= 0;
		_fft_size		= nextpow2(_convol_size);

		alloc_fft_buffers(_fft_size);
		break;

	default:
		exit(-1);
	}

}

void convolver::clear()
{
	_signal_size	= 0;
	_kernel_size	= 0;
	_convol_size	= 0;
	_fft_size		= 0;
	_segment_count	= 0;
	_segment_size 	= 0;
	_shape			= shape::same;
	_operation		= operation::convolution;
	_method			= method::automatic;

	fftw_mutex.lock();
	if (_signal_buffer) fftwf_free(_signal_buffer);
	if (_fft_fwd)		fftwf_destroy_plan(_fft_fwd);
	if (_fft_rev)		fftwf_destroy_plan(_fft_rev);
	fftw_mutex.unlock();

	_signal_buffer	= nullptr;
	_kernel_buffer	= nullptr;
	_output_buffer	= nullptr;
	_fft_fwd		= nullptr;
	_fft_rev		= nullptr;
}



void convolver::assign_kernel(std::complex<float>* kernel, int size)
{
	int n = std::min(size, _kernel_size);
	if (_method == method::fft_convolution ||
		_method == method::fft_overlap_add)
	{
		for (int i = 0; i < n; ++i)
			_kernel_buffer[i] = kernel[i];

		prepare_fft_kernel();
	}
}




void convolver::prepare_fft_kernel()
{
	for (int i = _kernel_size; i < _fft_size; ++i)
		_kernel_buffer[i] = std::complex<float>();

	fftwf_execute_dft(_fft_fwd, _kernel_buffer, _kernel_buffer);

	// prepare kernel
	float norm = float(1.0 / _fft_size);
	if (_operation == operation::correlation)
	{
		for (int i = 0; i < _fft_size; ++i)
			_kernel_buffer[i] = conj(_kernel_buffer[i]);
	}
	else
	{
		for (int i = 0; i < _fft_size; ++i)
			_kernel_buffer[i] *= norm;
	}
}



void convolver::alloc_fft_buffers(int fft_size)
{
	fftw_mutex.lock();
	_signal_buffer = reinterpret_cast<std::complex<float>*>(fftwf_malloc(size_t(_fft_size) * 3 * sizeof(std::complex<float>)));
	_kernel_buffer = _signal_buffer + _fft_size;
	_output_buffer = _kernel_buffer + _fft_size;
	_fft_fwd		= fftwf_plan_dft_1d(_fft_size, nullptr, nullptr, FFTW_FORWARD, FFTW_ESTIMATE);
	_fft_rev		= fftwf_plan_dft_1d(_fft_size, nullptr, nullptr, FFTW_BACKWARD, FFTW_ESTIMATE);
	fftw_mutex.unlock();

	if (!_signal_buffer || !_fft_fwd || !_fft_rev)
		exit(-1);

	memset(_signal_buffer, 0, size_t(_fft_size) * 3 * sizeof(std::complex<float>));
}




void convolver::apply_fft_kernel()
{
	fftwf_execute_dft(_fft_fwd, _signal_buffer, _output_buffer);

	for (int i = 0; i < _fft_size; ++i)
		_output_buffer[i] = _output_buffer[i] * _kernel_buffer[i];

	fftwf_execute_dft(_fft_rev, _output_buffer, _output_buffer);
}




void convolver::conv_r2c_same(float* signal, std::complex<float>* out)
{
	if (_method == method::fft_convolution)
	{
		for (int i = 0; i < _signal_size; ++i)			_signal_buffer[i] = std::complex<float>(signal[i]);
		for (int i = _signal_size; i < _fft_size; ++i)	_signal_buffer[i] = std::complex<float>();

		apply_fft_kernel();

		auto opb = _output_buffer + _kernel_size / 2;
		for (int i = 0; i < _signal_size; ++i)
			out[i] = opb[i];
	}
	else if (_method == method::fft_overlap_add)
	{
		for (int i = 0; i < _signal_size; ++i)
			out[i] = std::complex<float>();

		int r_begin = 0;
		while (r_begin < _signal_size)
		{
			int r_count = std::min(r_begin + _segment_size, _signal_size) - r_begin;
			
			for (int i = 0; i < r_count; ++i)				_signal_buffer[i] = std::complex<float>(signal[r_begin + i]);
			for (int i = r_count; i < _segment_size; ++i)	_signal_buffer[i] = std::complex<float>();
			
			apply_fft_kernel();
			
			int w_begin		= std::max(r_begin - _kernel_size / 2, 0);
			int b_offset	= std::max(_kernel_size / 2 - r_begin, 0);
			int w_count		= std::min(_fft_size - b_offset, _signal_size - w_begin - b_offset);

			auto cxbuf = _output_buffer + b_offset;
			for (int i = 0; i < w_count; ++i)
				out[w_begin + i] += cxbuf[i];

			r_begin += _segment_size;
		}
	}
}



