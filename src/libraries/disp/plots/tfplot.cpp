//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *
 * @file     tfplot.cpp
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     July 2018
 * @brief    Implementation of TFplot: crops a spectrogram to a frequency band and labels its axes.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "tfplot.h"

#include <algorithm>
#include <cmath>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

namespace
{
QString colorMapName(ColorMaps cmap)
{
    switch (cmap) {
        case Hot:
            return QStringLiteral("Hot");
        case HotNeg1:
            return QStringLiteral("HotNegative1");
        case HotNeg2:
            return QStringLiteral("HotNegative2");
        case Bone:
            return QStringLiteral("Bone");
        case RedBlue:
            return QStringLiteral("RedBlue");
        case Jet:
            break;
    }
    return QStringLiteral("Jet");
}
} // namespace

//=============================================================================================================

TFplot::TFplot(const MatrixXd& tf_matrix,
               qreal sample_rate,
               qreal lower_frq,
               qreal upper_frq,
               ColorMaps cmap,
               qreal tmin)
{
    if (tf_matrix.rows() == 0 || tf_matrix.cols() == 0 || sample_rate <= 0) {
        return;
    }

    // Row k is bin k of an FFT over all N columns
    const qreal binWidth = sample_rate / tf_matrix.cols();
    const Index last = tf_matrix.rows() - 1;
    const Index first = std::clamp<Index>(static_cast<Index>(std::ceil(std::min(lower_frq, upper_frq) / binWidth)), 0, last);
    const Index top = std::clamp<Index>(static_cast<Index>(std::floor(std::max(lower_frq, upper_frq) / binWidth)), first, last);

    // Highest frequency in the first row, drawn at the top
    MatrixXd power = tf_matrix.middleRows(first, top - first + 1).colwise().reverse().cwiseAbs();
    const double maxPower = power.maxCoeff();
    if (maxPower > 0.0) {
        power /= maxPower;
    }

    setColorMap(colorMapName(cmap));
    setColorRange(0.0, 1.0);
    updateData(power);
    setExtent(tmin, tmin + (tf_matrix.cols() - 1) / sample_rate, first * binWidth, top * binWidth);
    setXLabel(QStringLiteral("Time [s]"));
    setYLabel(QStringLiteral("Frequency [Hz]"));
    setColorbarLabel(QStringLiteral("Power (normalized)"));
}

//=============================================================================================================

TFplot::TFplot(const MatrixXd& tf_matrix,
               qreal sample_rate,
               ColorMaps cmap,
               qreal tmin)
: TFplot(tf_matrix, sample_rate, 0.0, sample_rate / 2.0, cmap, tmin)
{
}
