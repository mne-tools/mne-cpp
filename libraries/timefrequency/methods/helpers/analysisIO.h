#ifndef ANALYSISSETTINGS_H
#define ANALYSISSETTINGS_H

#include <Eigen/Core>
#include <complex>

namespace TIMEFREQUENCYLIB {

struct AnalysisSettings
{
    float	sampling_rate;
    float	freq_low;
    float	freq_high;
    int		input_size;
};

struct TimeFrequencyInput
{
    TimeFrequencyInput(Eigen::MatrixXd);
};

struct TimeFrequencyResult
{
    template <typename T>
    std::complex<T> getArray();

    Eigen::MatrixXcd getEigenMatrix();
};
}//namepace
#endif // ANALYSISSETTINGS_H
