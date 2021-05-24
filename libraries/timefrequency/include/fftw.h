#pragma once
#include <complex>

#pragma comment(lib, "libfftw3f-3.lib")

// FFTW MINIMAL INTERFACE
extern "C"
{
	extern void*	fftwf_malloc(unsigned long long size);
	extern void		fftwf_free(void* p);
	extern void*	fftwf_plan_dft_1d(int size, std::complex<float>* in, std::complex<float>* out, int sign, unsigned flags);
	extern void*	fftwf_plan_dft_r2c_1d(int size, float* in, std::complex<float>* out, unsigned flags);
	extern void*	fftwf_plan_dft_c2r_1d(int size, std::complex<float>* in, float* out, unsigned flags);
	extern void*	fftwf_plan_dft_2d(int n0, int n1, std::complex<float>* in, std::complex<float>* out, int sign, unsigned flags);

	extern void		fftwf_execute(void* p);
	extern void		fftwf_execute_dft(void* p, std::complex<float>* in, std::complex<float>* out);
	extern void		fftwf_execute_dft_r2c(void* p, float* in, std::complex<float>* out);
	extern void		fftwf_execute_dft_c2r(void* p, std::complex<float>* in, float* out);
	extern void		fftwf_destroy_plan(void* p);


#define FFTW_FORWARD (-1)
#define FFTW_BACKWARD (+1)
#define FFTW_ESTIMATE (1U << 6)
}

