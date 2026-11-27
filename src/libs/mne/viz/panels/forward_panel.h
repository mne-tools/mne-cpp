//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     forward_panel.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    ForwardPanel: compute, recompute and cluster the forward solution of real-time source imaging.
 */

#ifndef VIZ_FORWARD_PANEL_H
#define VIZ_FORWARD_PANEL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../viz_global.h"
#include "panel.h"
#include <mne/fiff/fiff_types.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <filesystem>
#include <functional>
#include <memory>
#include <string>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace mne
{
class FsAnnotationSet;
} // namespace mne

//=============================================================================================================
// DEFINE NAMESPACE mne::viz
//=============================================================================================================

namespace mne::viz
{

//=============================================================================================================
/**
 * @brief Controls and status of the forward solution: computation, head position updates, clustering with a brain
 * atlas, and a summary of the solution.
 *
 * Clustering needs the a2009s annotations of both hemispheres in the atlas directory; without them the clustering
 * switch turns off again and the atlas status asks for them.
 *
 * @since 3.0.0
 */
class VIZSHARED_EXPORT ForwardPanel : public Panel
{
public:
    /** State of the computation, as reported by the forward plugin. */
    enum class Status
    {
        Initializing = 0, /**< Preparing the computation. */
        Computing = 1,    /**< Computing the solution. */
        Recomputing = 2,  /**< Recomputing for a new head position. */
        Clustering = 3,   /**< Clustering the solution. */
        NotComputed = 4,  /**< No solution yet. */
        NotClustered = 5, /**< A solution without clustering. */
        Finished = 6      /**< Done. */
    };

    /** What the panel reports to its owner. */
    struct Callbacks
    {
        std::function<void()> compute;                            /**< "Compute Forward Solution" was clicked. */
        std::function<void(bool on)> recomputation_changed;       /**< Head position updates were switched. */
        std::function<void(bool on)> clustering_changed;          /**< Clustering was switched with an atlas loaded. */
        std::function<void(int sources)> cluster_sources_changed; /**< The number of sources per cluster changed. */
        std::function<void(const std::filesystem::path& directory,
                           const std::shared_ptr<mne::FsAnnotationSet>& annotations)>
            atlas_changed; /**< An atlas directory with both annotations was chosen. */
    };

    //=========================================================================================================
    /**
     * @param[in] atlasDirectory   Directory with the a2009s annotations loaded at start, e.g. the sample subject's
     *                             label directory; empty for none.
     */
    explicit ForwardPanel(const std::filesystem::path& atlasDirectory = {});

    ~ForwardPanel() override;

    /** @param[in] callbacks   What the panel reports. */
    void set_callbacks(Callbacks callbacks)
    {
        m_callbacks = std::move(callbacks);
    }

    //=========================================================================================================
    /**
     * Loads the a2009s annotations of an atlas directory and reports them when both hemispheres load.
     *
     * @param[in] directory   Atlas directory.
     * @param[in] notify      Whether to call Callbacks::atlas_changed.
     *
     * @return Whether both annotations loaded.
     */
    bool set_atlas_directory(const std::filesystem::path& directory, bool notify = true);

    /** @return Whether the atlas annotations are loaded. */
    bool atlas_loaded() const
    {
        return m_atlasLoaded;
    }

    //=========================================================================================================
    /**
     * Shows the state of the computation.
     *
     * @param[in] status   State; values beyond Finished show as Finished.
     */
    void set_status(int status);

    //=========================================================================================================
    /**
     * Shows a summary of a computed solution.
     *
     * @param[in] sourceOri    Source orientation: 0 fixed, else free.
     * @param[in] coordFrame   Coordinate frame (FIFFV_COORD_*).
     * @param[in] sources      Number of sources.
     * @param[in] channels     Number of channels.
     * @param[in] spaces       Number of source spaces.
     */
    void set_solution_information(mne::fiff_int_t sourceOri, mne::fiff_int_t coordFrame, int sources, int channels, int spaces);

    //=========================================================================================================
    /**
     * Shows the number of sources after clustering.
     *
     * @param[in] sources   Number of clustered sources.
     */
    void set_clustered_information(int sources);

    /** @return Whether head position updates recompute the solution. */
    bool recomputation() const;

    /** @return Whether the solution is clustered. */
    bool clustering() const;

    /** @return The number of sources per cluster. */
    int cluster_sources() const;

private:
    void changed(const std::string& key);

    Callbacks m_callbacks;
    bool m_atlasLoaded = false;
    bool m_updating = false;
    int m_listener = 0;
};

} // namespace mne::viz

#endif // VIZ_FORWARD_PANEL_H
