//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     filterplot.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Definition of the FilterPlot class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "filterplot.h"
#include "helpers/scenepainter.h"

#include <mne/dsp/filterkernel.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QPainter>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <complex>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace mne::disp;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FilterPlot::FilterPlot(QWidget* parent)
: QWidget(parent)
{
}

//=============================================================================================================

void FilterPlot::setFilter(const FilterKernel& filter)
{
    const Eigen::RowVectorXcd& spectrum = filter.fft_coefficients();
    if (filter.coefficients().cols() == 0 || spectrum.cols() == 0) {
        return;
    }
    std::vector<double> magnitude(static_cast<std::size_t>(spectrum.cols()));
    for (Eigen::Index i = 0; i < spectrum.cols(); ++i) {
        magnitude[static_cast<std::size_t>(i)] = std::abs(spectrum(i));
    }
    const double nyquist = filter.sampling_frequency() / 2.0;
    std::vector<double> cutoffs;
    for (double hz : {filter.highpass_freq(), filter.lowpass_freq()}) {
        if (hz > 0.0 && hz < nyquist) {
            cutoffs.push_back(hz);
        }
    }
    m_plot.set_response(std::move(magnitude), nyquist);
    m_plot.set_cutoffs(std::move(cutoffs));
    m_plot.set_title(filter.name());
    update();
}

//=============================================================================================================

scene::DisplayList FilterPlot::figure(const QSize& size) const
{
    scene::FilterResponsePlot printable = m_plot;
    printable.set_colors(scene::Color::black());
    return printable.render(size.width(), size.height(), QtTextMetrics(font()), toSceneFont(font()));
}

//=============================================================================================================

void FilterPlot::paintEvent(QPaintEvent* /*event*/)
{
    // In the palette's text colour, which follows the light or dark style
    scene::FilterResponsePlot onScreen = m_plot;
    onScreen.set_colors(toSceneColor(palette().text().color()));
    QPainter painter(this);
    scene::Canvas canvas(width(), height());
    onScreen.draw(canvas, QtTextMetrics(font()), toSceneFont(font()));
    paintScene(painter, canvas.display_list(), font());
}
