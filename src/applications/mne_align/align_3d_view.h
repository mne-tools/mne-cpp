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

#include <QPointer>
#include <QWidget>

#include <memory>

class BrainTreeModel;
class BrainView;

namespace DISP3DLIB { class MultimodalScene; }
namespace MNELIB    { class MNEBem; }

namespace MNEALIGN
{

class AcquiredPoints;

class Align3DView : public QWidget
{
    Q_OBJECT

public:
    explicit Align3DView(AcquiredPoints* acquired, QWidget* parent = nullptr);
    ~Align3DView() override;

    /** Replace any previously registered head BEM. */
    void setBem(std::shared_ptr<MNELIB::MNEBem> bem);
    void setViewCount(int count);
    void setRenderMode(const QString& modeName);
    void setCameraPreset(int preset);

    /** Number of enabled viewports as loaded/set in BrainView. */
    int     viewCount() const;
    /** Shader/render mode name as currently active in BrainView. */
    QString renderMode() const;
    /** Camera preset index last set via setCameraPreset(). */
    int     cameraPreset() const { return m_cameraPreset; }

    /** Access the underlying scene (used by the QRhi renderer). */
    DISP3DLIB::MultimodalScene* scene() const;

private slots:
    void onPointsChanged();

private:
    void rebuildAcquiredLayer();
    void rebuildBemSurfaces();
    void rebuildDigitizerLayer();
    void applyViewConfiguration();

    AcquiredPoints*                                m_pPoints = nullptr;
    std::unique_ptr<DISP3DLIB::MultimodalScene>    m_pScene;
    std::shared_ptr<MNELIB::MNEBem>                m_pBem;
    int                                            m_viewCount = 1;
    int                                            m_cameraPreset = 1;
    QString                                        m_renderMode = QStringLiteral("Anatomical");

    QPointer<BrainView>                            m_pBrainView;
    QPointer<BrainTreeModel>                       m_pBrainModel;
};

} // namespace MNEALIGN

#endif // MNE_ALIGN_ALIGN_3D_VIEW_H
