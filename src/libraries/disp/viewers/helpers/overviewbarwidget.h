//=============================================================================================================
/**
 * @file     overviewbarwidget.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     April, 2026
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
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * @brief    Declaration of the OverviewBarWidget class.
 *
 */

#ifndef OVERVIEWBARWIDGET_H
#define OVERVIEWBARWIDGET_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../disp_global.h"
#include "channelrhiview.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QImage>
#include <QPointer>
#include <QWidget>

//=============================================================================================================
// DEFINE NAMESPACE DISPLIB
//=============================================================================================================

namespace DISPLIB
{

//=============================================================================================================
// DISPLIB FORWARD DECLARATIONS
//=============================================================================================================

class ChannelDataModel;

//=============================================================================================================
/**
 * @brief Minimap / overview bar showing the full recording extent.
 *
 * A compact horizontal widget that renders a bird's-eye view of the entire
 * file with a viewport rectangle indicating the currently visible portion.
 * Clicking or dragging on the bar scrolls to that position.
 *
 * Features:
 *  - Per-channel-type RMS envelope rendered as coloured waveforms
 *  - Event / stimulus markers shown as vertical tick marks
 *  - Annotation spans shown as translucent overlays
 *  - Draggable viewport rectangle for quick navigation
 */
class DISPSHARED_EXPORT OverviewBarWidget : public QWidget
{
    Q_OBJECT

public:
    static constexpr int kBarHeight = 48;

    explicit OverviewBarWidget(QWidget *parent = nullptr);

    void setModel(ChannelDataModel *model);

    void setFirstFileSample(int first);
    void setLastFileSample(int last);
    void setSfreq(float sfreq);

    void setViewport(float scrollSample, float visibleSamples);

    void setEvents(const QVector<ChannelRhiView::EventMarker> &events);
    void setAnnotations(const QVector<ChannelRhiView::AnnotationSpan> &annotations);

    QSize sizeHint()        const override;
    QSize minimumSizeHint() const override;

signals:
    void scrollRequested(float targetSample);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    float xToSample(int x) const;
    void  rebuildEnvelope();

    QPointer<ChannelDataModel> m_model;
    int    m_firstFileSample = 0;
    int    m_lastFileSample  = -1;
    float  m_sfreq           = 1000.f;

    float  m_scrollSample    = 0.f;
    float  m_visibleSamples  = 0.f;

    QVector<ChannelRhiView::EventMarker>     m_events;
    QVector<ChannelRhiView::AnnotationSpan>  m_annotations;

    QImage m_envelopeImage;
    bool   m_envelopeDirty = true;

    bool   m_dragging      = false;
};

} // namespace DISPLIB

#endif // OVERVIEWBARWIDGET_H
