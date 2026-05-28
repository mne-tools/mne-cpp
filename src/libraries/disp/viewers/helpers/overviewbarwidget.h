//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file overviewbarwidget.h
 * @since 2026
 * @date  April 2026
 * @brief Minimap thumbnail of the entire recording shown above the raw browser.
 *
 * OverviewBarWidget paints a compressed amplitude-overview of the
 * whole acquisition next to a viewport rectangle indicating where the
 * raw browser is currently positioned. Clicks / drags on the bar emit
 * @c sampleClicked which lets the user jump anywhere in the dataset
 * with a single mouse interaction.
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
 * @brief Minimap thumbnail of the entire recording shown above the raw browser.
 *
 * Paints a compressed amplitude-overview of the whole acquisition
 * next to a viewport rectangle indicating where the raw browser is
 * currently positioned; clicks / drags emit @c sampleClicked.
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
