//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     hpi_panel.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    HpiPanel class definition.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "hpi_panel.h"
#include "../models/item_list.h"
#include <mne/base/byte_stream.h>
#include <mne/base/log.h>
#include <mne/base/string_utils.h>
#include <mne/fiff/fiff_constants.h>
#include <mne/fiff/fiff_dig_point.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <array>
#include <charconv>
#include <cstdio>
#include <memory>
#include <string_view>
#include <system_error>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::viz;

//=============================================================================================================
// DEFINE STATIC METHODS
//=============================================================================================================

namespace
{

constexpr const char* kDigitizerFile = "digitizerFile";
constexpr const char* kDigitizerStatus = "digitizerStatus";
constexpr const char* kLoadedCoils = "loadedCoils";
constexpr const char* kLoadedFiducials = "loadedFiducials";
constexpr const char* kLoadedEeg = "loadedEeg";
constexpr const char* kLoadedAdditional = "loadedAdditional";
constexpr const char* kCoilFreqs = "coilFreqs";
constexpr const char* kCoilPreset = "coilPreset";
constexpr const char* kOrder = "doFreqOrder";
constexpr const char* kSsp = "useSSP";
constexpr const char* kComp = "useCOMP";
constexpr const char* kContinuous = "continousHPI";
constexpr const char* kWindow = "fittingWindowSize";
constexpr const char* kSingleFit = "doSingleFit";
constexpr const char* kResults = "coilResults";
constexpr const char* kAverageError = "averageError";
constexpr const char* kMaxError = "maxError";
constexpr const char* kFitFeedback = "fitFeedback";
constexpr const char* kAverageGof = "averageGof";
constexpr const char* kMoveThreshold = "moveThreshold";
constexpr const char* kRotThreshold = "rotThreshold";
constexpr const char* kMoveResult = "moveResult";
constexpr const char* kRotResult = "rotResult";
constexpr const char* kMoveFeedback = "movementFeedback";

constexpr const char* kSetup = "HPI Setup";
constexpr const char* kFitting = "Fitting";
constexpr const char* kQuality = "Fitting Quality";
constexpr const char* kMovements = "Head Movements";

constexpr const char* kNoError = "0mm";
constexpr const char* kNoGof = "00.00%";

/** Neuromag VectorView coil frequencies in Hz, by number of coils. */
constexpr std::string_view kVectorViewPresets = R"({
    "4": [{"name": "VV 154-166Hz", "coils": [154, 158, 161, 166]},
          {"name": "VV 293-321Hz", "coils": [293, 307, 314, 321]},
          {"name": "VV 586-642Hz", "coils": [586, 614, 628, 642]}],
    "5": [{"name": "VV 154-172Hz", "coils": [154, 158, 161, 166, 172]},
          {"name": "VV 293-328Hz", "coils": [293, 307, 314, 321, 328]},
          {"name": "VV 586-656Hz", "coils": [586, 614, 628, 642, 656]}]
})";

SettingField make(const char* key, SettingField::Kind kind, const std::string& label, SettingValue value, const char* group,
                  bool persistent = false)
{
    SettingField field;
    field.key = key;
    field.kind = kind;
    field.label = label;
    field.default_value = std::move(value);
    field.group = group;
    field.persistent = persistent;
    return field;
}

SettingField millimeters(const char* key, const std::string& label, double value, const std::string& unit, bool persistent)
{
    SettingField field = make(key, SettingField::Kind::Double, label, value, persistent ? kQuality : kMovements, persistent);
    field.maximum = 9999.99;
    field.decimals = 2;
    field.unit = unit;
    return field;
}

std::string fixed(double value, const char* unit)
{
    std::array<char, 64> text{};
    std::snprintf(text.data(), text.size(), "%.2f%s", value, unit);
    return text.data();
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

HpiPanel::HpiPanel()
: Panel("HpiSettingsView", "HPI Fitting")
, m_frequencies{293, 307, 314, 321}
, m_presets(JsonDocument::from_json(kVectorViewPresets).object())
{
    SettingsForm& f = form();
    SettingField file = make(kDigitizerFile, SettingField::Kind::Path, "Load digitizers:", std::string(), kSetup);
    file.file_filter = "Fiff file (*.fif)";
    file.commit = SettingField::Commit::WhenFinished;
    f.add(std::move(file));
    f.add(make(kDigitizerStatus, SettingField::Kind::Info, "Digitizers:", std::string("not loaded"), kSetup));
    f.add(make(kLoadedCoils, SettingField::Kind::Info, "Loaded HPI coils:", std::string("0"), kSetup));
    f.add(make(kLoadedFiducials, SettingField::Kind::Info, "Loaded fiducials:", std::string("0"), kSetup));
    f.add(make(kLoadedEeg, SettingField::Kind::Info, "Loaded EEG positions:", std::string("0"), kSetup));
    f.add(make(kLoadedAdditional, SettingField::Kind::Info, "Loaded additional points:", std::string("0"), kSetup));
    f.add(make(kCoilPreset, SettingField::Kind::Choice, "Coil preset:", std::string(), kSetup));
    SettingField frequencies = make(kCoilFreqs, SettingField::Kind::Text, "Coil frequencies (Hz):", format_frequencies(m_frequencies), kSetup, true);
    frequencies.tooltip = "Comma-separated frequency of each coil in Hz; \"none\" for a coil without a frequency.";
    frequencies.commit = SettingField::Commit::WhenFinished;
    f.add(std::move(frequencies));
    f.add(make(kOrder, SettingField::Kind::Action, "Order coil frequencies", false, kSetup));

    f.add(make(kSsp, SettingField::Kind::Bool, "Use SSP", false, kFitting, true));
    f.add(make(kComp, SettingField::Kind::Bool, "Use compensators", false, kFitting, true));
    f.add(make(kContinuous, SettingField::Kind::Bool, "Do continous HPI fitting", false, kFitting, true));
    SettingField window = make(kWindow, SettingField::Kind::Int, "Win Size [Samples]:", 300, kFitting, true);
    window.minimum = 10;
    window.maximum = 1000000;
    window.step = 10;
    f.add(std::move(window));
    f.add(make(kSingleFit, SettingField::Kind::Action, "Perform single HPI fit", false, kFitting));

    auto results = std::make_shared<ItemList>(ItemList::SelectionMode::None);
    results->set_visible_rows(5);
    add_list(kResults, "Fitting results (coil: error, GoF):", std::move(results), kQuality);
    f.add(make(kAverageError, SettingField::Kind::Info, "Average error:", std::string(kNoError), kQuality));
    f.add(millimeters(kMaxError, "Error threshold:", 10.0, " mm", true));
    f.add(make(kFitFeedback, SettingField::Kind::Info, "Last fit:", std::string("No fit yet"), kQuality));
    f.add(make(kAverageGof, SettingField::Kind::Info, "Average GoF:", std::string("0.00"), kQuality));

    f.add(millimeters(kMoveThreshold, "Movement threshold:", 3.0, " mm", false));
    f.add(millimeters(kRotThreshold, "Rotation threshold:", 5.0, " °", false));
    f.add(make(kMoveResult, SettingField::Kind::Info, "Movement from last fit:", std::string(), kMovements));
    f.add(make(kRotResult, SettingField::Kind::Info, "Rotation from last fit:", std::string(), kMovements));
    f.add(make(kMoveFeedback, SettingField::Kind::Info, "Head movement:", std::string("No fit yet"), kMovements));

    show_frequencies();
    setup_presets();

    m_listener = f.add_listener([this](const std::string& key, const std::optional<SettingValue>& value) {
        if (!m_updating && (value || key == kOrder || key == kSingleFit)) {
            changed(key);
        }
    });
}

//=============================================================================================================

HpiPanel::~HpiPanel()
{
    form().remove_listener(m_listener);
}

//=============================================================================================================

void HpiPanel::changed(const std::string& key)
{
    if (key == kDigitizerFile) {
        const std::string file = form().get<std::string>(kDigitizerFile);
        if (!file.empty()) {
            load_digitizers(path_from_utf8(file));
        }
    } else if (key == kCoilFreqs) {
        set_coil_frequencies(parse_frequencies(form().get<std::string>(kCoilFreqs)));
        report_frequencies();
    } else if (key == kCoilPreset) {
        const std::string name = form().get<std::string>(kCoilPreset);
        for (const JsonValue& entry : m_presets[std::to_string(m_frequencies.size())].to_array()) {
            const JsonObject& preset = entry.to_object();
            if (preset["name"].to_std_string() == name) {
                std::vector<int> coils;
                const JsonArray& values = preset["coils"].to_array();
                coils.reserve(values.size());
                for (const JsonValue& coil : values) {
                    coils.push_back(coil.to_int());
                }
                set_coil_frequencies(coils);
                report_frequencies();
                break;
            }
        }
    } else if (key == kOrder) {
        if (m_callbacks.order_frequencies) {
            m_callbacks.order_frequencies();
        }
    } else if (key == kSingleFit) {
        if (m_callbacks.single_fit) {
            m_callbacks.single_fit();
        }
    } else if (key == kSsp) {
        if (m_callbacks.ssp_changed) {
            m_callbacks.ssp_changed(use_ssp());
        }
    } else if (key == kComp) {
        if (m_callbacks.compensators_changed) {
            m_callbacks.compensators_changed(use_compensators());
        }
    } else if (key == kContinuous) {
        if (m_callbacks.continuous_fitting_changed) {
            m_callbacks.continuous_fitting_changed(continuous_fitting());
        }
    } else if (key == kWindow) {
        if (m_callbacks.fitting_window_size_changed) {
            m_callbacks.fitting_window_size_changed(fitting_window_size());
        }
    } else if (key == kMaxError) {
        if (m_callbacks.allowed_error_changed) {
            m_callbacks.allowed_error_changed(allowed_error());
        }
    } else if (key == kMoveThreshold) {
        if (m_callbacks.allowed_movement_changed) {
            m_callbacks.allowed_movement_changed(allowed_movement());
        }
    } else if (key == kRotThreshold) {
        if (m_callbacks.allowed_rotation_changed) {
            m_callbacks.allowed_rotation_changed(allowed_rotation());
        }
    }
}

//=============================================================================================================

void HpiPanel::report_frequencies() const
{
    if (m_callbacks.coil_frequencies_changed) {
        m_callbacks.coil_frequencies_changed(m_frequencies);
    }
}

//=============================================================================================================

void HpiPanel::set_info(const std::string& key, const std::string& text, SettingField::Tone tone)
{
    const bool updating = std::exchange(m_updating, true);
    form().set_value(key, text);
    form().set_tone(key, tone);
    m_updating = updating;
}

//=============================================================================================================

void HpiPanel::show_frequencies()
{
    const bool updating = std::exchange(m_updating, true);
    form().set_value(kCoilFreqs, format_frequencies(m_frequencies));
    m_updating = updating;

    m_errors.resize(m_frequencies.size(), kNoError);
    m_goodness.resize(m_frequencies.size(), kNoGof);
    std::vector<std::string> rows;
    rows.reserve(m_frequencies.size());
    for (std::size_t i = 0; i < m_frequencies.size(); ++i) {
        rows.push_back("Coil " + std::to_string(i + 1) + ": " + m_errors[i] + ", " + m_goodness[i]);
    }
    list(kResults)->set_items(std::move(rows));
}

//=============================================================================================================

void HpiPanel::setup_presets()
{
    const std::vector<std::string> names = coil_presets();
    std::vector<SettingChoice> choices;
    choices.reserve(names.size() + 1);
    choices.push_back({"", "Load preset"});
    for (const std::string& name : names) {
        choices.push_back({name, name});
    }
    const bool updating = std::exchange(m_updating, true);
    form().set_choices(kCoilPreset, std::move(choices));
    form().set_value(kCoilPreset, std::string());
    m_updating = updating;
}

//=============================================================================================================

std::vector<std::string> HpiPanel::coil_presets() const
{
    const JsonArray& presets = m_presets[std::to_string(m_frequencies.size())].to_array();
    std::vector<std::string> names;
    names.reserve(presets.size());
    for (const JsonValue& entry : presets) {
        names.push_back(entry.to_object()["name"].to_std_string());
    }
    return names;
}

//=============================================================================================================

void HpiPanel::set_errors(const std::vector<double>& errors, double meanError)
{
    for (std::size_t i = 0; i < errors.size() && i < m_errors.size(); ++i) {
        m_errors[i] = fixed(errors[i] * 1000.0, " mm");
    }
    show_frequencies();
    set_info(kAverageError, fixed(meanError * 1000.0, " mm"));
    const bool bad = meanError * 1000.0 > allowed_error();
    set_info(kFitFeedback, bad ? "Bad" : "Good", bad ? SettingField::Tone::Negative : SettingField::Tone::Positive);
}

//=============================================================================================================

void HpiPanel::set_goodness_of_fit(const Eigen::VectorXd& goodness, double meanGoodness)
{
    for (Eigen::Index i = 0; i < goodness.size() && static_cast<std::size_t>(i) < m_goodness.size(); ++i) {
        m_goodness[static_cast<std::size_t>(i)] = fixed(goodness[i] * 100.0, " %");
    }
    show_frequencies();
    set_info(kAverageGof, fixed(meanGoodness * 100.0, " %"));
}

//=============================================================================================================

void HpiPanel::set_movement(double movement, double rotation)
{
    set_info(kMoveResult, fixed(movement * 1000.0, " mm"));
    set_info(kRotResult, fixed(rotation, " °"));
    const bool big = movement * 1000.0 > allowed_movement() || rotation > allowed_rotation();
    set_info(kMoveFeedback, big ? "Big" : "Small", big ? SettingField::Tone::Negative : SettingField::Tone::Positive);
}

//=============================================================================================================

void HpiPanel::set_digitizers(const mne::FiffDigPointSet& digitizers)
{
    int coils = 0;
    int fiducials = 0;
    int eeg = 0;
    int additional = 0;
    for (const FiffDigPoint& point : digitizers) {
        switch (point.kind) {
            case FIFFV_POINT_HPI:
                ++coils;
                break;
            case FIFFV_POINT_CARDINAL:
                ++fiducials;
                break;
            case FIFFV_POINT_EEG:
                ++eeg;
                break;
            case FIFFV_POINT_EXTRA:
                ++additional;
                break;
            default:
                break;
        }
    }
    set_info(kLoadedCoils, std::to_string(coils));
    set_info(kLoadedFiducials, std::to_string(fiducials));
    set_info(kLoadedEeg, std::to_string(eeg));
    set_info(kLoadedAdditional, std::to_string(additional));
    set_info(kDigitizerStatus, "loaded", SettingField::Tone::Positive);

    // One coil per HPI point; new coils start without a frequency and the fit results start afresh
    m_frequencies.resize(static_cast<std::size_t>(coils), -1);
    m_errors.clear();
    m_goodness.clear();
    show_frequencies();
    setup_presets();
}

//=============================================================================================================

bool HpiPanel::load_digitizers(const std::filesystem::path& file)
{
    const bool updating = std::exchange(m_updating, true);
    form().set_value(kDigitizerFile, utf8_from_path(file));
    m_updating = updating;

    std::error_code error;
    FiffDigPointSet digitizers;
    if (std::filesystem::is_regular_file(file, error)) {
        if (auto read = FiffDigPointSet::read(file)) {
            digitizers = std::move(*read);
        }
    }
    if (digitizers.empty()) {
        set_info(kDigitizerStatus, "File could not be loaded!", SettingField::Tone::Negative);
        return false;
    }
    set_digitizers(digitizers);
    report_frequencies();
    // Continuous fitting stops while the digitizers change
    if (continuous_fitting()) {
        const bool updatingContinuous = std::exchange(m_updating, true);
        form().set_value(kContinuous, false);
        m_updating = updatingContinuous;
        if (m_callbacks.continuous_fitting_changed) {
            m_callbacks.continuous_fitting_changed(false);
        }
    }
    if (m_callbacks.digitizers_changed) {
        m_callbacks.digitizers_changed(digitizers, file);
    }
    return true;
}

//=============================================================================================================

bool HpiPanel::load_coil_presets(const std::filesystem::path& file)
{
    const Expected<std::string> text = read_text_file(file);
    if (!text) {
        Log::warning("Cannot read HPI coil presets: {}", text.error().what());
        return false;
    }
    JsonParseError error;
    const JsonDocument presets = JsonDocument::from_json(*text, &error);
    if (error.error != JsonParseError::NoError || !presets.is_object()) {
        Log::warning("{} holds no JSON object of HPI coil presets", utf8_from_path(file));
        return false;
    }
    m_presets = presets.object();
    setup_presets();
    return true;
}

//=============================================================================================================

void HpiPanel::set_coil_frequencies(const std::vector<int>& frequencies)
{
    const std::size_t count = m_frequencies.size();
    m_frequencies = frequencies;
    show_frequencies();
    if (m_frequencies.size() != count) {
        setup_presets();
    }
}

//=============================================================================================================

bool HpiPanel::use_ssp() const
{
    return form().get<bool>(kSsp);
}

//=============================================================================================================

bool HpiPanel::use_compensators() const
{
    return form().get<bool>(kComp);
}

//=============================================================================================================

bool HpiPanel::continuous_fitting() const
{
    return form().get<bool>(kContinuous);
}

//=============================================================================================================

int HpiPanel::fitting_window_size() const
{
    return form().get<int>(kWindow);
}

//=============================================================================================================

double HpiPanel::allowed_error() const
{
    return form().get<double>(kMaxError);
}

//=============================================================================================================

double HpiPanel::allowed_movement() const
{
    return form().get<double>(kMoveThreshold);
}

//=============================================================================================================

double HpiPanel::allowed_rotation() const
{
    return form().get<double>(kRotThreshold);
}

//=============================================================================================================

void HpiPanel::load(const SettingsStore& store, const std::string& path)
{
    const bool updating = std::exchange(m_updating, true);
    Panel::load(store, path);
    m_updating = updating;
    const std::vector<int> stored = parse_frequencies(form().get<std::string>(kCoilFreqs));
    // Older versions stored the frequencies as a list the text field cannot read
    set_coil_frequencies(stored.empty() ? std::vector<int>(m_frequencies) : stored);
}

//=============================================================================================================

std::vector<int> HpiPanel::parse_frequencies(const std::string& text)
{
    std::vector<int> frequencies;
    std::size_t start = 0;
    while (start < text.size()) {
        std::size_t end = text.find_first_of(", ;\t", start);
        if (end == std::string::npos) {
            end = text.size();
        }
        if (end > start) {
            const std::string_view token(text.data() + start, end - start);
            int frequency = -1;
            const auto [rest, error] = std::from_chars(token.data(), token.data() + token.size(), frequency);
            frequencies.push_back(error == std::errc() && rest == token.data() + token.size() && frequency > 0 ? frequency : -1);
        }
        start = end + 1;
    }
    return frequencies;
}

//=============================================================================================================

std::string HpiPanel::format_frequencies(const std::vector<int>& frequencies)
{
    std::string text;
    for (std::size_t i = 0; i < frequencies.size(); ++i) {
        if (i > 0) {
            text += ", ";
        }
        text += frequencies[i] > 0 ? std::to_string(frequencies[i]) : std::string("none");
    }
    return text;
}
