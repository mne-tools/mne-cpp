#ifndef SUPERLETS_H
#define SUPERLETS_H

#include "analysismethod.h"

#include "helpers/analysisIO.h"
#include "helpers/morlet.h"
#include "helpers/convolver.h"

#include <vector>

namespace TIMEFREQUENCYLIB {

struct SuperletSettings : public AnalysisSettings{
    int		freq_count;
    float	wavelet_cycles;
    float	resolution_low;
    float	resolution_high;
};

class Superlets : public AnalysisMethod
{
    using filter_bank	= std::vector<convolver*>;
    using superlet		= std::vector<morlet>;
public:
    Superlets() = delete;
    Superlets(SuperletSettings& settings);

    virtual bool compute();
    virtual bool validateSettings();

protected:

    void initFromSettings();

    std::vector<float> linspace(float from, float to, int n);

    float mag_sqr(const std::complex<float>& z);

    float fractional(float x);

    SuperletSettings&                   m_settings;
    std::vector<filter_bank>			m_filters;
    std::vector<superlet>				m_superlets;
    std::vector<std::complex<float>>	m_conv_buffer;
    std::vector<double>					m_pooling_buffer;
    std::vector<float>					m_frequencies;
    std::vector<float>					m_orders;

};
} //namespace

#endif // SUPERLETS_H
