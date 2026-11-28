//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     spectrum_figure.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    SpectrumFigure class definition.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "spectrum_figure.h"
#include "canvas.h"
#include "text_metrics.h"
#include <mne/base/string_utils.h>
#include <mne/fiff/fiff_info.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <cmath>
#include <limits>
#include <string>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::viz;

//=============================================================================================================
// DEFINE STATIC METHODS
//=============================================================================================================

namespace
{

// Gap between the pointer and the read-out text
constexpr double kReadoutGap = 8.0;
// Lines of the frequency grid with a linear scale
constexpr int kLinearLines = 5;
// Significant digits of the grid labels and the read-out
constexpr int kDigits = 6;

std::string number(double value)
{
    return mne::format_number(value, 'g', kDigits);
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

void SpectrumFigure::set_info(std::shared_ptr<FiffInfo> info, SpectrumTable::Scale scale)
{
    // A new table, so the frequency axis follows the new scale with the next data
    m_scale = scale;
    m_table = SpectrumTable();
    m_table.set_scale(scale);
    m_table.set_info(std::move(info));
    changed(Change::Bounds);
}

//=============================================================================================================

void SpectrumFigure::set_data(const Eigen::MatrixXd& data)
{
    m_table.set_data(data);
    changed();
}

//=============================================================================================================

void SpectrumFigure::set_bounds(float lower, float upper)
{
    if (m_table.freq_scale().size() == 0) {
        return;
    }
    m_table.set_bounds(lower, upper);
    changed();
}

//=============================================================================================================

void SpectrumFigure::select(const std::vector<int>& channels)
{
    m_table.select(channels);
    changed(Change::Bounds);
}

//=============================================================================================================

void SpectrumFigure::reset_selection()
{
    m_table.reset_selection();
    changed(Change::Bounds);
}

//=============================================================================================================

void SpectrumFigure::toggle_frozen()
{
    m_table.toggle_frozen();
    changed();
}

//=============================================================================================================

std::optional<SpectrumFigure::Readout> SpectrumFigure::readout() const
{
    const double plotWidth = m_width - name_width;
    if (!m_pointer || plotWidth <= 0.0 || m_pointer->x < name_width || m_pointer->x > m_width || m_pointer->y < 0.0) {
        return std::nullopt;
    }
    Readout readout;
    readout.row = static_cast<int>(m_pointer->y / row_height);
    if (readout.row >= m_table.row_count()) {
        return std::nullopt;
    }
    const std::optional<Eigen::RowVectorXd> spectrum = m_table.spectrum(readout.row);
    const Eigen::RowVectorXd& bounded = m_table.freq_scale_bound();
    const int lower = m_table.lower_bin();
    const int upper = m_table.upper_bin();
    if (!spectrum || spectrum->size() == 0 || bounded.size() <= upper || upper < lower) {
        return std::nullopt;
    }
    // The bin nearest to the pointer between the bounds
    const double rate = (m_pointer->x - name_width) / plotWidth;
    double best = std::numeric_limits<double>::max();
    for (int bin = lower; bin <= upper && bin < spectrum->size(); ++bin) {
        if (const double distance = std::abs(bounded[bin] - rate); distance < best) {
            best = distance;
            readout.bin = bin;
        }
    }
    if (readout.bin < 0) {
        return std::nullopt;
    }
    readout.value = (*spectrum)[readout.bin];
    readout.frequency = bin_frequency(readout.bin);
    return readout;
}

//=============================================================================================================

Rect SpectrumFigure::bounds() const
{
    return {0.0, 0.0, m_width, m_table.row_count() * row_height};
}

//=============================================================================================================

void SpectrumFigure::draw(Canvas& canvas, const TextMetrics& metrics, const Font& font) const
{
    canvas.set_font(font);
    // Only the rows in the canvas, as long as the host only scrolls and scales
    int first = 0;
    int last = m_table.row_count() - 1;
    const Transform& transform = canvas.transform();
    if (transform.m12 == 0.0 && transform.m21 == 0.0 && transform.m22 > 0.0) {
        first = std::max(first, static_cast<int>(std::floor(-transform.dy / transform.m22 / row_height)));
        last = std::min(last, static_cast<int>(std::floor((canvas.height() - transform.dy) / transform.m22 / row_height)));
    }
    for (int row = first; row <= last; ++row) {
        draw_row(canvas, metrics, font, row);
    }
}

//=============================================================================================================

bool SpectrumFigure::zoomable() const
{
    return false;
}

//=============================================================================================================

void SpectrumFigure::resize(double width, double /*height*/)
{
    if (width != m_width) {
        m_width = width;
        changed(Change::Bounds);
    }
}

//=============================================================================================================

void SpectrumFigure::hover(std::optional<Point> point)
{
    m_pointer = point;
    changed();
}

//=============================================================================================================

bool SpectrumFigure::double_click(Point /*point*/)
{
    toggle_frozen();
    return true;
}

//=============================================================================================================

std::vector<std::pair<int, double>> SpectrumFigure::grid() const
{
    std::vector<std::pair<int, double>> lines;
    const int bins = m_table.bins();
    if (!m_table.info() || bins == 0) {
        return lines;
    }
    const double nyquist = m_table.info()->sfreq / 2.0;
    const double resolution = nyquist / bins;
    const auto bin_of = [resolution](double frequency) {
        return static_cast<int>(std::floor(frequency / resolution));
    };
    lines.emplace_back(0, 0.0);
    if (m_scale == SpectrumTable::Scale::Log) {
        // Decades: 1, 10, 100 Hz, ...
        const auto decades = static_cast<int>(std::ceil(std::log10(nyquist)));
        for (int decade = 0; decade < decades; ++decade) {
            const double frequency = std::pow(10.0, decade);
            lines.emplace_back(bin_of(frequency), frequency);
        }
    } else {
        for (int line = 1; line < kLinearLines; ++line) {
            const int bin = bin_of(line * nyquist / kLinearLines);
            lines.emplace_back(bin, bin_frequency(bin));
        }
    }
    return lines;
}

//=============================================================================================================

double SpectrumFigure::bin_frequency(int bin) const
{
    const Eigen::RowVectorXd& axis = m_table.freq_scale();
    if (!m_table.info() || bin < 0 || bin >= axis.size()) {
        return 0.0;
    }
    // Where the axis places the bin: its last bin is at the Nyquist frequency
    const double nyquist = m_table.info()->sfreq / 2.0;
    if (m_scale == SpectrumTable::Scale::Log) {
        return std::pow(10.0, axis[bin] * std::log10(nyquist + 1.0)) - 1.0;
    }
    return axis[bin] * nyquist;
}

//=============================================================================================================

void SpectrumFigure::draw_row(Canvas& canvas, const TextMetrics& metrics, const Font& font, int row) const
{
    const double top = row * row_height;
    const double plotWidth = m_width - name_width;
    const Color text{0, 0, 0};

    canvas.set_pen(text);
    canvas.draw_text({4.0, top, name_width - 8.0, row_height}, Align::Left | Align::VCenter, m_table.channel_name(row));
    canvas.set_pen(Color{220, 220, 220});
    canvas.draw_line({0.0, top + row_height - 0.5}, {m_width, top + row_height - 0.5});

    const std::optional<Eigen::RowVectorXd> spectrum = m_table.spectrum(row);
    const Eigen::RowVectorXd& bounded = m_table.freq_scale_bound();
    const int lower = m_table.lower_bin();
    const int upper = std::min(m_table.upper_bin(), static_cast<int>(bounded.size()) - 1);
    if (!spectrum || spectrum->size() <= upper || upper <= lower || plotWidth <= 0.0) {
        return;
    }

    // Frequency grid with labels at the top
    Pen grid{text, 0.5, LineStyle::Dot};
    const double ascent = metrics.ascent(font);
    for (const auto& [bin, frequency] : this->grid()) {
        if (bin <= lower || bin >= upper) {
            continue;
        }
        const double x = name_width + bounded[bin] * plotWidth;
        canvas.set_pen(grid);
        canvas.draw_line({x, top}, {x, top + row_height});
        canvas.set_pen(text);
        canvas.draw_text({x + 2.0, top + ascent}, number(frequency) + "Hz");
    }

    // The spectrum between the bounds, scaled to its range with a pixel to spare at both edges
    const Eigen::RowVectorXd visible = spectrum->segment(lower, upper - lower + 1);
    const double minimum = visible.minCoeff();
    const double maximum = visible.maxCoeff();
    const double scale = (row_height - 2.0) / (maximum > minimum ? maximum - minimum : 1.0);
    const double bottom = top + row_height - 1.0;
    std::vector<Point> points;
    points.reserve(static_cast<std::size_t>(upper - lower) + 1);
    for (int bin = lower; bin <= upper; ++bin) {
        points.push_back({name_width + bounded[bin] * plotWidth, bottom - ((*spectrum)[bin] - minimum) * scale});
    }
    const std::optional<Readout> current = readout();
    const bool hovered = current && current->row == row;
    Color curve = m_table.frozen() ? Color{128, 128, 128} : Color{0, 0, 128};
    if (hovered) {
        curve = m_table.frozen() ? Color{128, 0, 0} : Color{255, 0, 0};
    }
    canvas.save();
    canvas.set_antialiasing(true);
    canvas.set_pen(Pen{curve, 1.0});
    canvas.draw_polyline(points);
    canvas.restore();

    if (!hovered) {
        return;
    }
    // Read-out: a line at the pointer and the bin's value and frequency beside it, inside the row
    const Point at = *m_pointer;
    canvas.set_pen(Pen{Color{128, 128, 128}, 1.0, LineStyle::Dash});
    canvas.draw_line({at.x, top}, {at.x, top + row_height});
    const std::string label = number(current->value) + " [dB], " + number(current->frequency) + " [Hz]";
    const double width = metrics.advance(font, label);
    const double x = at.x + kReadoutGap + width > m_width ? at.x - kReadoutGap - width : at.x + kReadoutGap;
    const double y = std::clamp(at.y - kReadoutGap, top + ascent, top + row_height - 2.0);
    canvas.set_pen(text);
    canvas.draw_text({x, y}, label);
}
