//=============================================================================================================
/**
 * @file     channelrhiview.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March, 2026
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
 * @brief    Declaration of the ChannelRhiView class.
 *
 */

#ifndef CHANNELRHIVIEW_H
#define CHANNELRHIVIEW_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../disp_global.h"
#include "channeldatamodel.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QColor>
#include <QPointer>

#ifdef MNE_DISP_RHIWIDGET
#  include <QRhiWidget>
#endif

//=============================================================================================================
// DEFINE NAMESPACE DISPLIB
//=============================================================================================================

namespace DISPLIB
{

//=============================================================================================================
/**
 * @brief ChannelRhiView – GPU-accelerated (or QPainter-fallback) channel signal renderer.
 *
 * When compiled with Qt6 + GuiPrivate (MNE_DISP_RHIWIDGET defined) this widget
 * subclasses QRhiWidget and renders all channel traces via a custom GLSL pipeline
 * with min/max decimation.  Smooth scrolling and zoom animations are driven by
 * Q_PROPERTY so that QPropertyAnimation can be used directly.
 *
 * When MNE_DISP_RHIWIDGET is not defined the widget falls back to a QPainter
 * implementation that still applies min/max decimation for fast rendering.
 *
 * In both cases the public interface is identical.
 */
#ifdef MNE_DISP_RHIWIDGET
class DISPSHARED_EXPORT ChannelRhiView : public QRhiWidget
#else
class DISPSHARED_EXPORT ChannelRhiView : public QWidget
#endif
{
    Q_OBJECT

    Q_PROPERTY(float scrollSample    READ scrollSample    WRITE setScrollSample    NOTIFY scrollSampleChanged)
    Q_PROPERTY(float samplesPerPixel READ samplesPerPixel WRITE setSamplesPerPixel NOTIFY samplesPerPixelChanged)

public:
    explicit ChannelRhiView(QWidget *parent = nullptr);
    ~ChannelRhiView() override;

    //=========================================================================================================
    /**
     * Attach the data model.  The view does NOT take ownership.
     *
     * @param[in] model  The channel data model to render.
     */
    void setModel(ChannelDataModel *model);

    // ── Scroll / zoom ─────────────────────────────────────────────────

    float scrollSample()    const { return m_scrollSample; }
    float samplesPerPixel() const { return m_samplesPerPixel; }

    //=========================================================================================================
    /**
     * Set the left-edge scroll position.  Updates only the GPU uniform —
     * no VBO rebuild unless the prefetch window is exhausted.
     *
     * @param[in] sample  Absolute sample index for the left viewport edge.
     */
    void setScrollSample(float sample);

    //=========================================================================================================
    /**
     * Set the horizontal zoom (samples per screen pixel).
     * Triggers a VBO rebuild with new decimation.
     *
     * @param[in] spp  Samples per pixel (> 1 = zoomed out, < 1 = zoomed in).
     */
    void setSamplesPerPixel(float spp);

    //=========================================================================================================
    /**
     * Smoothly animate the scroll position.
     *
     * @param[in] targetSample  Target left-edge sample index.
     * @param[in] durationMs    Animation duration in milliseconds (0 = instant).
     */
    void scrollTo(float targetSample, int durationMs = 200);

    //=========================================================================================================
    /**
     * Smoothly animate the zoom level.
     *
     * @param[in] targetSpp   Target samples-per-pixel.
     * @param[in] durationMs  Animation duration in milliseconds (0 = instant).
     */
    void zoomTo(float targetSpp, int durationMs = 200);

    //=========================================================================================================
    /**
     * Background colour of the render surface.
     *
     * @param[in] color  The new background colour.
     */
    void setBackgroundColor(const QColor &color);
    QColor backgroundColor() const { return m_bgColor; }

    //=========================================================================================================
    /**
     * How many samples to prefetch beyond the visible window on each side
     * (in multiples of the visible window).  Default = 1.0.
     *
     * @param[in] factor  Prefetch factor.
     */
    void setPrefetchFactor(float factor);

    //=========================================================================================================
    /**
     * Returns the absolute sample index currently shown at x = 0 (left edge).
     * (Same as scrollSample rounded to int.)
     */
    int visibleFirstSample() const;

    //=========================================================================================================
    /**
     * Returns the number of samples currently visible.
     */
    int visibleSampleCount() const;

signals:
    void scrollSampleChanged(float sample);
    void samplesPerPixelChanged(float spp);

    //=========================================================================================================
    /**
     * Emitted whenever the user clicks; provides the sample index under the cursor.
     */
    void sampleClicked(int sample);

protected:
#ifdef MNE_DISP_RHIWIDGET
    void initialize(QRhiCommandBuffer *cb) override;
    void render(QRhiCommandBuffer *cb) override;
    void releaseResources() override;
#else
    void paintEvent(QPaintEvent *event) override;
#endif

    void resizeEvent(QResizeEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    // ── GPU resource management (QRhi path only) ──────────────────────
#ifdef MNE_DISP_RHIWIDGET
    struct ChannelGpuData {
        std::unique_ptr<QRhiBuffer> vbo;
        int                         vertexCount  = 0;
        int                         vboFirstSample = 0; // absolute first sample in VBO
    };

    void ensurePipeline();
    void rebuildVBOs(QRhiResourceUpdateBatch *batch);
    void updateUBO(QRhiResourceUpdateBatch *batch);
    bool isVboDirty() const;

    std::unique_ptr<QRhiBuffer>                  m_ubo;
    std::unique_ptr<QRhiShaderResourceBindings>  m_srb;
    std::unique_ptr<QRhiGraphicsPipeline>        m_pipeline;
    QVector<ChannelGpuData>                      m_gpuChannels;
    int                                          m_uboStride = 256;
    bool                                         m_pipelineDirty = true;
    bool                                         m_vboDirty      = true;
#endif

    // ── State shared by both paths ─────────────────────────────────────
    QPointer<ChannelDataModel> m_model;
    float                      m_scrollSample     = 0.f;
    float                      m_samplesPerPixel  = 1.f;
    float                      m_prefetchFactor   = 1.0f;
    QColor                     m_bgColor          { 30, 30, 30 };

    // ── Drag scroll support ────────────────────────────────────────────
    bool  m_dragging        = false;
    int   m_dragStartX      = 0;
    float m_dragStartScroll = 0.f;

    // ── Prefetch window tracking ───────────────────────────────────────
    int   m_vboWindowFirst  = 0;
    int   m_vboWindowLast   = 0;
};

} // namespace DISPLIB

#endif // CHANNELRHIVIEW_H
