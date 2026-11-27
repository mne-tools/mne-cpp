//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     forward_panel.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    ForwardPanel class definition.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "forward_panel.h"
#include <mne/base/string_utils.h>
#include <mne/fiff/fiff_constants.h>
#include <mne/fs/fs_annotationset.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <array>
#include <utility>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::viz;

//=============================================================================================================
// DEFINE STATIC FUNCTIONS
//=============================================================================================================

namespace
{

constexpr const char* kCompute = "compute";
constexpr const char* kRecompute = "recompute";
constexpr const char* kCluster = "cluster";
constexpr const char* kClusterSources = "clusterSources";
constexpr const char* kStatus = "status";
constexpr const char* kAtlasDirectory = "atlasDirectory";
constexpr const char* kAtlasStatus = "atlasStatus";

const std::string kComputation = "Forward Solution Computation";
const std::string kClustering = "Clustering";
const std::string kInformation = "Forward Solution Information";

SettingField unsaved(SettingField field)
{
    // The forward plugin decides afresh each session
    field.persistent = false;
    return field;
}

SettingField make(const char* key, SettingField::Kind kind, const std::string& label, SettingValue value, const std::string& group)
{
    SettingField field;
    field.key = key;
    field.kind = kind;
    field.label = label;
    field.default_value = std::move(value);
    field.group = group;
    return unsaved(std::move(field));
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ForwardPanel::ForwardPanel(const std::filesystem::path& atlasDirectory)
: Panel("FwdSettingsView", "Forward Solution")
{
    SettingsForm& f = form();
    f.add(make(kCompute, SettingField::Kind::Action, "Compute Forward Solution", false, kComputation));
    f.add(make(kRecompute, SettingField::Kind::Bool, "Enable head position update", false, kComputation));
    f.add(make(kCluster, SettingField::Kind::Bool, "Cluster forward solution", true, kComputation));
    SettingField sources = make(kClusterSources, SettingField::Kind::Int, "Number of sources per cluster", 200, kComputation);
    sources.minimum = 5;
    sources.maximum = 999;
    sources.commit = SettingField::Commit::WhenFinished;
    f.add(std::move(sources));
    f.add(make(kStatus, SettingField::Kind::Info, "Status computation:", std::string("Not Computed"), kComputation));
    f.set_tone(kStatus, SettingField::Tone::Negative);

    f.add(make(kAtlasDirectory, SettingField::Kind::Directory, "Brain atlas directory", utf8_from_path(atlasDirectory), kClustering));
    f.add(make(kAtlasStatus, SettingField::Kind::Info, "(lh+rh).aparc.a2009s.annot", std::string("not loaded"), kClustering));

    f.add(make("sourceSpaces", SettingField::Kind::Info, "Source spaces:", std::string("0"), kInformation));
    f.add(make("sources", SettingField::Kind::Info, "Number of sources:", std::string("0"), kInformation));
    f.add(make("clusteredSources", SettingField::Kind::Info, "Number of clustered sources:", std::string("Not Clustered"), kInformation));
    f.add(make("sourceOrientation", SettingField::Kind::Info, "Source orientation:", std::string("fixed"), kInformation));
    f.add(make("channels", SettingField::Kind::Info, "Number of channels:", std::string("0"), kInformation));
    f.add(make("coordinateFrame", SettingField::Kind::Info, "Coordinate frame:", std::string("Head Space"), kInformation));

    // Nothing can listen yet, so the start atlas is not reported
    if (!atlasDirectory.empty()) {
        set_atlas_directory(atlasDirectory, false);
    }

    m_listener = f.add_listener([this](const std::string& key, const std::optional<SettingValue>& value) {
        if (!m_updating && (value || key == kCompute)) {
            changed(key);
        }
    });
}

//=============================================================================================================

ForwardPanel::~ForwardPanel()
{
    form().remove_listener(m_listener);
}

//=============================================================================================================

void ForwardPanel::changed(const std::string& key)
{
    if (key == kCompute) {
        if (m_callbacks.compute) {
            m_callbacks.compute();
        }
    } else if (key == kRecompute) {
        if (m_callbacks.recomputation_changed) {
            m_callbacks.recomputation_changed(recomputation());
        }
    } else if (key == kCluster) {
        // Clustering needs the atlas
        if (!m_atlasLoaded && clustering()) {
            m_updating = true;
            form().set_value(kCluster, false);
            form().set_value(kAtlasStatus, std::string("not loaded: load an annotation set before clustering"));
            form().set_tone(kAtlasStatus, SettingField::Tone::Warning);
            m_updating = false;
            return;
        }
        if (m_callbacks.clustering_changed) {
            m_callbacks.clustering_changed(clustering());
        }
    } else if (key == kClusterSources) {
        if (m_callbacks.cluster_sources_changed) {
            m_callbacks.cluster_sources_changed(cluster_sources());
        }
    } else if (key == kAtlasDirectory) {
        set_atlas_directory(path_from_utf8(form().get<std::string>(kAtlasDirectory)));
    }
}

//=============================================================================================================

bool ForwardPanel::set_atlas_directory(const std::filesystem::path& directory, bool notify)
{
    m_updating = true;
    form().set_value(kAtlasDirectory, utf8_from_path(directory));
    m_updating = false;
    auto annotations = std::make_shared<mne::FsAnnotationSet>(directory / "lh.aparc.a2009s.annot", directory / "rh.aparc.a2009s.annot");
    m_atlasLoaded = !annotations->empty() && annotations->size() == 2;
    m_updating = true;
    form().set_value(kAtlasStatus, std::string(m_atlasLoaded ? "loaded" : "not loaded"));
    form().set_tone(kAtlasStatus, m_atlasLoaded ? SettingField::Tone::Positive : SettingField::Tone::Normal);
    m_updating = false;
    if (m_atlasLoaded && notify && m_callbacks.atlas_changed) {
        m_callbacks.atlas_changed(directory, annotations);
    }
    return m_atlasLoaded;
}

//=============================================================================================================

void ForwardPanel::set_status(int status)
{
    static const std::array<const char*, 7> kTexts = {"Initializing", "Computing", "Recomputing", "Clustering", "Not Computed", "Not Clustered", "Finished"};
    const int index = (status >= 0 && status < static_cast<int>(kTexts.size())) ? status : static_cast<int>(Status::Finished);
    SettingField::Tone tone = SettingField::Tone::Negative;
    if (index == static_cast<int>(Status::NotClustered)) {
        tone = SettingField::Tone::Warning;
    } else if (index == static_cast<int>(Status::Finished)) {
        tone = SettingField::Tone::Positive;
    }
    m_updating = true;
    form().set_value(kStatus, std::string(kTexts[static_cast<std::size_t>(index)]));
    form().set_tone(kStatus, tone);
    m_updating = false;
}

//=============================================================================================================

void ForwardPanel::set_solution_information(mne::fiff_int_t sourceOri, mne::fiff_int_t coordFrame, int sources, int channels, int spaces)
{
    std::string frame = "Unknown";
    if (coordFrame == FIFFV_COORD_HEAD) {
        frame = "Head Space";
    } else if (coordFrame == FIFFV_COORD_MRI) {
        frame = "MRI Space";
    }
    m_updating = true;
    form().set_value("sourceOrientation", std::string(sourceOri == 0 ? "fixed" : "free"));
    form().set_value("coordinateFrame", frame);
    form().set_value("sources", std::to_string(sources));
    form().set_value("clusteredSources", std::string("Not clustered"));
    form().set_value("channels", std::to_string(channels));
    form().set_value("sourceSpaces", std::to_string(spaces));
    m_updating = false;
}

//=============================================================================================================

void ForwardPanel::set_clustered_information(int sources)
{
    m_updating = true;
    form().set_value("clusteredSources", std::to_string(sources));
    m_updating = false;
}

//=============================================================================================================

bool ForwardPanel::recomputation() const
{
    return form().get<bool>(kRecompute);
}

//=============================================================================================================

bool ForwardPanel::clustering() const
{
    return form().get<bool>(kCluster);
}

//=============================================================================================================

int ForwardPanel::cluster_sources() const
{
    return form().get<int>(kClusterSources);
}
