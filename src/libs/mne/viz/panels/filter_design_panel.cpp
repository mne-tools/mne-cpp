//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     filter_design_panel.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    FilterDesignPanel class definition.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "filter_design_panel.h"
#include "../figures/raster_renderer.h"
#include "../figures/svg_renderer.h"
#include "../figures/text_metrics.h"
#include "../host/settings_store.h"
#include <mne/base/log.h>
#include <mne/base/string_utils.h>
#include <mne/dsp/filterio.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <cctype>
#include <format>
#include <utility>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::viz;

//=============================================================================================================
// DEFINE STATIC METHODS
//=============================================================================================================

namespace
{

constexpr const char* kHasFrom = "hasFrom";
constexpr const char* kHasTo = "hasTo";
constexpr const char* kStatus = "status";
constexpr const char* kDesignGroup = "Design";
constexpr const char* kFilesGroup = "Files";
constexpr int kMinTaps = 16;

SettingField frequency(const char* key, const char* label, double value)
{
    SettingField field;
    field.key = key;
    field.kind = SettingField::Kind::Double;
    field.label = label;
    field.default_value = value;
    field.maximum = 300.0;
    field.step = 0.1;
    field.decimals = 1;
    field.unit = " Hz";
    field.commit = SettingField::Commit::WhenFinished;
    field.group = kDesignGroup;
    return field;
}

SettingField file(const char* key, const char* label, const char* filter, bool save)
{
    SettingField field;
    field.key = key;
    field.kind = SettingField::Kind::Path;
    field.label = label;
    field.default_value = std::string();
    field.file_filter = filter;
    field.save_path = save;
    field.persistent = false;
    field.group = kFilesGroup;
    return field;
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FilterDesignPanel::FilterDesignPanel()
: Panel("FilterDesignView", "Filter Design")
, m_response(std::make_shared<FilterResponseFigure>())
{
    set_figure(m_response);
    SettingsForm& f = form();
    f.add_choice(channel_type_key, "Apply filter to", {{"All", "All"}, {"MEG", "MEG"}, {"EEG", "EEG"}, {"ECG", "ECG"}, {"EOG", "EOG"}, {"EMG", "EMG"}}, "All", kDesignGroup);
    f.add_choice(type_key, "Filter type", {{"LPF", "Low pass"}, {"HPF", "High pass"}, {"BPF", "Band pass"}, {"NOTCH", "Notch"}}, "BPF", kDesignGroup);
    f.add_flag(kHasFrom, true);
    f.add_flag(kHasTo, true);
    SettingField fromField = frequency(from_key, "From", 1.0);
    fromField.visible_if = kHasFrom;
    f.add(std::move(fromField));
    SettingField toField = frequency(to_key, "To", 40.0);
    toField.visible_if = kHasTo;
    f.add(std::move(toField));
    SettingField tapsField;
    tapsField.key = taps_key;
    tapsField.kind = SettingField::Kind::Int;
    tapsField.label = "Filter taps";
    tapsField.default_value = 128;
    tapsField.minimum = kMinTaps;
    tapsField.maximum = 4096;
    tapsField.step = 2;
    tapsField.commit = SettingField::Commit::WhenFinished;
    tapsField.group = kDesignGroup;
    f.add(std::move(tapsField));
    f.add(frequency(transition_key, "Transition band", 0.1));
    std::vector<SettingChoice> methods;
    methods.reserve(mne::FilterKernel::design_methods.size());
    for (const mne::FilterParameter& method : mne::FilterKernel::design_methods) {
        methods.push_back({method.name(), method.name()});
    }
    f.add_choice(method_key, "Design method", std::move(methods), "Cosine", kDesignGroup);
    f.add(file(export_coefficients_key, "Export coefficients to", "Text file (*.txt)", true));
    f.add(file(load_filter_key, "Load filter from", "Text file (*.txt)", false));
    f.add(file(export_plot_key, "Export plot to", "Vector graphic (*.svg);;Image (*.png)", true));
    f.add_info(kStatus, "Status", std::string(), kFilesGroup);

    m_listener = f.add_listener([this](const std::string& key, const std::optional<SettingValue>& value) {
        if (!m_updating && value) {
            changed(key);
        }
    });
    design(false);
}

//=============================================================================================================

FilterDesignPanel::~FilterDesignPanel()
{
    form().remove_listener(m_listener);
}

//=============================================================================================================

void FilterDesignPanel::set_sampling_rate(double sfreq)
{
    if (sfreq <= 0.0) {
        mne::Log::warning("[FilterDesignPanel::set_sampling_rate] Sampling frequency is <= 0. Returning.");
        return;
    }
    m_sfreq = sfreq;
    const bool updating = std::exchange(m_updating, true);
    form().set_range(transition_key, 0.0, m_sfreq / 2.0);
    m_updating = updating;
    design(false);
}

//=============================================================================================================

void FilterDesignPanel::set_max_taps(int taps)
{
    if (taps % 2 != 0) {
        --taps;
    }
    const bool updating = std::exchange(m_updating, true);
    form().set_range(taps_key, kMinTaps, std::max(taps, kMinTaps));
    m_updating = updating;
    design(false);
}

//=============================================================================================================

void FilterDesignPanel::set_from(double from)
{
    const bool updating = std::exchange(m_updating, true);
    form().set_value(from_key, from);
    m_updating = updating;
    design(false);
}

//=============================================================================================================

void FilterDesignPanel::set_to(double to)
{
    const bool updating = std::exchange(m_updating, true);
    form().set_value(to_key, to);
    m_updating = updating;
    design(false);
}

//=============================================================================================================

void FilterDesignPanel::set_channel_type(const std::string& type)
{
    const bool updating = std::exchange(m_updating, true);
    form().set_value(channel_type_key, type);
    m_updating = updating;
}

//=============================================================================================================

double FilterDesignPanel::from() const
{
    return form().get<double>(from_key);
}

//=============================================================================================================

double FilterDesignPanel::to() const
{
    return form().get<double>(to_key);
}

//=============================================================================================================

int FilterDesignPanel::taps() const
{
    const int taps = form().get<int>(taps_key);
    return taps % 2 != 0 ? taps - 1 : taps;
}

//=============================================================================================================

std::string FilterDesignPanel::channel_type() const
{
    return form().get<std::string>(channel_type_key);
}

//=============================================================================================================

mne::Expected<void> FilterDesignPanel::load_filter(const std::filesystem::path& path)
{
    const auto loaded = mne::FilterIO::read_filter(path);
    if (!loaded) {
        return std::unexpected(loaded.error());
    }
    const bool updating = std::exchange(m_updating, true);
    SettingsForm& f = form();
    const double nyquist = m_sfreq / 2.0;
    // Wide ranges first, so neither edge is clamped to the other's old value
    f.set_range(from_key, 0.0, nyquist);
    f.set_range(to_key, 0.0, nyquist);
    const std::string& type = loaded->filter_type().name();
    if (type != "UNKNOWN") {
        f.set_value(type_key, type);
    }
    f.set_value(from_key, loaded->highpass_freq());
    f.set_value(to_key, loaded->lowpass_freq());
    f.set_value(taps_key, loaded->filter_order());
    f.set_value(transition_key, loaded->parks_width() * (loaded->sampling_frequency() / 2.0));
    f.set_value(method_key, loaded->design_method().name());
    m_updating = updating;
    update_type_fields();
    design(true);
    return {};
}

//=============================================================================================================

mne::Expected<void> FilterDesignPanel::export_coefficients(const std::filesystem::path& path) const
{
    // A copy named after the design, as the current kernel keeps its name
    mne::FilterKernel named = m_filter;
    named.set_name(std::format("{}_{}_{}_Fs{}", m_filter.filter_type().name(), static_cast<int>(m_filter.highpass_freq()),
                               static_cast<int>(m_filter.lowpass_freq()), static_cast<int>(m_filter.sampling_frequency())));
    return mne::FilterIO::write_filter(path, named);
}

//=============================================================================================================

mne::Expected<void> FilterDesignPanel::export_plot(const std::filesystem::path& path, double width, double height) const
{
    const DisplayList picture = m_response->picture(width, height, ApproximateTextMetrics(), Font{});
    std::string extension = path.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    if (extension == ".png") {
        return to_raster(picture).save_png(path);
    }
    return write_svg(picture, path);
}

//=============================================================================================================

void FilterDesignPanel::load(const SettingsStore& store, const std::string& path)
{
    if (path.empty()) {
        return;
    }
    const bool updating = std::exchange(m_updating, true);
    const double nyquist = m_sfreq / 2.0;
    form().set_range(from_key, 0.0, nyquist);
    form().set_range(to_key, 0.0, nyquist);
    Panel::load(store, path);
    const std::string methodKey = settings_key(path, method_key);
    if (store.contains(methodKey)) {
        const int method = store.read_int(methodKey, 0);
        if (method >= 0 && method < static_cast<int>(mne::FilterKernel::design_methods.size())) {
            form().set_value(method_key, mne::FilterKernel::design_methods[static_cast<std::size_t>(method)].name());
        }
    }
    m_updating = updating;
    update_type_fields();
    design(false);
}

//=============================================================================================================

void FilterDesignPanel::save(SettingsStore& store, const std::string& path) const
{
    if (path.empty()) {
        return;
    }
    Panel::save(store, path);
    store.set_value(settings_key(path, method_key), mne::FilterKernel::design_method_index(form().get<std::string>(method_key)));
}

//=============================================================================================================

void FilterDesignPanel::changed(const std::string& key)
{
    if (key == channel_type_key) {
        if (m_callbacks.channel_type_changed) {
            m_callbacks.channel_type_changed(channel_type());
        }
    } else if (key == type_key) {
        update_type_fields();
        design(true);
    } else if (key == from_key || key == to_key || key == taps_key || key == transition_key || key == method_key) {
        design(true);
    } else if (key == export_coefficients_key || key == load_filter_key || key == export_plot_key) {
        const std::string file = form().get<std::string>(key);
        if (file.empty()) {
            return;
        }
        const std::filesystem::path target = mne::path_from_utf8(file);
        if (key == export_coefficients_key) {
            report_status(export_coefficients(target), "Coefficients written");
        } else if (key == load_filter_key) {
            report_status(load_filter(target), "Filter loaded");
        } else {
            report_status(export_plot(target), "Plot written");
        }
    }
}

//=============================================================================================================

void FilterDesignPanel::design(bool report)
{
    const bool updating = std::exchange(m_updating, true);
    SettingsForm& f = form();
    const double nyquist = m_sfreq / 2.0;
    const int type = mne::FilterKernel::filter_type_index(f.get<std::string>(type_key));
    const bool band = type != mne::FilterKernel::filter_type_index("LPF") && type != mne::FilterKernel::filter_type_index("HPF");
    f.set_range(from_key, 0.0, nyquist);
    f.set_range(to_key, 0.0, nyquist);
    if (band) {
        // The upper edge stays above the lower one
        if (f.get<double>(to_key) < f.get<double>(from_key)) {
            f.set_value(to_key, f.get<double>(from_key) + 1.0);
        }
        f.set_range(to_key, f.get<double>(from_key), nyquist);
        f.set_range(from_key, 0.0, f.get<double>(to_key));
    }
    const double from = f.get<double>(from_key);
    const double to = f.get<double>(to_key);
    double center = from;
    double bandwidth = 0.0;
    if (type == mne::FilterKernel::filter_type_index("LPF")) {
        center = to;
    } else if (band) {
        bandwidth = to - from;
        center = from + bandwidth / 2.0;
    }
    const int method = std::max(mne::FilterKernel::design_method_index(f.get<std::string>(method_key)), 0);
    m_filter = mne::FilterKernel("Designed Filter", type, taps(), center / nyquist, bandwidth / nyquist,
                                 f.get<double>(transition_key) / nyquist, m_sfreq, method);
    m_response->set_filter(m_filter);
    m_updating = updating;

    if (!report) {
        return;
    }
    if (m_callbacks.from_changed) {
        m_callbacks.from_changed(from);
    }
    if (m_callbacks.to_changed) {
        m_callbacks.to_changed(to);
    }
    if (m_callbacks.filter_changed) {
        m_callbacks.filter_changed(m_filter);
    }
}

//=============================================================================================================

void FilterDesignPanel::update_type_fields()
{
    const bool updating = std::exchange(m_updating, true);
    const std::string type = form().get<std::string>(type_key);
    form().set_value(kHasFrom, type != "LPF");
    form().set_value(kHasTo, type != "HPF");
    m_updating = updating;
}

//=============================================================================================================

void FilterDesignPanel::report_status(const Expected<void>& result, const std::string& done)
{
    const bool updating = std::exchange(m_updating, true);
    if (result) {
        form().set_value(kStatus, done);
        form().set_tone(kStatus, SettingField::Tone::Positive);
    } else {
        mne::Log::warning("{}", result.error().what());
        form().set_value(kStatus, std::string(result.error().what()));
        form().set_tone(kStatus, SettingField::Tone::Negative);
    }
    m_updating = updating;
}
