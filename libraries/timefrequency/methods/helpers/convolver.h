#pragma once
#include <complex>

namespace TIMEFREQUENCYLIB {

class convolver
{
public:
	enum class method
	{
		automatic,
		time_domain,
		fft_overlap_add,
		fft_convolution
	};

	enum class shape
	{
		same,
		full
	};

	enum class operation
	{
		convolution,
		correlation
	};


public:
	convolver(int signal_size, int kernel_size)
	{
		initialize(signal_size, kernel_size);
	}

	~convolver()
	{
		clear();
	}

	void initialize(int signal_size, int kernel_size);
	void clear();

	void conv_r2c_same(float* signal, std::complex<float>* output);
	void assign_kernel(std::complex<float>* kernel, int size);

protected:
	void prepare_fft_kernel();
	void alloc_fft_buffers(int fft_size);
	void apply_fft_kernel();


protected:
	int					_signal_size	= 0;
	int					_kernel_size	= 0;
	int					_convol_size	= 0;
	int					_fft_size		= 0;
	int					_segment_size	= 0;
	int					_segment_count	= 0;
	shape				_shape;
	method				_method;
	operation			_operation;

private:
	void				*_fft_fwd		= nullptr;
	void				*_fft_rev		= nullptr;
	std::complex<float>	*_signal_buffer	= nullptr;
	std::complex<float>	*_output_buffer	= nullptr;
	std::complex<float>	*_kernel_buffer	= nullptr;

private:
	static const int	default_segment_size = 512;
};

}
