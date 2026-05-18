//=============================================================================================================
/**
 * @file     align_3d_view.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *       following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 *       the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * @brief    Lightweight 3-D viewer for MNE Align.
 *
 *           Hosts a @ref DISP3DLIB::MultimodalScene and registers two
 *           layers: the loaded head BEM mesh (`SceneLayerKind::BrainSurface`)
 *           and the live set of acquired digitisation points
 *           (`SceneLayerKind::Custom`, id="acquired_points"). The actual
 *           QRhi rendering plugs in via `MultimodalScene` consumers; this
 *           widget keeps a status placeholder until the renderer is wired
 *           up.
 */

#ifndef MNE_ALIGN_ALIGN_3D_VIEW_H
#define MNE_ALIGN_ALIGN_3D_VIEW_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <QMap>
#include <QMatrix4x4>
#include <QPointer>
#include <QQuaternion>
#include <QTimer>
#include <QWidget>

#include <memory>

#include "acquired_points.h"

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class BrainTreeModel;
class BrainView;

namespace DISP3DLIB { class MultimodalScene; }
namespace MNELIB    { class MNEBem; }

//=============================================================================================================
// DEFINE NAMESPACE MNEALIGN
//=============================================================================================================

namespace MNEALIGN
{

//=============================================================================================================
/**
 * @brief Camera focus mode for the 3-D viewer.
 */
enum class CameraFocus { Brain, Pointer };

//=============================================================================================================
/**
 * @brief 3-D viewer widget for the MNE Align digitisation workflow.
 *
 * Hosts a @ref DISP3DLIB::MultimodalScene, renders the head BEM mesh,
 * digitised points, and live Polhemus tracker markers. Manages the
 * device→head and head→MRI transform chain.
 */
class Align3DView : public QWidget
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     * @brief Constructs the 3-D viewer.
     *
     * @param[in] acquired   Shared digitised-point store (not owned).
     * @param[in] parent     Parent widget.
     */
    explicit Align3DView(AcquiredPoints* acquired, QWidget* parent = nullptr);

    //=========================================================================================================
    /**
     * @brief Destroys the 3-D viewer.
     */
    ~Align3DView() override;

    //=========================================================================================================
    /**
     * @brief Replace any previously registered head BEM.
     *
     * @param[in] bem   BEM model to display.
     */
    void setBem(std::shared_ptr<MNELIB::MNEBem> bem);

    /**
     * @brief Set the number of viewport panes (1–4).
     *
     * @param[in] count   Number of viewports.
     */
    void setViewCount(int count);

    /**
     * @brief Set the render shader mode ("Anatomical" or "Holographic").
     *
     * @param[in] modeName   Shader mode name.
     */
    void setRenderMode(const QString& modeName);

    /**
     * @brief Set the camera preset index (0–6).
     *
     * @param[in] preset   Camera preset index.
     */
    void setCameraPreset(int preset);

    /**
     * @brief Set the camera focus target.
     *
     * @param[in] focus   Camera focus mode.
     */
    void setCameraFocus(CameraFocus focus);

    /**
     * @brief Set the Polhemus pen station number (1–4).
     *
     * @param[in] station   Pen station id.
     */
    void setPenStation(int station);

    /** @return Current camera preset index. */
    int         cameraPreset() const { return m_cameraPreset; }

    /** @return Current camera focus mode. */
    CameraFocus cameraFocus()  const { return m_cameraFocus; }

    /** @return The underlying multimodal scene. */
    DISP3DLIB::MultimodalScene* scene() const;

    /** @return The loaded BEM data (may be null). */
    std::shared_ptr<MNELIB::MNEBem> bem() const { return m_pBem; }

    /** @return Combined device→MRI transform (device→head × head→MRI). */
    QMatrix4x4 trackerToMri() const { return m_headToMri * m_deviceToHead; }

    /** @return Device→Head transform (runtime, not stored in trans.fif). */
    QMatrix4x4 deviceToHead() const { return m_deviceToHead; }

    /** @return Head→MRI coregistration (stored in trans.fif). */
    QMatrix4x4 headToMri() const { return m_headToMri; }

    /**
     * @brief Notify that the digitizer connection state changed.
     *
     * @param[in] connected   True if a digitizer is connected.
     */
    void setDigitizerConnected(bool connected);

    /**
     * @brief Push a live tracker pose from the digitizer.
     *
     * @param[in] station      Polhemus station id (1–4).
     * @param[in] position     Position in metres, sensor frame.
     * @param[in] orientation  Sensor orientation quaternion.
     */
    void setLiveDigitizerPose(int station, const QVector3D& position, const QQuaternion& orientation);

signals:
    /** @brief Emitted when the viewport count changes. */
    void viewCountChanged(int count);

    /** @brief Emitted when the render shader mode changes. */
    void renderModeChanged(const QString& modeName);

    /** @brief Emitted on single-click ray-pick against the BEM surface. */
    void surfacePointClicked(const QVector3D& worldPos);

    /** @brief Emitted on double-click ray-pick against the BEM surface. */
    void surfacePointDoubleClicked(const QVector3D& worldPos);

private slots:
    void onPointsChanged();
    void onLiveUpdateTick();

private:
    void rebuildAcquiredLayer();
    void rebuildBemSurfaces();
    void rebuildDigitizerLayer();
    void rebuildStaticMarkers();
    void recomputeAlignment();
    void applyViewConfiguration();

    AcquiredPoints*                                m_pPoints = nullptr;
    std::unique_ptr<DISP3DLIB::MultimodalScene>    m_pScene;
    std::shared_ptr<MNELIB::MNEBem>                m_pBem;
    int                                            m_cameraPreset = 1;
    CameraFocus                                    m_cameraFocus = CameraFocus::Brain;
    int                                            m_penStation = 1;

    // Per-station live tracker state
    struct StationPose {
        QVector3D   position;
        QQuaternion orientation;
    };
    bool                                           m_digitizerConnected = false;
    bool                                           m_liveTrackerDirty = false;
    QMap<int, StationPose>                         m_stationPoses;
    QTimer                                         m_liveUpdateTimer;
    QMatrix4x4                                     m_deviceToHead;  ///< runtime offset, not in trans.fif
    QMatrix4x4                                     m_headToMri;     ///< coregistration for trans.fif

    QPointer<BrainView>                            m_pBrainView;
    QPointer<BrainTreeModel>                       m_pBrainModel;
};

} // namespace MNEALIGN

#endif // MNE_ALIGN_ALIGN_3D_VIEW_H
