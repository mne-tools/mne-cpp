//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2015-2026 MNE-CPP Authors
 *
 * @file     spectrogram.cpp
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Martin Henfling <martin.henfling@tu-ilmenau.de>;
 *           Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     September, 2015
 * @brief    Definition of the Spectrogram class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "spectrogram.h"

#include <mne/base/thread_pool.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <unsupported/Eigen/FFT>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <cmath>
#include <numbers>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MatrixXd Spectrogram::make_spectrogram(VectorXd signal, int windowSize)
{
    const Index n = signal.size();
    MatrixXd tf(n / 2, n);
    if (n == 0) {
        return tf;
    }
    signal.array() -= signal.mean();
    const Index defaultWindow = std::max<Index>(1, n / 15); // whole samples
    const double scale = static_cast<double>(windowSize > 0 ? windowSize : defaultWindow);
    const double norm = std::pow(2.0, 0.25) / std::sqrt(scale);
    const ArrayXd samples = ArrayXd::LinSpaced(n, 0.0, static_cast<double>(n - 1));

#ifdef EIGEN_FFTW_DEFAULT
    fftw_make_planner_thread_safe();
#endif
    ThreadPool& pool = ThreadPool::global();
    const Index chunks = std::min<Index>(n, 4 * static_cast<Index>(pool.thread_count()) + 1);
    pool.parallel_for(0, static_cast<std::size_t>(chunks), [&](std::size_t chunk) {
        Eigen::FFT<double> fft;
        VectorXd windowed(n);
        VectorXcd spectrum(n);
        const Index first = n * static_cast<Index>(chunk) / chunks;
        const Index last = n * (static_cast<Index>(chunk) + 1) / chunks;
        for (Index j = first; j < last; ++j) {
            windowed = signal.array() * (((samples - static_cast<double>(j)) / scale).square() * -std::numbers::pi).exp() * norm;
            fft.fwd(spectrum, windowed);
            tf.col(j) = spectrum.head(n / 2).cwiseAbs2();
        }
    });
    return tf;
}
