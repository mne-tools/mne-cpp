//=============================================================================================================
/**
 * @file     mri_slices.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided
 * that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and
 *       the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions
 *       and the following disclaimer in the documentation and/or other materials provided with the
 *       distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors may be used to endorse or
 *       promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * @brief    MriSlicesPlugin — mne_inspect plugin that loads a 3-D MRI volume
 *           (MGH / MGZ via MriVolData) and publishes axial, coronal and
 *           sagittal ortho slices into a MultimodalScene. Supports a shared
 *           crosshair that follows scene picks.
 *
 *           NOTE: mne_inspect does not yet ship a plugin loader. The plugin
 *           is built as a standalone static library; see plugins/README.md
 *           for the host-side wiring still needed.
 *
 */

#ifndef MNEINSPECT_MRI_SLICES_PLUGIN_H
#define MNEINSPECT_MRI_SLICES_PLUGIN_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <disp3D/renderable/sliceobject.h>
#include <disp3D/scene/pickresult.h>

#include <mri/mri_vol_data.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QObject>
#include <QString>
#include <QVector3D>

#include <array>
#include <memory>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace DISP3DLIB {
    class MultimodalScene;
}

namespace MRISLICESPLUGIN
{

//=============================================================================================================
/**
 * @brief mne_inspect plugin: load an MRI volume and render ortho slices.
 *
 * Responsibilities:
 *   - Load a 3-D volume from MGH / MGZ via `MRILIB::MriVolData::read`. NIfTI
 *     is not yet wired (the codebase ships only MGH/MGZ IO at v2.3.0); see
 *     plugins/README.md for the open question.
 *   - Maintain a crosshair in MRI RAS coordinates and (re-)materialise three
 *     `DISP3DLIB::SliceObject` payloads (axial, coronal, sagittal) that
 *     intersect that crosshair.
 *   - Register / refresh the three layers on a `MultimodalScene` under the
 *     ids `mri_axial`, `mri_coronal`, `mri_sagittal`.
 *   - When the scene reports a pick whose `kind == MriVoxel`, snap the
 *     crosshair to that hit point so the other ortho views follow.
 *
 * The plugin owns the `MriVolData` and the three `SliceObject` payloads; it
 * carries no GUI or QRhi state of its own.
 */
class MriSlicesPlugin : public QObject
{
    Q_OBJECT

public:
    explicit MriSlicesPlugin(QObject* parent = nullptr);
    ~MriSlicesPlugin() override;

    //=========================================================================================================
    /**
     * Load an MRI volume from disk.
     *
     * Supported formats: FreeSurfer MGH/MGZ (.mgh, .mgz). NIfTI loading is
     * not yet hooked up — see plugins/README.md.
     *
     * On success the crosshair is reset to the volume centre and the three
     * ortho `SliceObject`s are rebuilt accordingly.
     *
     * @param[in] path   Path to the volume file.
     * @return true on success.
     */
    bool loadVolume(const QString& path);

    //=========================================================================================================
    /**
     * @return Path of the most recent successful load. Empty if none.
     */
    QString sourcePath() const { return m_sourcePath; }

    //=========================================================================================================
    /**
     * @return Underlying volume, or nullptr if no volume is loaded.
     */
    const MRILIB::MriVolData* volume() const { return m_volume.get(); }

    //=========================================================================================================
    /**
     * @return Current crosshair position in MRI RAS coordinates.
     */
    Eigen::Vector3f crosshair() const { return m_crosshair; }

    //=========================================================================================================
    /**
     * Move the crosshair and regenerate the three ortho slices. Emits
     * @ref crosshairChanged when the value actually changes.
     */
    void setCrosshair(const Eigen::Vector3f& rasPoint);

    //=========================================================================================================
    /**
     * Convenience: same as @ref setCrosshair, accepts a `QVector3D`.
     */
    void setCrosshair(const QVector3D& rasPoint);

    //=========================================================================================================
    /** @return Renderable for the axial slice (Z = const). May be null when
     *  no volume is loaded. */
    DISP3DLIB::SliceObject* axialSlice() const { return m_slices[0].get(); }

    //=========================================================================================================
    /** @return Renderable for the coronal slice (Y = const). */
    DISP3DLIB::SliceObject* coronalSlice() const { return m_slices[1].get(); }

    //=========================================================================================================
    /** @return Renderable for the sagittal slice (X = const). */
    DISP3DLIB::SliceObject* sagittalSlice() const { return m_slices[2].get(); }

    //=========================================================================================================
    /** @return Scene-layer ids in fixed order: axial, coronal, sagittal. */
    static std::array<QString, 3> sceneLayerIds();

    //=========================================================================================================
    /**
     * Attach the plugin to a `MultimodalScene`. Registers (or updates) the
     * three `SceneLayerKind::MriSlice` layers and subscribes to the scene's
     * `picked` signal so MRI-voxel picks move the crosshair.
     *
     * Passing a null pointer detaches.
     */
    void attachScene(DISP3DLIB::MultimodalScene* scene);

public slots:
    //=========================================================================================================
    /**
     * Pick callback. If `pick.kind == PickKind::MriVoxel`, snap the
     * crosshair to `pick.world` and emit @ref crosshairChanged.
     */
    void handlePick(const DISP3DLIB::PickResult& pick);

signals:
    /** Emitted when a new volume is loaded. */
    void volumeChanged();

    /** Emitted when the crosshair moves. */
    void crosshairChanged(const QVector3D& rasPoint);

private:
    void rebuildSlices();
    void publishToScene();
    Eigen::Vector3f volumeCenter() const;

    std::unique_ptr<MRILIB::MriVolData>              m_volume;
    std::array<std::unique_ptr<DISP3DLIB::SliceObject>, 3> m_slices;
    DISP3DLIB::MultimodalScene*                      m_scene = nullptr;
    Eigen::Vector3f                                  m_crosshair = Eigen::Vector3f::Zero();
    QString                                          m_sourcePath;
};

} // namespace MRISLICESPLUGIN

#endif // MNEINSPECT_MRI_SLICES_PLUGIN_H
