//=============================================================================================================
/**
 * @file     brainview.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     January, 2026
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
 *
 * @brief    BrainView class declaration.
 *
 */

#ifndef BRAINVIEW_H
#define BRAINVIEW_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "brainrenderer.h"
#include "brainsurface.h"
#include "sourceestimateoverlay.h"
#include <mne/mne_bem.h>
#include <QWidget>
#include <QRhiWidget>
#include <QMap>
#include <QElapsedTimer>
#include <memory> 
#include <QQuaternion> 

class QLabel;
class QTimer; 

//=============================================================================================================
/**
 * BrainView is the main widget for the 3D brain visualization. It handles user interaction,
 * surface loading, and coordinates with the BrainRenderer.
 *
 * @brief    BrainView class.
 */
class BrainView : public QRhiWidget
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     * Constructor
     *
     * @param[in] parent     Parent widget.
     */
    explicit BrainView(QWidget *parent = nullptr);

    //=========================================================================================================
    /**
     * Destructor
     */
    ~BrainView();

    //=========================================================================================================
    /**
     * Load a FreeSurfer surface file.
     *
     * @param[in] subjectPath Path to subjects directory.
     * @param[in] subject    Subject name.
     * @param[in] hemi       Hemisphere ("lh" or "rh").
     * @param[in] type       Surface type (e.g., "pial", "inflated").
     * @return True if successful.
     */
    bool loadSurface(const QString &subjectPath, const QString &subject, const QString &hemi, const QString &type);
    
    //=========================================================================================================
    /**
     * Load an atlas annotation file (.annot).
     *
     * @param[in] subjectPath Path to subjects directory.
     * @param[in] subject    Subject name.
     * @param[in] hemi       Hemisphere ("lh" or "rh").
     * @return True if successful.
     */
    bool loadAtlas(const QString &subjectPath, const QString &subject, const QString &hemi);

    //=========================================================================================================
    /**
     * Load BEM surfaces from a FIF file.
     *
     * @param[in] fifPath    Path to BEM FIF file.
     * @return True if successful.
     */
    bool loadBem(const QString &fifPath);
    
    //=========================================================================================================
    /**
     * Load sensors (meg/eeg/digitizer) from a raw/average FIF file or montage.
     *
     * @param[in] fifPath    Path to FIF file.
     * @return True if successful.
     */
    bool loadSensors(const QString &fifPath);

public slots:
    //=========================================================================================================
    /**
     * Set the active surface type to search for (e.g. "pial").
     *
     * @param[in] type       The surface type to activate.
     */
    void setActiveSurface(const QString &type);
    
    //=========================================================================================================
    /**
     * Set the shader mode (Standard, Holographic, Atlas).
     *
     * @param[in] mode       The shader mode to set.
     */
    void setShaderMode(const QString &mode);
    
    //=========================================================================================================
    /**
     * Set the shader mode for BEM surfaces (Standard, Holographic, Atlas).
     *
     * @param[in] mode       The shader mode to set.
     */
    void setBemShaderMode(const QString &mode);
    
    //=========================================================================================================
    /**
     * Set the overlay mode (Surface, Annotation, Scientific).
     *
     * @param[in] mode       The visualization mode to set.
     */
    void setVisualizationMode(const QString &mode);
    
    //=========================================================================================================
    /**
     * Toggle visibility of a hemisphere.
     * 
     * @param[in] hemiIdx    0 for LH, 1 for RH.
     * @param[in] visible    Visibility state.
     */
    void setHemiVisible(int hemiIdx, bool visible);

    //=========================================================================================================
    /**
     * Toggle visibility of a BEM surface layer.
     * 
     * @param[in] name       "head", "outer_skull", or "inner_skull".
     * @param[in] visible    Visibility state.
     */
    void setBemVisible(const QString &name, bool visible);

    //=========================================================================================================
    /**
     * Set whether BEM surfaces should use their standard (colorful) definition or white.
     * 
     * @param[in] enabled    True to use standard colors (Red/Green/Blue), False for White.
     */
    void setBemHighContrast(bool enabled);

    //=========================================================================================================
    /**
     * Toggle visibility of sensor groups.
     * 
     * @param[in] type       "MEG", "EEG", or "Digitizer".
     * @param[in] visible    Visibility state.
     */
    void setSensorVisible(const QString &type, bool visible);

    //=========================================================================================================
    /**
     * Enable or disable lighting for the scene.
     *
     * @param[in] enabled    True to enable lighting, false to disable.
     */
    void setLightingEnabled(bool enabled);

    //=========================================================================================================
    /**
     * Save a snapshot of the current view to a file.
     */
    void saveSnapshot();

    //=========================================================================================================
    /**
     * Load source estimate files (.stc) for both hemispheres.
     *
     * @param[in] lhPath     Path to left hemisphere .stc file.
     * @param[in] rhPath     Path to right hemisphere .stc file.
     * @return True if successful.
     */
    bool loadSourceEstimate(const QString &lhPath, const QString &rhPath);

    //=========================================================================================================
    /**
     * Set the current time point for source estimate visualization.
     *
     * @param[in] index      Time sample index.
     */
    void setTimePoint(int index);

    //=========================================================================================================
    /**
     * Set the colormap for source estimate visualization.
     *
     * @param[in] name       Colormap name ("Hot", "Jet", etc.).
     */
    void setSourceColormap(const QString &name);

    //=========================================================================================================
    /**
     * Set threshold values for source estimate visualization.
     *
     * @param[in] min        Minimum threshold.
     * @param[in] mid        Mid-point threshold.
     * @param[in] max        Maximum threshold.
     */
    void setSourceThresholds(float min, float mid, float max);
    
    //=========================================================================================================
    /**
     * Get the time step of the loaded source estimate.
     * 
     * @return Time step in seconds. Returns 0 if not loaded.
     */
    float stcStep() const;

signals:
    //=========================================================================================================
    /**
     * Emitted when the time point changes.
     *
     * @param[in] index      Time sample index.
     * @param[in] time       Time in seconds.
     */
    void timePointChanged(int index, float time);

    //=========================================================================================================
    /**
     * Emitted when a source estimate is loaded.
     *
     * @param[in] numTimePoints     Number of time samples in the source estimate.
     */
    void sourceEstimateLoaded(int numTimePoints);

protected:
    void initialize(QRhiCommandBuffer *cb) override;
    void render(QRhiCommandBuffer *cb) override;
    
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    std::unique_ptr<BrainRenderer> m_renderer;
    QMap<QString, std::shared_ptr<BrainSurface>> m_surfaces;
    std::shared_ptr<BrainSurface> m_activeSurface;
    QString m_activeSurfaceType;
    
    // Sensors (Lists of surfaces/meshes for coils/electrodes)
    QList<std::shared_ptr<BrainSurface>> m_megSensors;
    QList<std::shared_ptr<BrainSurface>> m_eegSensors;
    QList<std::shared_ptr<BrainSurface>> m_digitizers;
    
    BrainRenderer::ShaderMode m_brainShaderMode = BrainRenderer::Standard;
    BrainRenderer::ShaderMode m_bemShaderMode = BrainRenderer::Standard;
    bool m_lightingEnabled = true;
    
    QQuaternion m_cameraRotation;
    QVector3D m_sceneCenter = QVector3D(0,0,0);
    float m_zoom = 0.0f;
    QPoint m_lastMousePos;
    
    int m_frameCount = 0;
    QElapsedTimer m_fpsTimer;
    QLabel *m_fpsLabel = nullptr;
    QTimer *m_updateTimer = nullptr;
    int m_snapshotCounter = 0;
    
    std::unique_ptr<SourceEstimateOverlay> m_sourceOverlay;
    int m_currentTimePoint = 0;
};

#endif // BRAINVIEW_H
