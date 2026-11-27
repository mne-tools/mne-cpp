//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     brainframecomposer.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     May 2026
 * @brief    BrainFrameComposer: the frames of the brain view from its scene, for any GPU backend.
 */

#ifndef VIZ_BRAINFRAMECOMPOSER_H
#define VIZ_BRAINFRAMECOMPOSER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../viz_global.h"
#include "rendertypes.h"
#include "viewstate.h"
#include "../gpu/brain_frame.h"
#include "../views/camera.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <array>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace mne::viz
{
class BrainSurface;
class DipoleObject;
class ElectrodeObject;
class NetworkObject;
class PolylineObject;
} // namespace mne::viz

//=============================================================================================================
// DEFINE NAMESPACE mne::viz
//=============================================================================================================

namespace mne::viz
{

/**
 * @brief An MRI slice drawn in the brain view.
 *
 * @since 3.0.0
 */
struct BrainSliceLayer
{
    BrainTextureView image;          /**< Slice image; the red channel is the intensity. */
    std::array<float, 20> quad = {}; /**< Four world corners, each position (3 floats) and texture coordinates (2 floats), as a strip. */
    float opacity = 1.f;             /**< Opacity. */
    float window_center = 0.5f;      /**< Intensity window centre, 0 to 1. */
    float window_width = 1.f;        /**< Intensity window width, 0 to 1. */
};

/**
 * @brief A live video frame drawn on the head surface, or as a disk facing the camera without one.
 *
 * @since 3.0.0
 */
struct BrainVideoLayer
{
    BrainTextureView frame;                            /**< Video frame. */
    BrainTextureView depth;                            /**< Depth frame the decal follows, or none for a flat decal. */
    Eigen::Vector3f focus = Eigen::Vector3f::Zero();   /**< Centre of the frame in world coordinates. */
    float size = 0.05f;                                /**< Width of the frame in metres. */
    Eigen::Vector3f up_hint = Eigen::Vector3f::Zero(); /**< Up direction of the frame, or zero to derive one. */
    float opacity = 1.f;                               /**< Opacity. */
    float depth_scale = 0.f;                           /**< Displacement of the relief per depth unit. */
    int depth_steps = 0;                               /**< Quantisation steps of the relief. */
};

/**
 * @brief Everything the brain view draws, shared by all panes; the pointed objects must live while a frame is composed.
 *
 * @since 3.0.0
 */
struct BrainScene
{
    const std::map<std::string, std::shared_ptr<BrainSurface>>* surfaces = nullptr; /**< Surfaces by key, e.g. "lh_pial", "bem_head". */
    std::string meg_field_key;                                                      /**< Surface showing the MEG field map. */
    std::string eeg_field_key;                                                      /**< Surface showing the EEG field map. */
    std::vector<const DipoleObject*> dipoles;                                       /**< Dipole sets. */
    const ElectrodeObject* electrodes = nullptr;                                    /**< Intracranial electrodes, or none. */
    const NetworkObject* network = nullptr;                                         /**< Connectivity network, or none. */
    const PolylineObject* head_path = nullptr;                                      /**< Head movement path, or none. */
    const BrainSurface* pointer = nullptr;                                          /**< Marker drawn at pointer_position, or none. */
    Eigen::Vector3f pointer_position = Eigen::Vector3f::Zero();                     /**< Position of the pointer marker. */
    std::vector<BrainSliceLayer> slices;                                            /**< Visible MRI slices. */
    std::optional<BrainVideoLayer> video;                                           /**< Video overlay with a frame, or none. */
    bool lighting = true;                                                           /**< Whether surfaces and glyphs are lit. */
};

/**
 * @brief A pane of the brain view: where it draws, from which camera, and what its view shows.
 *
 * @since 3.0.0
 */
struct BrainScenePane
{
    GpuRect viewport;              /**< Pane in target pixels, top-left origin. */
    CameraMatrices camera;         /**< Camera of the pane. */
    const SubView* view = nullptr; /**< Surface type, shaders, overlay mode and visibility of the pane. */
};

/**
 * @brief Describes the frames of the brain view as GpuFrame: what each pane draws in which order with which shader.
 *
 * Brain surfaces of the pane's surface type are drawn first, then the other opaque surfaces (source spaces,
 * digitizers), electrodes and MRI slices, the translucent surfaces (BEM, sensors, digitizer markers) back to front
 * with the video decal after its head surface, then dipoles, the network, the head path and the pointer marker.
 * The GPU copies of surfaces and glyphs are dropped once their owners are destroyed.
 *
 * @since 3.0.0
 */
class VIZSHARED_EXPORT BrainFrameComposer
{
public:
    //=========================================================================================================
    /**
     * Starts a frame.
     *
     * @param[in] target   Size, pixel ratio and clip-space correction of the target.
     * @param[in] clear    Colour the frame clears to.
     */
    void begin(const GpuFrameTarget& target, Color clear = Color::black());

    //=========================================================================================================
    /**
     * Draws the scene into the panes.
     *
     * @param[in] scene   What to draw.
     * @param[in] panes   Panes in drawing order.
     *
     * @return For each pane, the keys of the brain surfaces of its surface type it shows.
     */
    std::vector<std::vector<std::string>> draw(const BrainScene& scene, std::span<const BrainScenePane> panes);

    //=========================================================================================================
    /**
     * Ends the frame, blending a screen overlay (labels, pane separators) over all panes.
     *
     * @param[in] overlay   RGBA pixels of the target size or a figure of the logical size, or none.
     * @param[in] scale     Texture pixels per figure pixel of a figure.
     *
     * @return The frame, referring to resources().
     */
    GpuFrame finish(std::optional<std::variant<Image, DisplayList>> overlay = std::nullopt, double scale = 1.0);

    /** @return Buffers, textures and pipelines of the frames. */
    const GpuResources& resources() const
    {
        return m_builder.resources();
    }

    /** @return The number of meshes with buffers. */
    int mesh_count() const
    {
        return m_builder.mesh_count();
    }

private:
    std::uint64_t key_of(const RenderKey& key);
    void draw_pane(const BrainScene& scene, const BrainScenePane& pane, std::size_t index, std::vector<std::string>& drawn);

    struct Key
    {
        std::weak_ptr<const void> owner; /**< Expires with the renderable. */
        std::uint64_t id = 0;            /**< Key of its buffers in the builder. */
    };

    BrainFrameBuilder m_builder;
    std::unordered_map<const void*, Key> m_keys; /**< Builder keys of renderables by RenderKey::id(). */
    std::uint64_t m_nextKey = 1;
    Eigen::Matrix4f m_clipCorrection = Eigen::Matrix4f::Identity();
};

} // namespace mne::viz

#endif // VIZ_BRAINFRAMECOMPOSER_H
