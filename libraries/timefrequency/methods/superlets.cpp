#include "superlets.h"

#include <cmath>

using namespace TIMEFREQUENCYLIB;

Superlets::Superlets(SuperletSettings& settings)
 : m_settings(settings)
{

}

bool Superlets::validateSettings()
{

}

bool Superlets::compute(TimeFrequencyInput input, TimeFrequencyResult output)
{
//    size_t input_size = m_settings.input_size;

//    for (size_t i_freq = 0; i_freq < m_superlets.size(); ++i_freq)
//    {
//        // initialize the pooling buffer
//        std::fill(m_pooling_buffer.begin(), m_pooling_buffer.end(), 1.0);

//        if (m_superlets[i_freq].size() > 1)
//        {
//            // superresolution
//            size_t n_wavelets	= size_t(std::floor(m_orders[i_freq]));
//            double r_factor		= 1.0 / n_wavelets;

//            for (size_t i_wave = 0; i_wave < n_wavelets; ++i_wave)
//            {
//                // perform R2C convolution
//                m_filters[i_freq][i_wave]->conv_r2c_same(input, m_conv_buffer.data());

//                // pool with exponent = 1
//                for (size_t i = 0; i < input_size; ++i)
//                {
//                    m_pooling_buffer[i] *= 2.0 * mag_sqr(m_conv_buffer[i]);
//                }
//            }

//            // determine if fractional superlet should be used
//            if (fractional(m_orders[i_freq]) != 0			&&	// order is fractional
//                m_settings.fractional								&&	// we are allowed to use it
//                m_superlets[i_freq].size() == n_wavelets + 1	)	// we really have a wavelet available for it
//            {
//                double exponent = fractional(m_orders[i_freq]);
//                r_factor		= 1.0 / (n_wavelets + exponent);

//                // perform convolution with the last wavelet
//                m_filters[i_freq].back()->conv_r2c_same(input, m_conv_buffer.data());

//                // pool with fractional exponent
//                for (size_t i = 0; i < input_size; ++i)
//                {
//                    m_pooling_buffer[i] *= std::pow(2.0 * mag_sqr(m_conv_buffer[i]), exponent);
//                }
//            }

//            // perform geometric mean and save to output buffer
//            for (size_t i = 0; i < input_size; ++i)
//            {
//                output[i] += float(std::pow(m_pooling_buffer[i], r_factor));
//            }
//        }
//        else
//        {
//            // standard CWT
//            m_filters[i_freq].front()->conv_r2c_same(input, m_conv_buffer.data());

//            for (size_t i = 0; i < input_size; ++i)
//                output[i] += float(2.0 * mag_sqr(m_conv_buffer[i]));
//        }

//        output += input_size;
//    }

    return false;
}

void Superlets::initFromSettings()
{
    m_filters.resize(m_settings.freq_count);
    m_superlets.resize(m_settings.freq_count);
    m_conv_buffer.resize(m_settings.input_size);
    m_pooling_buffer.resize(m_settings.input_size);

    m_frequencies = linspace(m_settings.freq_low, m_settings.freq_high, m_settings.freq_count);
    m_orders = linspace(m_settings.resolution_low, m_settings.resolution_high, m_settings.freq_count);

    if (!m_settings.fractional)
        std::transform(m_orders.begin(), m_orders.end(), m_orders.begin(), std::roundf);

    for (size_t i_freq = 0; i_freq < m_settings.freq_count; ++i_freq)
    {
        float center_freq	= m_frequencies[i_freq];
        int n_wavelets		= int(std::ceil(m_orders[i_freq]));

        for (size_t i_wave = 0; i_wave < n_wavelets; ++i_wave)
        {
            float ncyc = m_settings.multiplicative
                ? (i_wave + 1) * m_settings.wavelet_cycles
                : m_settings.wavelet_cycles + i_wave;

            m_superlets[i_freq]	.emplace_back(center_freq, ncyc, m_settings.sampling_rate);
            m_filters[i_freq]	.push_back(new convolver(m_settings.input_size, m_superlets[i_freq].back().size()));
            m_filters[i_freq]	.back()->assign_kernel(m_superlets[i_freq].back().data(), m_superlets[i_freq].back().size());
        }
    }
}

std::vector<float> Superlets::linspace(float from, float to, int n)
{
    if		(n <= 0) return { };
    else if (n == 1) return { from };
    else
    {
        std::vector<float> result(n);

        float dv = (to - from) / (n - 1);
        for (size_t i = 0; i < result.size(); ++i)
            result[i] = from + i * dv;

        return result;
    }
}

float Superlets::mag_sqr(const std::complex<float>& z)
{
    return z.real() * z.real() + z.imag() * z.imag();
}

float Superlets::fractional(float x)
{
    return x - (int)x;
}
