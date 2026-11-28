//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     spectrum_panel.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    SpectrumPanel class definition.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "spectrum_panel.h"
#include <mne/fiff/fiff_info.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cmath>
#include <utility>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::viz;

//=============================================================================================================
// DEFINE STATIC METHODS
//=============================================================================================================

namespace
{

constexpr const char* kLower = "lower";
constexpr const char* kUpper = "upper";

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

SpectrumPanel::SpectrumPanel()
: Panel("SpectrumView", "Frequency Spectrum")
, m_spectrum(std::make_shared<SpectrumFigure>())
{
    set_figure(m_spectrum);
    for (const auto& [key, label] : {std::pair{kLower, "Lower Frequency"}, std::pair{kUpper, "Upper Frequency"}}) {
        SettingField bound;
        bound.key = key;
        bound.kind = SettingField::Kind::Double;
        bound.label = label;
        bound.default_value = 0.0;
        // Until init() knows the Nyquist frequency, bounds set before are kept
        bound.maximum = 1e6;
        bound.step = 0.001;
        bound.decimals = 3;
        bound.unit = " Hz";
        bound.style = SettingField::Style::Slider;
        bound.persistent = false;
        form().add(std::move(bound));
    }
    m_listener = form().add_listener([this](const std::string&, const std::optional<SettingValue>& value) {
        if (m_updating || !value) {
            return;
        }
        // The lower bound stays below the upper one
        if (form().get<double>(kLower) > form().get<double>(kUpper)) {
            form().set_value(kLower, form().get<double>(kUpper));
            return;
        }
        apply_bounds();
        if (m_boundsCallback) {
            m_boundsCallback(lower_bound(), upper_bound());
        }
    });
}

//=============================================================================================================

SpectrumPanel::~SpectrumPanel()
{
    form().remove_listener(m_listener);
}

//=============================================================================================================

void SpectrumPanel::init(std::shared_ptr<FiffInfo> info, SpectrumTable::Scale scale)
{
    const double nyquist = info ? std::floor(info->sfreq / 2.0) : 0.0;
    m_spectrum->set_info(std::move(info), scale);
    m_boundsApplied = false;
    const bool updating = std::exchange(m_updating, true);
    form().set_range(kLower, 0.0, nyquist);
    form().set_range(kUpper, 0.0, nyquist);
    // Without bounds yet the whole spectrum shows
    if (form().get<double>(kUpper) <= 0.0) {
        form().set_value(kUpper, nyquist);
    }
    m_updating = updating;
}

//=============================================================================================================

void SpectrumPanel::add_data(const Eigen::MatrixXd& data)
{
    if (!m_spectrum->table().info()) {
        return;
    }
    m_spectrum->set_data(data);
    if (!m_boundsApplied) {
        apply_bounds();
    }
}

//=============================================================================================================

void SpectrumPanel::set_boundaries(float lower, float upper)
{
    const bool updating = std::exchange(m_updating, true);
    form().set_value(kUpper, static_cast<double>(upper));
    form().set_value(kLower, static_cast<double>(lower));
    m_updating = updating;
    apply_bounds();
}

//=============================================================================================================

float SpectrumPanel::lower_bound() const
{
    return static_cast<float>(form().get<double>(kLower));
}

//=============================================================================================================

float SpectrumPanel::upper_bound() const
{
    return static_cast<float>(form().get<double>(kUpper));
}

//=============================================================================================================

void SpectrumPanel::apply_bounds()
{
    if (m_spectrum->table().freq_scale().size() == 0) {
        return;
    }
    m_spectrum->set_bounds(lower_bound(), upper_bound());
    m_boundsApplied = true;
}
