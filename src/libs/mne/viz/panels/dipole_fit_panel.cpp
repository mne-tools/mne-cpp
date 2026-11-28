//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     dipole_fit_panel.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    DipoleFitPanel class definition.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "dipole_fit_panel.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <array>
#include <ctime>
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

constexpr const char* kNone = "None";

constexpr const char* kMeasurement = "measurement";
constexpr const char* kSet = "set";
constexpr const char* kMeg = "meg";
constexpr const char* kEeg = "eeg";
constexpr const char* kTmin = "tmin";
constexpr const char* kTmax = "tmax";
constexpr const char* kTstep = "tstep";
constexpr const char* kTint = "tint";
constexpr const char* kBmin = "bmin";
constexpr const char* kBmax = "bmax";
constexpr const char* kMinDistance = "minDistance";
constexpr const char* kGuessRadius = "guessRadius";
constexpr const char* kGradNoise = "gradNoise";
constexpr const char* kMagNoise = "magNoise";
constexpr const char* kEegNoise = "eegNoise";
constexpr const char* kGradReg = "gradReg";
constexpr const char* kMagReg = "magReg";
constexpr const char* kEegReg = "eegReg";
constexpr const char* kNoise = "noise";
constexpr const char* kBem = "bem";
constexpr const char* kMri = "mri";
constexpr const char* kOriginX = "originX";
constexpr const char* kOriginY = "originY";
constexpr const char* kOriginZ = "originZ";
constexpr const char* kRadius = "eegRadius";
constexpr const char* kName = "name";
constexpr const char* kFit = "fit";

/** No baseline: the plugin's marker for an empty interval. */
constexpr int kNoBaseline = 1000000;

SettingField make(const char* key, SettingField::Kind kind, const std::string& label, SettingValue value, const char* group)
{
    SettingField field;
    field.key = key;
    field.kind = kind;
    field.label = label;
    field.default_value = std::move(value);
    field.group = group;
    // The dipole fit starts afresh each session
    field.persistent = false;
    return field;
}

SettingField whole(const char* key, const std::string& label, int value, int minimum, int maximum, const char* group)
{
    SettingField field = make(key, SettingField::Kind::Int, label, value, group);
    field.minimum = minimum;
    field.maximum = maximum;
    return field;
}

SettingField decimal(const char* key, const std::string& label, double value, double minimum, double maximum, const char* group)
{
    SettingField field = make(key, SettingField::Kind::Double, label, value, group);
    field.minimum = minimum;
    field.maximum = maximum;
    field.decimals = 2;
    return field;
}

const char* model_key(DipoleFitPanel::Model kind)
{
    switch (kind) {
        case DipoleFitPanel::Model::Measurement:
            return kMeasurement;
        case DipoleFitPanel::Model::Bem:
            return kBem;
        case DipoleFitPanel::Model::Mri:
            return kMri;
        case DipoleFitPanel::Model::Noise:
            return kNoise;
    }
    return kMeasurement;
}

SettingField model_field(const char* key, const std::string& label, const char* group)
{
    SettingField field = make(key, SettingField::Kind::Choice, label, std::string(kNone), group);
    field.choices = {{kNone, kNone}};
    return field;
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

DipoleFitPanel::DipoleFitPanel()
: Panel("DipoleFitView", "Dipole Fit")
{
    set_width(330);
    SettingsForm& f = form();
    f.add(model_field(kMeasurement, "Measurement file:", "Input"));
    f.add(whole(kSet, "Set:", 1, 0, 99, "Input"));
    f.add(make(kMeg, SettingField::Kind::Bool, "MEG", false, "Modality"));
    f.add(make(kEeg, SettingField::Kind::Bool, "EEG", false, "Modality"));
    f.add(whole(kTmin, "Start time (ms):", 0, 0, 99999999, "Time Scale"));
    f.add(whole(kTmax, "End time (ms):", 0, 0, 99999999, "Time Scale"));
    f.add(whole(kTstep, "Time step between frames (ms):", -1, -1, 99999999, "Time Scale"));
    f.add(whole(kTint, "Time integration (ms):", 0, 0, 99, "Time Scale"));
    f.add(whole(kBmin, "Baseline min (ms):", 0, -99999999, 99999999, "Preprocessing"));
    f.add(whole(kBmax, "Baseline max (ms):", 0, -99999999, 99999999, "Preprocessing"));
    f.add(decimal(kMinDistance, "Min distance from inner skull (mm):", 10.0, 0.0, 999.99, "Fitting"));
    f.add(decimal(kGuessRadius, "Spherical volume radius guess (mm):", 80.0, 0.0, 999.99, "Fitting"));
    f.add(decimal(kGradNoise, "GRAD Noise (fT/cm):", 5.0, -99.0, 99.99, "Noise"));
    f.add(decimal(kMagNoise, "MAG Noise (fT):", 20.0, -99.0, 99.99, "Noise"));
    f.add(decimal(kEegNoise, "EEG Noise (uV):", 0.2, -99.0, 99.99, "Noise"));
    f.add(model_field(kNoise, "Noise matrix:", "Noise"));
    f.add(decimal(kGradReg, "Planar fixed (fT/cm):", 0.1, -99.0, 99.99, "Regularization"));
    f.add(decimal(kMagReg, "Axial fixed (fT):", 0.1, -99.0, 99.99, "Regularization"));
    f.add(decimal(kEegReg, "EEG fixed (uV):", 0.1, -99.0, 99.99, "Regularization"));
    f.add(model_field(kBem, "BEM model:", "Forward Model"));
    f.add(model_field(kMri, "MRI coords:", "Forward Model"));
    f.add(decimal(kOriginX, "Origin x (mm):", 0.0, -200.0, 200.0, "Sphere Model"));
    f.add(decimal(kOriginY, "Origin y (mm):", 0.0, -200.0, 200.0, "Sphere Model"));
    f.add(decimal(kOriginZ, "Origin z (mm):", 40.0, -200.0, 200.0, "Sphere Model"));
    f.add(decimal(kRadius, "EEG sphere radius (mm):", 90.0, 1.0, 300.0, "Sphere Model"));
    f.add(make(kName, SettingField::Kind::Text, "Name:", std::string(), "Fit"));
    f.add(make(kFit, SettingField::Kind::Action, "Fit", false, "Fit"));

    m_listener = f.add_listener([this](const std::string& key, const std::optional<SettingValue>& value) {
        if (!m_updating && (value || key == kFit)) {
            changed(key);
        }
    });
}

//=============================================================================================================

DipoleFitPanel::~DipoleFitPanel()
{
    form().remove_listener(m_listener);
}

//=============================================================================================================

void DipoleFitPanel::changed(const std::string& key)
{
    if (key == kFit) {
        if (m_callbacks.fit) {
            m_callbacks.fit(fit_name());
        }
    } else if (key == kSet) {
        if (m_callbacks.set_changed) {
            m_callbacks.set_changed(form().get<int>(kSet));
        }
    } else if (key == kMeg || key == kEeg) {
        report_modality();
    } else if (key == kTmin || key == kTmax || key == kTstep || key == kTint) {
        report_time();
    } else if (key == kBmin || key == kBmax) {
        report_baseline();
    } else if (key == kMinDistance || key == kGuessRadius) {
        report_fitting();
    } else if (key == kGradNoise || key == kMagNoise || key == kEegNoise) {
        report_noise();
    } else if (key == kGradReg || key == kMagReg || key == kEegReg) {
        report_regularization();
    } else if (key == kOriginX || key == kOriginY || key == kOriginZ || key == kRadius) {
        report_sphere();
    } else {
        for (const Model kind : {Model::Measurement, Model::Bem, Model::Mri, Model::Noise}) {
            if (key == model_key(kind)) {
                if (kind == Model::Measurement) {
                    name_fit();
                }
                if (m_callbacks.model_changed) {
                    m_callbacks.model_changed(kind, model(kind));
                }
            }
        }
    }
}

//=============================================================================================================

void DipoleFitPanel::name_fit()
{
    const std::string name = measurement_name(model(Model::Measurement));
    std::string fitName;
    if (!name.empty()) {
        const std::time_t seconds = std::time(nullptr);
        std::tm local{};
#ifdef _WIN32
        localtime_s(&local, &seconds);
#else
        localtime_r(&seconds, &local);
#endif
        std::array<char, 64> month{};
        std::array<char, 64> rest{};
        std::strftime(month.data(), month.size(), "%B", &local);
        std::strftime(rest.data(), rest.size(), "%Y %H:%M:%S", &local);
        fitName = "Dipole Fit - " + name + " - " + month.data() + " " + std::to_string(local.tm_mday) + " " + rest.data();
    }
    const bool updating = std::exchange(m_updating, true);
    form().set_value(kName, fitName);
    m_updating = updating;
}

//=============================================================================================================

std::string DipoleFitPanel::measurement_name(const std::string& measurement)
{
    // The first part between dots that is not empty
    std::size_t start = measurement.find_first_not_of('.');
    if (start == std::string::npos) {
        return {};
    }
    std::string name = measurement.substr(start, measurement.find('.', start) - start);
    for (const char* suffix : {"-ave", "_ave", "-raw", "_raw"}) {
        if (name.ends_with(suffix)) {
            name.resize(name.size() - 4);
            break;
        }
    }
    return name;
}

//=============================================================================================================

void DipoleFitPanel::add_model(Model kind, const std::string& name)
{
    const char* key = model_key(kind);
    std::vector<SettingChoice> choices = form().field(key)->choices;
    choices.push_back({name, name});
    const bool updating = std::exchange(m_updating, true);
    form().set_choices(key, std::move(choices));
    m_updating = updating;
}

//=============================================================================================================

void DipoleFitPanel::remove_model(Model kind, const std::string& name)
{
    const char* key = model_key(kind);
    std::vector<SettingChoice> choices = form().field(key)->choices;
    const auto it = std::ranges::find(choices, name, &SettingChoice::value);
    if (it == choices.end() || name == kNone) {
        return;
    }
    choices.erase(it);
    const bool updating = std::exchange(m_updating, true);
    form().set_choices(key, std::move(choices));
    m_updating = updating;
}

//=============================================================================================================

std::string DipoleFitPanel::model(Model kind) const
{
    return form().get<std::string>(model_key(kind), kNone);
}

//=============================================================================================================

void DipoleFitPanel::report_parameters() const
{
    report_time();
    report_modality();
    report_fitting();
    report_baseline();
    report_noise();
    report_regularization();
    report_sphere();
}

//=============================================================================================================

void DipoleFitPanel::report_modality() const
{
    if (m_callbacks.modality_changed) {
        m_callbacks.modality_changed(form().get<bool>(kEeg), form().get<bool>(kMeg));
    }
}

//=============================================================================================================

void DipoleFitPanel::report_time() const
{
    if (m_callbacks.time_changed) {
        const SettingsForm& f = form();
        m_callbacks.time_changed(f.get<int>(kTmin), f.get<int>(kTmax), f.get<int>(kTstep), f.get<int>(kTint));
    }
}

//=============================================================================================================

void DipoleFitPanel::report_baseline() const
{
    if (m_callbacks.baseline_changed) {
        const int bmin = form().get<int>(kBmin);
        const int bmax = form().get<int>(kBmax);
        if (bmin != bmax) {
            m_callbacks.baseline_changed(bmin, bmax);
        } else {
            m_callbacks.baseline_changed(kNoBaseline, kNoBaseline);
        }
    }
}

//=============================================================================================================

void DipoleFitPanel::report_fitting() const
{
    if (m_callbacks.fitting_changed) {
        m_callbacks.fitting_changed(form().get<double>(kMinDistance), form().get<double>(kGuessRadius));
    }
}

//=============================================================================================================

void DipoleFitPanel::report_noise() const
{
    if (m_callbacks.noise_changed) {
        const SettingsForm& f = form();
        m_callbacks.noise_changed(f.get<double>(kGradNoise), f.get<double>(kMagNoise), f.get<double>(kEegNoise));
    }
}

//=============================================================================================================

void DipoleFitPanel::report_regularization() const
{
    if (m_callbacks.regularization_changed) {
        const SettingsForm& f = form();
        m_callbacks.regularization_changed(f.get<double>(kGradReg), f.get<double>(kMagReg), f.get<double>(kEegReg));
    }
}

//=============================================================================================================

void DipoleFitPanel::report_sphere() const
{
    if (m_callbacks.sphere_changed) {
        const SettingsForm& f = form();
        m_callbacks.sphere_changed(f.get<double>(kOriginX), f.get<double>(kOriginY), f.get<double>(kOriginZ), f.get<double>(kRadius));
    }
}

//=============================================================================================================

std::string DipoleFitPanel::fit_name() const
{
    return form().get<std::string>(kName);
}
