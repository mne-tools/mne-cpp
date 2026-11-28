//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     filter_response_figure.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    FilterResponseFigure class definition.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "filter_response_figure.h"
#include "canvas.h"
#include <mne/dsp/filterkernel.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <complex>
#include <utility>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::viz;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

void FilterResponseFigure::set_filter(const mne::FilterKernel& filter)
{
    const Eigen::RowVectorXcd& spectrum = filter.fft_coefficients();
    if (filter.coefficients().cols() == 0 || spectrum.cols() == 0) {
        return;
    }
    std::vector<double> magnitude;
    magnitude.reserve(static_cast<std::size_t>(spectrum.cols()));
    for (Eigen::Index i = 0; i < spectrum.cols(); ++i) {
        magnitude.push_back(std::abs(spectrum(i)));
    }
    const double nyquist = filter.sampling_frequency() / 2.0;
    std::vector<double> cutoffs;
    cutoffs.reserve(2);
    for (double hz : {filter.highpass_freq(), filter.lowpass_freq()}) {
        if (hz > 0.0 && hz < nyquist) {
            cutoffs.push_back(hz);
        }
    }
    m_plot.set_response(std::move(magnitude), nyquist);
    m_plot.set_cutoffs(std::move(cutoffs));
    m_plot.set_title(filter.name());
    changed();
}

//=============================================================================================================

void FilterResponseFigure::set_colors(Color foreground, Color cutoff)
{
    m_plot.set_colors(foreground, cutoff);
    changed();
}

//=============================================================================================================

DisplayList FilterResponseFigure::picture(double width, double height, const TextMetrics& metrics, const Font& font) const
{
    return m_plot.render(width, height, metrics, font);
}

//=============================================================================================================

Rect FilterResponseFigure::bounds() const
{
    return {0.0, 0.0, m_width, m_height};
}

//=============================================================================================================

void FilterResponseFigure::draw(Canvas& canvas, const TextMetrics& metrics, const Font& font) const
{
    m_plot.draw(canvas, metrics, font);
}

//=============================================================================================================

bool FilterResponseFigure::zoomable() const
{
    return false;
}

//=============================================================================================================

void FilterResponseFigure::resize(double width, double height)
{
    if (width != m_width || height != m_height) {
        m_width = width;
        m_height = height;
        changed(Change::Bounds);
    }
}
