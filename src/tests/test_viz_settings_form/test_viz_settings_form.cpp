//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_viz_settings_form.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Tests SettingsForm, the toolkit-neutral description of settings panels, without Qt.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/viz/forms/settings_form.h>
#include <mne/viz/host/settings_store.h>
#include <mne/viz/panels/artifact_panel.h>
#include <mne/viz/panels/averaging_panel.h>
#include <mne/viz/panels/compensator_panel.h>
#include <mne/viz/panels/connectivity_panel.h>
#include <mne/viz/panels/filter_settings_panel.h>
#include <mne/viz/figures/canvas.h>
#include <mne/viz/figures/text_metrics.h>
#include <mne/viz/panels/covariance_panel.h>
#include <mne/viz/panels/forward_panel.h>
#include <mne/fiff/fiff_constants.h>
#include <mne/viz/panels/minimum_norm_panel.h>
#include <mne/viz/panels/projectors_panel.h>
#include <mne/viz/panels/sphara_panel.h>
#include <mne/viz/panels/time_frequency_panel.h>
#include <mne/viz/panels/trigger_detection_panel.h>
#include <mne/fiff/fiff_info.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cmath>
#include <cstdio>
#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::viz;

namespace
{

int g_failures = 0;

#define CHECK(condition)                                                                       \
    do {                                                                                       \
        if (!(condition)) {                                                                    \
            std::fprintf(stderr, "%s:%d: CHECK(%s) failed\n", __FILE__, __LINE__, #condition); \
            ++g_failures;                                                                      \
        }                                                                                      \
    } while (false)

void test_fields_and_values()
{
    SettingsForm form("Averaging");
    form.add_int("trials", "Number of trials", 10, 1, 100, 1, " trials", "General");
    form.add_double("prestim", "Pre-stimulus", 0.1, 0.0, 1.0, 0.01, 2, " s", "General");
    form.add_bool("baseline", "Baseline correction", false, "Baseline");
    form.add_choice("mode", "Mode", {{"mean", "Mean"}, {"median", "Median"}}, "median");
    form.add_text("name", "Name", "avg");
    form.add_action("reset", "Reset");
    form.add_custom("trials_view");
    CHECK(form.title() == "Averaging" && form.fields().size() == 7);
    CHECK((form.groups() == std::vector<std::string>{"General", "Baseline", ""}));

    // Defaults; actions and custom fields hold no value
    CHECK(form.get<int>("trials") == 10 && form.get<double>("prestim") == 0.1 && !form.get<bool>("baseline"));
    CHECK(form.get<std::string>("mode") == "median" && form.get<std::string>("name") == "avg");
    CHECK(!form.value("reset") && !form.value("trials_view") && !form.value("missing"));
    // Ints read as doubles; the wrong type gives the fallback
    CHECK(form.get<double>("trials") == 10.0 && form.get<int>("prestim", -1) == -1);

    // Numbers are clamped and rounded, choices must exist, types must fit
    CHECK(form.set_value("trials", 500) && form.get<int>("trials") == 100);
    CHECK(form.set_value("trials", 7.6) && form.get<int>("trials") == 8);
    CHECK(!form.set_value("trials", 8));
    CHECK(form.set_value("prestim", -2) && form.get<double>("prestim") == 0.0);
    CHECK(!form.set_value("mode", std::string("max")) && form.get<std::string>("mode") == "median");
    CHECK(!form.set_value("baseline", 1) && !form.set_value("missing", true));

    // Ranges and choices change; values follow
    form.set_range("trials", 20, 50);
    CHECK(form.get<int>("trials") == 20 && form.field("trials")->minimum == 20.0);
    form.set_range("trials", 30, 10);
    CHECK(form.field("trials")->maximum == 30.0);
    form.set_choices("mode", {{"mean", "Mean"}});
    CHECK(form.get<std::string>("mode") == "mean");
    form.set_label("trials", "Trials");
    CHECK(form.field("trials")->label == "Trials");

    // Bulk values and reset
    form.set_values({{"baseline", true}, {"name", std::string("evoked")}, {"unknown", 3}});
    CHECK(form.get<bool>("baseline") && form.get<std::string>("name") == "evoked");
    CHECK(form.values().size() == 5);
    form.reset();
    CHECK(!form.get<bool>("baseline") && form.get<std::string>("name") == "avg" && form.get<int>("trials") == 30);

    // A field with the key of another replaces it
    form.add_bool("name", "Named", true);
    CHECK(form.fields().size() == 7 && form.get<bool>("name"));
}

void test_conditions()
{
    SettingsForm form;
    form.add_bool("baseline", "Baseline correction", false);
    SettingField from;
    from.key = "from";
    from.kind = SettingField::Kind::Int;
    from.enabled_if = "baseline";
    from.visible_if = "!hidden";
    form.add(from);
    form.add_bool("hidden", "Hide", false);
    CHECK(!form.is_enabled("from") && form.is_visible("from"));
    form.set_value("baseline", true);
    form.set_value("hidden", true);
    CHECK(form.is_enabled("from") && !form.is_visible("from"));
    CHECK(form.is_enabled("baseline") && !form.is_enabled("missing"));
}

void test_flags()
{
    SettingsForm form;
    form.add_flag("offline");
    SettingField compute;
    compute.key = "compute";
    compute.kind = SettingField::Kind::Action;
    compute.visible_if = "offline";
    form.add(compute);
    form.add_bool("baseline", "Baseline correction", false);

    // A flag holds a value conditions read, but is neither shown nor stored
    const SettingField* flag = form.field("offline");
    CHECK(flag && flag->kind == SettingField::Kind::Bool && flag->hidden && !flag->persistent);
    CHECK(!form.is_visible("offline") && !form.is_visible("compute"));
    CHECK(form.set_value("offline", true) && form.get<bool>("offline"));
    CHECK(form.is_visible("compute") && !form.is_visible("offline"));
    CHECK(form.add_flag("live", true).default_value == SettingValue(true) && form.get<bool>("live"));
    // Ordinary fields are shown and stored
    CHECK(!form.field("baseline")->hidden && form.field("baseline")->persistent && form.is_visible("baseline"));
}

void test_listeners()
{
    SettingsForm form;
    form.add_int("n", "N", 1, 0, 10);
    form.add_action("go", "Go");
    std::vector<std::pair<std::string, std::optional<SettingValue>>> heard;
    std::vector<std::string> changedFields;
    const int id = form.add_listener([&heard](const std::string& key, const std::optional<SettingValue>& value) {
        heard.emplace_back(key, value);
    });
    form.add_structure_listener([&changedFields](const std::string& key) {
        changedFields.push_back(key);
    });

    form.set_value("n", 5);
    form.set_value("n", 5);
    form.trigger("go");
    form.trigger("n");
    CHECK(heard.size() == 2 && heard[0].first == "n" && heard[0].second == SettingValue(5));
    CHECK(heard[1].first == "go" && !heard[1].second);

    // A range change that clamps the value is heard by both kinds of listeners
    form.set_range("n", 0, 3);
    CHECK(heard.size() == 3 && heard[2].second == SettingValue(3));
    form.add_bool("flag", "Flag", false);
    CHECK((changedFields == std::vector<std::string>{"n", "flag"}));

    form.remove_listener(id);
    form.set_value("n", 1);
    CHECK(heard.size() == 3);
}

void test_info_paths_colors_and_removal()
{
    SettingsForm form;
    std::vector<std::string> structure;
    form.add_structure_listener([&structure](const std::string& key) {
        structure.push_back(key);
    });
    const SettingField& info = form.add_info("status", "Status", "idle");
    CHECK(info.kind == SettingField::Kind::Info && !info.persistent && form.get<std::string>("status") == "idle");
    form.set_tone("status", SettingField::Tone::Negative);
    CHECK(form.field("status")->tone == SettingField::Tone::Negative);
    CHECK(form.set_value("status", std::string("failed")) && form.get<std::string>("status") == "failed");

    form.add_path("bem", "BEM", "/data/bem.fif", "FIFF files (*.fif)");
    form.add_directory("subjects", "Subjects", "/data/subjects");
    CHECK(form.field("bem")->file_filter == "FIFF files (*.fif)" && form.field("subjects")->kind == SettingField::Kind::Directory);

    // Colours are "#rrggbb" or "#rrggbbaa"
    form.add_color("color", "Colour", "#ff0000");
    CHECK(form.get<std::string>("color") == "#ff0000");
    CHECK(form.set_value("color", std::string("#00ff0080")));
    CHECK(!form.set_value("color", std::string("red")) && !form.set_value("color", std::string("#12345g")));
    CHECK(form.get<std::string>("color") == "#00ff0080");

    // Removing a field drops its value and tells the hosts
    CHECK(form.remove("bem") && !form.remove("bem"));
    CHECK(!form.field("bem") && !form.value("bem"));
    CHECK((structure == std::vector<std::string>{"status", "status", "bem", "subjects", "color", "bem"}));
}

void test_compensator_panel()
{
    std::vector<mne::FiffCtfComp> comps(3);
    comps[0].kind = 1;
    comps[1].kind = 2;
    comps[2].kind = 3;
    std::vector<int> chosen;
    CompensatorPanel panel;
    CHECK(panel.id() == "CompensatorView" && panel.title() == "Compensators" && panel.width() == 330);
    panel.set_selection_callback([&chosen](int to) {
        chosen.push_back(to);
    });
    panel.set_compensators(comps);
    CHECK(panel.form().fields().size() == 3 && chosen.empty());

    // At most one grade is on
    panel.form().set_value(CompensatorPanel::field_key(2), true);
    panel.form().set_value(CompensatorPanel::field_key(3), true);
    CHECK(panel.selected() == 3 && !panel.form().get<bool>(CompensatorPanel::field_key(2)));
    panel.form().set_value(CompensatorPanel::field_key(3), false);
    CHECK((chosen == std::vector<int>{2, 3, 0}) && panel.selected() == 0);
    panel.select(2);

    // The chosen grade is remembered and chosen again with the next data
    MemorySettingsStore store;
    panel.save(store, "app");
    CHECK(store.read_string("app/CompensatorView/active", {}) == "1:0,2:1,3:0");
    CompensatorPanel restored;
    restored.load(store, "app");
    restored.set_selection_callback([&chosen](int to) {
        chosen.push_back(to);
    });
    restored.set_compensators(comps);
    CHECK(restored.selected() == 2 && chosen.back() == 2 && restored.form().get<bool>(CompensatorPanel::field_key(2)));

    // New data replace the check boxes
    restored.set_compensators({comps[0]});
    CHECK(restored.form().fields().size() == 1 && restored.selected() == 0);
}

void test_projectors_panel()
{
    mne::FiffProj eeg;
    eeg.active = true;
    eeg.desc = "Average EEG = reference";
    mne::FiffProj ecg;
    ecg.active = false;
    ecg.desc = "ECG\nline";
    std::vector<std::vector<bool>> reported;
    ProjectorsPanel panel;
    panel.set_selection_callback([&reported](const std::vector<mne::FiffProj>& projs) {
        reported.emplace_back();
        for (const mne::FiffProj& proj : projs) {
            reported.back().push_back(proj.active);
        }
    });
    panel.set_projectors({eeg, ecg});
    CHECK(panel.form().fields().size() == 3 && !panel.form().get<bool>(ProjectorsPanel::kAllKey));
    CHECK((reported.back() == std::vector<bool>{true, false}));

    panel.form().set_value(ProjectorsPanel::field_key(1), true);
    CHECK(panel.form().get<bool>(ProjectorsPanel::kAllKey) && (reported.back() == std::vector<bool>{true, true}));
    panel.form().set_value(ProjectorsPanel::kAllKey, false);
    CHECK(!panel.form().get<bool>(ProjectorsPanel::field_key(0)) && (reported.back() == std::vector<bool>{false, false}));
    panel.set_active(1, true);

    // Remembered by description, which may contain the separators
    MemorySettingsStore store;
    panel.save(store, "app/");
    ProjectorsPanel restored;
    restored.load(store, "app");
    restored.set_projectors({eeg, ecg});
    CHECK(!restored.projectors()[0].active && restored.projectors()[1].active);

    // Modes reach the panel
    restored.set_gui_mode(Panel::GuiMode::Clinical);
    restored.set_processing_mode(Panel::ProcessingMode::Offline);
    CHECK(restored.gui_mode() == Panel::GuiMode::Clinical && restored.processing_mode() == Panel::ProcessingMode::Offline);
}

void test_forward_panel()
{
    ForwardPanel panel;
    CHECK(panel.id() == "FwdSettingsView" && !panel.atlas_loaded() && panel.clustering() && panel.cluster_sources() == 200);
    int computations = 0;
    std::vector<bool> clusterings;
    ForwardPanel::Callbacks callbacks;
    callbacks.compute = [&computations] {
        ++computations;
    };
    callbacks.clustering_changed = [&clusterings](bool on) {
        clusterings.push_back(on);
    };
    panel.set_callbacks(std::move(callbacks));

    // States show as toned text
    panel.set_status(static_cast<int>(ForwardPanel::Status::NotClustered));
    CHECK(panel.form().get<std::string>("status") == "Not Clustered" && panel.form().field("status")->tone == SettingField::Tone::Warning);
    panel.set_status(42);
    CHECK(panel.form().get<std::string>("status") == "Finished" && panel.form().field("status")->tone == SettingField::Tone::Positive);

    panel.set_solution_information(1, FIFFV_COORD_MRI, 8000, 306, 2);
    CHECK(panel.form().get<std::string>("sourceOrientation") == "free" && panel.form().get<std::string>("coordinateFrame") == "MRI Space");
    CHECK(panel.form().get<std::string>("sources") == "8000");

    // Without an atlas clustering cannot be switched on
    panel.form().set_value("cluster", false);
    panel.form().set_value("cluster", true);
    CHECK(!panel.clustering() && (clusterings == std::vector<bool>{false}));
    CHECK(panel.form().field("atlasStatus")->tone == SettingField::Tone::Warning);
    CHECK(!panel.set_atlas_directory("/nonexistent-atlas-directory"));

    // Range and the compute button
    panel.form().set_value("clusterSources", 5000);
    CHECK(panel.cluster_sources() == 999);
    panel.form().trigger("compute");
    CHECK(computations == 1);
    // Nothing of the forward panel is stored
    MemorySettingsStore store;
    panel.save(store, "app");
    CHECK(store.values().empty());
}

void test_processing_panels()
{
    // Covariance: whole seconds of samples, reported only on user changes
    CovariancePanel covariance;
    std::vector<int> samples;
    covariance.set_samples_callback([&samples](int value) {
        samples.push_back(value);
    });
    covariance.set_minimum_samples(1000);
    covariance.set_samples(90000);
    CHECK(covariance.samples() == 60000 && samples.empty());
    covariance.form().set_value("samples", 2000);
    CHECK((samples == std::vector<int>{2000}));

    // Time-frequency: trial and row together
    TimeFrequencyPanel timeFrequency;
    std::vector<std::pair<int, int>> trialRows;
    timeFrequency.set_callback([&trialRows](int trial, int row) {
        trialRows.emplace_back(trial, row);
    });
    timeFrequency.form().set_value("rowNumber", 4);
    CHECK(trialRows.size() == 1 && trialRows.back().second == 4 && timeFrequency.row() == 4);

    // SPHARA: the system sets labels and limits and reports clamped values
    SpharaPanel sphara;
    std::vector<std::tuple<std::string, int, int>> options;
    SpharaPanel::Callbacks spharaCallbacks;
    spharaCallbacks.options_changed = [&options](const std::string& system, int first, int second) {
        options.emplace_back(system, first, second);
    };
    sphara.set_callbacks(std::move(spharaCallbacks));
    sphara.form().set_value("system", std::string("EEG"));
    CHECK(options.size() == 1 && std::get<0>(options.back()) == "EEG" && std::get<1>(options.back()) == 256);
    CHECK(!sphara.form().is_visible("second") && sphara.form().field("first")->label == "EEG");

    // Minimum norm: a fixed method cannot change; the CMNE checkpoint persists
    MinimumNormPanel fixed("dSPM");
    CHECK(fixed.method() == "dSPM" && !fixed.form().is_enabled("method"));
    MinimumNormPanel minimumNorm;
    minimumNorm.set_model_checkpoint("model.onnx");
    MemorySettingsStore store;
    minimumNorm.save(store, "app");
    CHECK(store.values().size() == 1);
    MinimumNormPanel restoredNorm;
    std::vector<std::filesystem::path> checkpoints;
    MinimumNormPanel::Callbacks normCallbacks;
    normCallbacks.model_checkpoint_changed = [&checkpoints](const std::filesystem::path& path) {
        checkpoints.push_back(path);
    };
    restoredNorm.set_callbacks(std::move(normCallbacks));
    restoredNorm.load(store, "app");
    CHECK(restoredNorm.model_checkpoint() == std::filesystem::path("model.onnx") && checkpoints.size() == 1);
}

void test_connectivity_panel()
{
    ConnectivityPanel panel;
    CHECK(panel.metric() == "COR" && panel.number_trials() == 10 && panel.lower_frequency() == 7.0);
    std::vector<std::pair<float, float>> bands;
    std::vector<int> trials;
    ConnectivityPanel::Callbacks callbacks;
    // A host may revert changes while being told about them
    callbacks.number_trials_changed = [&panel, &trials](int value) {
        trials.push_back(value);
        panel.set_number_trials(15);
    };
    callbacks.frequency_band_changed = [&bands](float low, float high) {
        bands.emplace_back(low, high);
    };
    panel.set_callbacks(std::move(callbacks));
    panel.form().set_value("connNrTrials", 40);
    CHECK((trials == std::vector<int>{40}) && panel.number_trials() == 15);
    panel.set_frequency_band(4.0, 8.0);
    CHECK(bands.size() == 2 && bands.back() == std::make_pair(4.0f, 8.0f));

    // Restored settings are not reported
    MemorySettingsStore store;
    panel.form().set_value("connMethod", std::string("PLV"));
    panel.save(store, "app");
    ConnectivityPanel restored;
    std::vector<std::string> metrics;
    ConnectivityPanel::Callbacks restoredCallbacks;
    restoredCallbacks.metric_changed = [&metrics](const std::string& metric) {
        metrics.push_back(metric);
    };
    restored.set_callbacks(std::move(restoredCallbacks));
    restored.load(store, "app");
    CHECK(restored.metric() == "PLV" && restored.number_trials() == 15 && restored.upper_frequency() == 8.0 && metrics.empty());
}

void test_trigger_detection_panel()
{
    mne::FiffInfo info;
    for (const char* name : {"MEG 0111", "STI 001", "STI 014"}) {
        mne::FiffChInfo channel;
        channel.ch_name = name;
        channel.kind = std::string(name).starts_with("STI") ? FIFFV_STIM_CH : FIFFV_MEG_CH;
        info.chs.push_back(channel);
    }
    // The old keys: the channel by index, the threshold as mantissa and exponent; colours by trigger type
    MemorySettingsStore store;
    store.set_value("app/TriggerDetectionView/Activated", true);
    store.set_value("app/TriggerDetectionView/ChannelIndex", 1);
    store.set_value("app/TriggerDetectionView/FirstThresholdValue", 2.0);
    store.set_value("app/TriggerDetectionView/SecondThresholdValue", -2);
    store.set_value("app/TriggerDetectionView/ColorTypes", std::string("4"));
    store.set_value("app/TriggerDetectionView/Colors/4", std::string("#00ff00"));
    TriggerDetectionPanel panel;
    std::vector<std::string> channels;
    TriggerDetectionPanel::Callbacks callbacks;
    callbacks.trigger_info_changed = [&channels](const std::map<double, std::string>&, bool, const std::string& channel, double) {
        channels.push_back(channel);
    };
    panel.set_callbacks(std::move(callbacks));
    panel.load(store, "app");
    CHECK(channels.empty() && panel.active() && panel.threshold() == 0.02);
    panel.init(info);
    CHECK((channels == std::vector<std::string>{"STI 014"}) && panel.colors().at(4.0) == "#00ff00");

    // The colour of a detected type shows; another colour is kept for the type
    panel.set_detected_triggers(2, {{2, {{100, 4.0}, {200, 5.0}}}});
    CHECK(panel.form().get<std::string>("triggerColorType") == "4" && panel.form().get<std::string>("triggerColor") == "#00ff00");
    panel.form().set_value("triggerColorType", std::string("5"));
    panel.form().set_value("triggerColor", std::string("#123456"));
    CHECK(panel.colors().at(5.0) == "#123456" && channels.size() == 2);
    MemorySettingsStore saved;
    panel.save(saved, "app");
    CHECK(saved.read_string("app/TriggerDetectionView/ColorTypes", {}) == "4;5" && saved.read_int("app/TriggerDetectionView/ChannelIndex", -1) == 1);
    CHECK(saved.read_string("app/TriggerDetectionView/Colors/5", {}) == "#123456");
}

void test_artifact_and_averaging_panels()
{
    mne::FiffChInfo eog;
    eog.kind = FIFFV_EOG_CH;
    ArtifactPanel artifact({eog});
    std::vector<std::map<std::string, double>> thresholds;
    artifact.set_thresholds_callback([&thresholds](const std::map<std::string, double>& value) {
        thresholds.push_back(value);
    });
    artifact.form().set_value("artifactThresholdsSecond/eog", -4);
    CHECK(thresholds.size() == 1 && thresholds.back().at("eog") == 1e-4 && thresholds.back().at("Active") == 0.0);
    artifact.set_channels({});
    CHECK(thresholds.size() == 2 && thresholds.back().size() == 1 && !artifact.rejection_active());

    // A stored baseline outside the stored epoch window is moved into it, silently
    MemorySettingsStore store;
    store.set_value("app/AveragingSettingsView/preStimSeconds", 50);
    store.set_value("app/AveragingSettingsView/postStimSeconds", 200);
    store.set_value("app/AveragingSettingsView/baselineFromSeconds", -300);
    store.set_value("app/AveragingSettingsView/baselineToSeconds", 500);
    store.set_value("app/AveragingSettingsView/currentStimChannel", std::string("STI 101"));
    AveragingPanel averaging({{"STI 014", 1}, {"STI 101", 2}});
    int reports = 0;
    AveragingPanel::Callbacks callbacks;
    callbacks.baseline_to_changed = [&reports](int) {
        ++reports;
    };
    callbacks.baseline_from_changed = callbacks.baseline_to_changed;
    averaging.set_callbacks(std::move(callbacks));
    averaging.load(store, "app");
    CHECK(averaging.baseline_from() == -50 && averaging.baseline_to() == 0 && reports == 0);
    CHECK(averaging.stim_channel_index() == 2);
    // Shortening the window clamps the baseline and reports it
    averaging.form().set_value("baselineFromSeconds", -40);
    averaging.form().set_value("preStimSeconds", 20);
    CHECK(averaging.baseline_from() == -20 && reports == 2);
}

//=============================================================================================================

void test_filter_design_panel()
{
    FilterDesignPanel panel;
    CHECK(panel.id() == "FilterDesignView" && panel.figure());
    CHECK(panel.from() == 1.0 && panel.to() == 40.0 && panel.taps() == 128 && panel.channel_type() == "All");
    CHECK(panel.filter().filter_type().name() == "BPF" && panel.filter().sampling_frequency() == 600.0);

    std::vector<double> edges;
    int designs = 0;
    std::vector<std::string> types;
    FilterDesignPanel::Callbacks callbacks;
    callbacks.filter_changed = [&designs](const mne::FilterKernel&) {
        ++designs;
    };
    callbacks.from_changed = [&edges](double from) {
        edges.push_back(from);
    };
    callbacks.to_changed = [&edges](double to) {
        edges.push_back(to);
    };
    callbacks.channel_type_changed = [&types](const std::string& type) {
        types.push_back(type);
    };
    panel.set_callbacks(std::move(callbacks));

    // Programmatic setters are silent and clamp to the Nyquist frequency
    panel.set_sampling_rate(200.0);
    panel.set_max_taps(101);
    panel.set_to(150.0);
    panel.set_channel_type("EEG");
    CHECK(designs == 0 && types.empty() && panel.to() == 100.0 && panel.form().field(FilterDesignPanel::taps_key)->maximum == 100.0);
    CHECK(panel.taps() == 100 && panel.filter().filter_order() == 100 && panel.channel_type() == "EEG");

    // User edits design and report; the lower edge stays below the upper one
    panel.form().set_value(FilterDesignPanel::to_key, 30.0);
    CHECK(designs == 1 && (edges == std::vector<double>{1.0, 30.0}) && std::abs(panel.filter().lowpass_freq() - 30.0) < 1e-9);
    CHECK(panel.form().field(FilterDesignPanel::from_key)->maximum == 30.0);
    panel.form().set_value(FilterDesignPanel::channel_type_key, std::string("MEG"));
    CHECK((types == std::vector<std::string>{"MEG"}));

    // A low pass only has an upper edge
    panel.form().set_value(FilterDesignPanel::type_key, std::string("LPF"));
    CHECK(designs == 2 && panel.filter().filter_type().name() == "LPF" && std::abs(panel.filter().lowpass_freq() - 30.0) < 1e-9);
    CHECK(!panel.form().is_visible(FilterDesignPanel::from_key) && panel.form().is_visible(FilterDesignPanel::to_key));
    panel.form().set_value(FilterDesignPanel::type_key, std::string("BPF"));

    // The response figure lays itself out to the view and draws a curve and the filter's name
    FilterResponseFigure& figure = panel.response();
    CHECK(!figure.zoomable() && !figure.plot().magnitude_db().empty() && figure.plot().nyquist() == 100.0);
    figure.resize(320.0, 200.0);
    CHECK(figure.bounds().width == 320.0 && figure.bounds().height == 200.0);
    Canvas canvas(320.0, 200.0);
    figure.draw(canvas, ApproximateTextMetrics(), Font{});
    bool curve = false;
    bool title = false;
    for (const Command& command : canvas.display_list().commands) {
        if (const auto* path = std::get_if<PathShape>(&command.shape)) {
            curve = curve || path->points.size() > 10;
        } else if (const auto* text = std::get_if<TextShape>(&command.shape)) {
            title = title || text->text == panel.filter().name();
        }
    }
    CHECK(curve && title);

    // Coefficients round-trip through a file, and the plot exports as SVG and PNG
    const std::filesystem::path dir = std::filesystem::current_path() / "test_viz_settings_form_filter";
    std::filesystem::create_directories(dir);
    CHECK(panel.export_coefficients(dir / "designed.txt").has_value());
    CHECK(panel.export_plot(dir / "plot.svg").has_value() && std::filesystem::file_size(dir / "plot.svg") > 0);
    CHECK(panel.export_plot(dir / "plot.PNG", 200.0, 120.0).has_value() && std::filesystem::file_size(dir / "plot.PNG") > 0);
    panel.form().set_value(FilterDesignPanel::from_key, 5.0);
    const int before = designs;
    panel.form().set_value(FilterDesignPanel::load_filter_key, (dir / "designed.txt").string());
    CHECK(designs == before + 1 && std::abs(panel.from() - 1.0) < 1e-6 && std::abs(panel.to() - 30.0) < 1e-6);
    CHECK(!panel.load_filter(dir / "missing.txt").has_value());
    std::filesystem::remove_all(dir);

    // Settings keep the keys of the Qt view, the design method as its index, and restore silently
    MemorySettingsStore store;
    panel.form().set_value(FilterDesignPanel::method_key, std::string("Tschebyscheff"));
    panel.save(store, "app");
    CHECK(store.value("app/FilterDesignView/filterDesignMethod") == SettingValue(1));
    CHECK(store.value("app/FilterDesignView/filterTo") == SettingValue(30.0));
    CHECK(!store.contains("app/FilterDesignView/exportFilter"));
    FilterDesignPanel restored;
    int restoredDesigns = 0;
    FilterDesignPanel::Callbacks restoredCallbacks;
    restoredCallbacks.filter_changed = [&restoredDesigns](const mne::FilterKernel&) {
        ++restoredDesigns;
    };
    restored.set_callbacks(std::move(restoredCallbacks));
    restored.load(store, "app");
    CHECK(restoredDesigns == 0 && restored.to() == 30.0 && restored.channel_type() == "MEG");
    CHECK(restored.filter().design_method().name() == "Tschebyscheff");
}

//=============================================================================================================

void test_filter_settings_panel()
{
    FilterSettingsPanel panel;
    CHECK(panel.id() == "FilterSettingsView" && !panel.active() && panel.design());
    FilterDesignPanel& design = *panel.design();
    CHECK(panel.form().get<double>(FilterSettingsPanel::from_key) == design.from());

    bool activated = false;
    int requests = 0;
    FilterSettingsPanel::Callbacks callbacks;
    callbacks.activation_changed = [&activated](bool on) {
        activated = on;
    };
    callbacks.show_design = [&requests]() {
        ++requests;
    };
    panel.set_callbacks(std::move(callbacks));
    int designs = 0;
    std::vector<std::string> types;
    FilterDesignPanel::Callbacks designCallbacks;
    designCallbacks.filter_changed = [&designs](const mne::FilterKernel&) {
        ++designs;
    };
    designCallbacks.channel_type_changed = [&types](const std::string& type) {
        types.push_back(type);
    };
    design.set_callbacks(std::move(designCallbacks));

    panel.set_sampling_rate(500.0);
    CHECK(designs == 0 && panel.form().field(FilterSettingsPanel::to_key)->maximum == 250.0);

    // Edits here are edits of the design, which reports them
    panel.form().set_value(FilterSettingsPanel::to_key, 70.0);
    panel.form().set_value(FilterSettingsPanel::from_key, 3.0);
    CHECK(designs == 2 && design.from() == 3.0 && design.to() == 70.0);
    panel.form().set_value(FilterSettingsPanel::channel_type_key, std::string("EOG"));
    CHECK((types == std::vector<std::string>{"EOG"}) && design.channel_type() == "EOG");

    // Edits of the design show here
    design.form().set_value(FilterDesignPanel::to_key, 60.0);
    design.set_from(8.0);
    CHECK(panel.form().get<double>(FilterSettingsPanel::to_key) == 60.0 && panel.form().get<double>(FilterSettingsPanel::from_key) == 8.0);

    panel.form().set_value(FilterSettingsPanel::active_key, true);
    panel.form().trigger(FilterSettingsPanel::show_design_key);
    CHECK(activated && panel.active() && requests == 1);

    // The clinical mode hides the design action
    CHECK(panel.form().is_visible(FilterSettingsPanel::show_design_key));
    panel.set_gui_mode(Panel::GuiMode::Clinical);
    CHECK(!panel.form().is_visible(FilterSettingsPanel::show_design_key));

    // Settings include the design's and restore silently
    MemorySettingsStore store;
    panel.save(store, "app");
    CHECK(store.value("app/FilterSettingsView/filterActivated") == SettingValue(true));
    CHECK(store.value("app/FilterDesignView/filterFrom") == SettingValue(8.0));
    FilterSettingsPanel restored;
    bool reported = false;
    FilterSettingsPanel::Callbacks restoredCallbacks;
    restoredCallbacks.activation_changed = [&reported](bool) {
        reported = true;
    };
    restored.set_callbacks(std::move(restoredCallbacks));
    restored.load(store, "app");
    CHECK(!reported && restored.active() && restored.design()->from() == 8.0 && restored.design()->channel_type() == "EOG");
    CHECK(restored.form().get<double>(FilterSettingsPanel::to_key) == 60.0);
}

} // namespace

//=============================================================================================================
// MAIN
//=============================================================================================================

int main()
{
    const std::vector<std::pair<const char*, std::function<void()>>> tests = {
        {"fields_and_values", test_fields_and_values},
        {"conditions", test_conditions},
        {"flags", test_flags},
        {"listeners", test_listeners},
        {"info_paths_colors_and_removal", test_info_paths_colors_and_removal},
        {"compensator_panel", test_compensator_panel},
        {"projectors_panel", test_projectors_panel},
        {"forward_panel", test_forward_panel},
        {"processing_panels", test_processing_panels},
        {"connectivity_panel", test_connectivity_panel},
        {"trigger_detection_panel", test_trigger_detection_panel},
        {"artifact_and_averaging_panels", test_artifact_and_averaging_panels},
        {"filter_design_panel", test_filter_design_panel},
        {"filter_settings_panel", test_filter_settings_panel},
    };
    for (const auto& [name, test] : tests) {
        const int before = g_failures;
        test();
        std::printf("%s %s\n", g_failures == before ? "PASS" : "FAIL", name);
    }
    std::printf("%d failure(s)\n", g_failures);
    return g_failures == 0 ? 0 : 1;
}
