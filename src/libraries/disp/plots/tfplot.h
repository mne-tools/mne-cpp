//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *
 * @file     tfplot.h
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     July 2018
 * @brief    Time-frequency power image of a spectrogram, with time and frequency axes and a colour bar.
 *
 * TFplot accepts a spectrogram as produced by @c DSPLIB::Spectrogram::makeSpectrogram (rows = the
 * first N/2 frequency bins of an N-point FFT, columns = the N time samples) together with the sample
 * rate, an optional frequency band, a colour map and the time of the first sample, and draws it with
 * @c ImageSc: time in seconds, frequency in Hz and power normalized to its maximum in the band.
 */

#ifndef TFPLOT_H
#define TFPLOT_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp_global.h"
#include "imagesc.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
// DEFINE NAMESPACE DISPLIB
//=============================================================================================================

namespace DISPLIB
{

//=============================================================================================================
// DISPLIB FORWARD DECLARATIONS
//=============================================================================================================

enum ColorMaps
{
    Hot,
    HotNeg1,
    HotNeg2,
    Jet,
    Bone,
    RedBlue
};

//=============================================================================================================
/**
 * @brief Time-frequency power image of a spectrogram: time in seconds, frequency in Hz, normalized power.
 *
 * Row k of the spectrogram is the FFT bin at k * sample_rate / N Hz, where N, the number of columns, is
 * the FFT length; column j is the time tmin + j / sample_rate s.
 *
 * @snippet ex_disp_plots/main.cpp tf_plot_usage
 */
class DISPSHARED_EXPORT TFplot : public ImageSc
{
    Q_OBJECT

public:
    typedef QSharedPointer<TFplot> SPtr;            /**< Shared pointer type for TFplot class. */
    typedef QSharedPointer<const TFplot> ConstSPtr; /**< Const shared pointer type for TFplot class. */

    //=========================================================================================================
    /**
     * Plots the frequency bins from @p lower_frq to @p upper_frq.
     *
     * @param[in] tf_matrix         Spectrogram power, N/2 frequency bins x N time samples.
     * @param[in] sample_rate       Sample rate of the signal in Hz.
     * @param[in] lower_frq         Lowest frequency to plot in Hz.
     * @param[in] upper_frq         Highest frequency to plot in Hz.
     * @param[in] cmap              Colour map.
     * @param[in] tmin              Time of the first sample in s, e.g. the start of an epoch.
     */
    TFplot(const Eigen::MatrixXd& tf_matrix,
           qreal sample_rate,
           qreal lower_frq,
           qreal upper_frq,
           ColorMaps cmap,
           qreal tmin = 0.0);

    //=========================================================================================================
    /**
     * Plots all frequency bins.
     *
     * @param[in] tf_matrix         Spectrogram power, N/2 frequency bins x N time samples.
     * @param[in] sample_rate       Sample rate of the signal in Hz.
     * @param[in] cmap              Colour map.
     * @param[in] tmin              Time of the first sample in s, e.g. the start of an epoch.
     */
    TFplot(const Eigen::MatrixXd& tf_matrix,
           qreal sample_rate,
           ColorMaps cmap,
           qreal tmin = 0.0);
};
} // NAMESPACE

#endif // TFPLOT_H
