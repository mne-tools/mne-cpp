//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *
 * @file     tfplot.h
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @author   Juan GPC <jgarciaprieto@mgh.harvard.edu>
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     July 2018
 * @brief    Time-frequency spectrogram widget rendering an Eigen matrix as a colour image.
 *
 * TFplot accepts a spectrogram matrix (rows = frequency bins, columns
 * = time samples) together with a sample rate and a colour-map choice
 * (@c Hot, @c Jet, @c Bone, @c RedBlue and signed variants) and paints
 * it into a @c QWidget with frequency-band cropping support. It is
 * used by the TF-MNE viewer to inspect Wavelet / STFT spectrograms in
 * stand-alone windows without depending on the heavier 3-D pipeline.
 */

#ifndef TFPLOT_H
#define TFPLOT_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp_global.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QWidget>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/SparseCore>
#include <unsupported/Eigen/FFT>

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

enum ColorMaps {
    Hot,
    HotNeg1,
    HotNeg2,
    Jet,
    Bone,
    RedBlue
};

//=============================================================================================================
/**
 * @brief Time-frequency spectrogram QWidget rendering an Eigen matrix as a colour image.
 *
 * Two constructors accept either a full or a frequency-cropped
 * @c MatrixXd spectrogram together with a sample rate and a
 * @ref ColorMaps enum entry; @c calc_plot() builds the cached
 * @c QImage that subsequent @c paintEvents draw.
 */
class DISPSHARED_EXPORT TFplot : public QWidget
{
    Q_OBJECT

public:
    typedef QSharedPointer<TFplot> SPtr;            /**< Shared pointer type for TFplot class. */
    typedef QSharedPointer<const TFplot> ConstSPtr; /**< Const shared pointer type for TFplot class. */

    //=========================================================================================================
    /**
     * Constructs TFplot class
     *
     * @param[in] tf_matrix         given spectrogram.
     * @param[in] sample_rate       given sample rate of signal related to th spectrogram.
     * @param[in] lower_frq         lower bound frequency, that should be plotted.
     * @param[in] upper_frq         upper bound frequency, that should be plotted.
     * @param[in] cmap              colormap used to plot the spectrogram.
     *
     */
    TFplot(Eigen::MatrixXd tf_matrix,
           qreal sample_rate,
           qreal lower_frq,
           qreal upper_frq,
           ColorMaps cmap);

    //=========================================================================================================
    /**
     * Constructs TFplot class
     *
     * @param[in] tf_matrix         given spectrogram.
     * @param[in] sample_rate       given sample rate of signal related to th spectrogram.
     * @param[in] cmap              colormap used to plot the spectrogram.
     *
     */
    TFplot(Eigen::MatrixXd tf_matrix,
           qreal sample_rate,
           ColorMaps cmap);

protected:
    //=========================================================================================================
    /**
     * Calculates a image to plot the tf_matrix
     *
     * @param[in] tf_matrix         given spectrogram.
     * @param[in] sample_rate       given sample rate of signal related to th spectrogram.
     * @param[in] cmap              colormap used to plot the spectrogram.
     * @param[in] lower_frq         lower bound frequency, that should be plotted.
     * @param[in] upper_frq         upper bound frequency, that should be plotted.
     *
     */
    void calc_plot(Eigen::MatrixXd tf_matrix,
                   qreal sample_rate,
                   ColorMaps cmap,
                   qreal lower_frq,
                   qreal upper_frq);

    virtual void resizeEvent(QResizeEvent *event);
};
} // NAMESPACE

#endif // TFPLOT_H
