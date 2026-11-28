//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     averaging_panel.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    AveragingPanel class definition.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "averaging_panel.h"
#include "../host/settings_store.h"
#include "../models/item_list.h"
#include <mne/fiff/fiff_evoked_set.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <memory>
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

constexpr const char* kOffline = "offline";
constexpr const char* kHasTrials = "hasDetectedTrials";
constexpr const char* kStimChannel = "currentStimChannel";
constexpr const char* kNumAverages = "numAverages";
constexpr const char* kPreStim = "preStimSeconds";
constexpr const char* kPostStim = "postStimSeconds";
constexpr const char* kBaselineActive = "doBaselineCorrection";
constexpr const char* kBaselineFrom = "baselineFromSeconds";
constexpr const char* kBaselineTo = "baselineToSeconds";
constexpr const char* kTrials = "detectedTrials";
constexpr const char* kReset = "reset";
constexpr const char* kDropRejected = "dropRejected";
constexpr const char* kAutoCompute = "autoCompute";
constexpr const char* kCompute = "compute";
constexpr int kMostMilliseconds = 10000;
constexpr int kMostShownAverages = 10;

/** A field in milliseconds that reports when editing finishes. */
SettingField milliseconds(const char* key, const char* label, int value, int minimum, const char* group, const char* prefix = "")
{
    SettingField field;
    field.key = key;
    field.kind = SettingField::Kind::Int;
    field.label = label;
    field.default_value = value;
    field.minimum = minimum;
    field.maximum = kMostMilliseconds;
    field.step = 10;
    field.decimals = 0;
    field.prefix = prefix;
    field.unit = " ms";
    field.group = group;
    field.commit = SettingField::Commit::WhenFinished;
    return field;
}

/** Calls @p callback with @p value when set. */
template<typename T>
void report(const std::function<void(T)>& callback, T value)
{
    if (callback) {
        callback(value);
    }
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

AveragingPanel::AveragingPanel(const std::map<std::string, int>& stimChannels)
: Panel("AveragingSettingsView", "Averaging Settings")
, m_stimChannels(stimChannels)
{
    SettingsForm& f = form();
    f.add_flag(kOffline);
    f.add_flag(kHasTrials);
    constexpr const char* general = "General";
    SettingField stimChannel;
    stimChannel.key = kStimChannel;
    stimChannel.kind = SettingField::Kind::Choice;
    stimChannel.label = "Stimulus channel";
    stimChannel.group = general;
    stimChannel.visible_if = "!offline";
    // The stored channel may be unknown until the stimulus channels arrive, so the panel keeps it aside
    stimChannel.persistent = false;
    f.add(std::move(stimChannel));
    SettingField numAverages;
    numAverages.key = kNumAverages;
    numAverages.kind = SettingField::Kind::Int;
    numAverages.label = "Number of trials";
    numAverages.default_value = 10;
    numAverages.minimum = 1;
    numAverages.maximum = 1000;
    numAverages.decimals = 0;
    numAverages.group = general;
    numAverages.visible_if = "!offline";
    numAverages.commit = SettingField::Commit::WhenFinished;
    f.add(std::move(numAverages));
    f.add(milliseconds(kPreStim, "Pre-stimulus", 100, 0, general, "-"));
    f.add(milliseconds(kPostStim, "Post-stimulus", 400, 10, general));
    constexpr const char* baseline = "Baseline correction";
    f.add_bool(kBaselineActive, "Activate", false, baseline);
    f.add(milliseconds(kBaselineFrom, "Baseline min", 0, -kMostMilliseconds, baseline));
    f.add(milliseconds(kBaselineTo, "Baseline max", 0, -kMostMilliseconds, baseline));
    add_list(kTrials, {}, std::make_shared<ItemList>(), "Detected Trials");
    SettingField trials = *f.field(kTrials);
    trials.visible_if = kHasTrials;
    f.remove(kTrials);
    f.add(std::move(trials));
    SettingField reset;
    reset.key = kReset;
    reset.kind = SettingField::Kind::Action;
    reset.label = "Reset";
    reset.visible_if = "!offline";
    f.add(std::move(reset));
    SettingField dropRejected;
    dropRejected.key = kDropRejected;
    dropRejected.kind = SettingField::Kind::Bool;
    dropRejected.label = "Drop rejected";
    dropRejected.default_value = false;
    dropRejected.visible_if = kOffline;
    dropRejected.persistent = false;
    SettingField autoCompute = dropRejected;
    autoCompute.key = kAutoCompute;
    autoCompute.label = "Live average based on events";
    f.add(std::move(dropRejected));
    f.add(std::move(autoCompute));
    SettingField compute;
    compute.key = kCompute;
    compute.kind = SettingField::Kind::Action;
    compute.label = "Save";
    compute.visible_if = kOffline;
    f.add(std::move(compute));

    m_listener = f.add_listener([this](const std::string& key, const std::optional<SettingValue>&) {
        if (!m_updating) {
            changed(key);
        }
    });
    const bool updating = std::exchange(m_updating, true);
    f.set_range(kBaselineFrom, -pre_stim(), post_stim());
    f.set_range(kBaselineTo, -pre_stim(), post_stim());
    m_updating = updating;
    fill_stim_channels();
}

//=============================================================================================================

AveragingPanel::~AveragingPanel()
{
    form().remove_listener(m_listener);
}

//=============================================================================================================

void AveragingPanel::set_stim_channels(const std::map<std::string, int>& stimChannels)
{
    if (!stimChannels.empty()) {
        m_stimChannels = stimChannels;
        fill_stim_channels();
    }
}

//=============================================================================================================

void AveragingPanel::fill_stim_channels()
{
    std::vector<SettingChoice> choices;
    choices.reserve(m_stimChannels.size());
    for (const auto& [name, index] : m_stimChannels) {
        choices.push_back({name, name});
    }
    const bool updating = std::exchange(m_updating, true);
    form().set_choices(kStimChannel, std::move(choices));
    form().set_value(kStimChannel, m_stimChannel);
    m_updating = updating;
}

//=============================================================================================================

void AveragingPanel::set_detected_epochs(const mne::FiffEvokedSet& evokedSet)
{
    form().set_value(kHasTrials, !evokedSet.evoked.empty());
    if (evokedSet.evoked.empty()) {
        return;
    }
    const std::size_t count = std::min<std::size_t>(evokedSet.evoked.size(), kMostShownAverages);
    std::vector<std::string> items;
    items.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        items.push_back(evokedSet.evoked[i].comment + ": " + std::to_string(evokedSet.evoked[i].nave));
    }
    list(kTrials)->set_items(std::move(items));
}

//=============================================================================================================

int AveragingPanel::stim_channel_index() const
{
    const auto it = m_stimChannels.find(form().get<std::string>(kStimChannel));
    return it == m_stimChannels.end() ? 0 : it->second;
}

//=============================================================================================================

bool AveragingPanel::baseline_correction() const
{
    return form().get<bool>(kBaselineActive);
}

//=============================================================================================================

int AveragingPanel::num_averages() const
{
    return form().get<int>(kNumAverages);
}

//=============================================================================================================

int AveragingPanel::baseline_from() const
{
    return form().get<int>(kBaselineFrom);
}

//=============================================================================================================

int AveragingPanel::baseline_to() const
{
    return form().get<int>(kBaselineTo);
}

//=============================================================================================================

int AveragingPanel::pre_stim() const
{
    return form().get<int>(kPreStim);
}

//=============================================================================================================

int AveragingPanel::post_stim() const
{
    return form().get<int>(kPostStim);
}

//=============================================================================================================

bool AveragingPanel::auto_compute() const
{
    return form().get<bool>(kAutoCompute);
}

//=============================================================================================================

void AveragingPanel::load(const SettingsStore& store, const std::string& path)
{
    if (path.empty()) {
        return;
    }
    const bool updating = std::exchange(m_updating, true);
    // The stored baseline is checked against the stored epoch window, not against the current one
    form().set_range(kBaselineFrom, -kMostMilliseconds, kMostMilliseconds);
    form().set_range(kBaselineTo, -kMostMilliseconds, kMostMilliseconds);
    Panel::load(store, path);
    const int preStim = pre_stim();
    const int postStim = post_stim();
    if (const int from = baseline_from(); from < -preStim || from > postStim) {
        form().set_value(kBaselineFrom, -preStim);
    }
    if (const int to = baseline_to(); to > postStim || to < -preStim) {
        form().set_value(kBaselineTo, 0);
    }
    form().set_range(kBaselineFrom, -preStim, postStim);
    form().set_range(kBaselineTo, -preStim, postStim);
    m_stimChannel = store.read_string(settings_key(path, kStimChannel), m_stimChannel);
    m_updating = updating;
    fill_stim_channels();
}

//=============================================================================================================

void AveragingPanel::save(SettingsStore& store, const std::string& path) const
{
    if (path.empty()) {
        return;
    }
    Panel::save(store, path);
    store.set_value(settings_key(path, kStimChannel), m_stimChannel);
}

//=============================================================================================================

void AveragingPanel::processing_mode_changed()
{
    form().set_value(kOffline, processing_mode() == ProcessingMode::Offline);
}

//=============================================================================================================

void AveragingPanel::changed(const std::string& key)
{
    if (key == kBaselineActive) {
        report(m_callbacks.baseline_active_changed, baseline_correction());
    } else if (key == kDropRejected) {
        report(m_callbacks.drop_rejected_changed, form().get<bool>(kDropRejected));
    } else if (key == kAutoCompute) {
        report(m_callbacks.auto_compute_changed, auto_compute());
    } else if (key == kReset) {
        if (m_callbacks.reset) {
            m_callbacks.reset();
        }
    } else if (key == kCompute) {
        if (m_callbacks.compute) {
            m_callbacks.compute();
        }
    } else if (key == kStimChannel) {
        m_stimChannel = form().get<std::string>(kStimChannel);
        if (m_callbacks.stim_channel_changed) {
            m_callbacks.stim_channel_changed(m_stimChannel);
        }
    } else if (key == kNumAverages) {
        report(m_callbacks.num_averages_changed, num_averages());
    } else if (key == kPreStim || key == kPostStim || key == kBaselineFrom || key == kBaselineTo) {
        // The epoch window bounds the baseline, the baseline bounds itself; clamped values report too
        const SettingField* from = form().field(kBaselineFrom);
        const SettingField* to = form().field(kBaselineTo);
        const double fromMaximum = from->maximum;
        const double toMinimum = to->minimum;
        const double toMaximum = to->maximum;
        const double fromMinimum = from->minimum;
        if (key == kPreStim) {
            form().set_range(kBaselineTo, -pre_stim(), toMaximum);
            form().set_range(kBaselineFrom, -pre_stim(), fromMaximum);
        } else if (key == kPostStim) {
            form().set_range(kBaselineTo, toMinimum, post_stim());
            form().set_range(kBaselineFrom, fromMinimum, post_stim());
        } else if (key == kBaselineFrom) {
            form().set_range(kBaselineTo, baseline_from(), toMaximum);
        } else {
            form().set_range(kBaselineFrom, fromMinimum, baseline_to());
        }
        const std::map<std::string, const std::function<void(int)>*> callbacks = {
            {kPreStim, &m_callbacks.pre_stim_changed},
            {kPostStim, &m_callbacks.post_stim_changed},
            {kBaselineFrom, &m_callbacks.baseline_from_changed},
            {kBaselineTo, &m_callbacks.baseline_to_changed}};
        report(*callbacks.at(key), form().get<int>(key));
    }
}
