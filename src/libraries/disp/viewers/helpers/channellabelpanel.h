//=============================================================================================================
/**
 * @file     channellabelpanel.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March, 2026
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
 * @brief    Declaration of the ChannelLabelPanel class.
 *
 */

#ifndef CHANNELLABELPANEL_H
#define CHANNELLABELPANEL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../disp_global.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QPointer>
#include <QVector>
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
 * @brief Fixed-width panel showing channel names and metadata, left of the render surface.
 *
 * Repaints one row per visible channel (height / visibleChannelCount pixels each).
 * Each row shows: a type-colour strip, channel name, type abbreviation, and a red "BAD" badge
 * when the channel is flagged bad.  Hovering over a row shows a tooltip with full detail.
 *
 * Vertical drag on the panel scrolls the channel window (emits channelScrollRequested).
 */
class DISPSHARED_EXPORT ChannelLabelPanel : public QWidget
{
    Q_OBJECT

public:
    explicit ChannelLabelPanel(QWidget *parent = nullptr);

    void setModel(ChannelDataModel *model);

    //=========================================================================================================
    /**
     * Set the index of the first visible channel row (kept in sync with ChannelRhiView).
     */
    void setFirstVisibleChannel(int ch);

    //=========================================================================================================
    /**
     * Set how many channel rows are simultaneously visible (kept in sync with ChannelRhiView).
     */
    void setVisibleChannelCount(int count);

    //=========================================================================================================
    /**
     * Restrict the panel to a subset of model channel indices.
     * When @p indices is empty all model channels are available (no filter).
     * Must be kept in sync with ChannelRhiView::setChannelIndices().
     *
     * @param[in] indices  Ordered list of model channel indices to display.
     */
    void setChannelIndices(const QVector<int> &indices);

    //=========================================================================================================
    /**
     * Show or hide bad-channel rows.  When @p hide is true, bad channels are
     * removed from the visible row list so the panel stays aligned with the trace view.
     */
    void setHideBadChannels(bool hide);

    //=========================================================================================================
    /**
     * Update the visible sample window used for the per-channel RMS level bar.
     * Call this whenever the horizontal scroll position changes.
     *
     * @param[in] firstSample  Absolute first sample in the visible window.
     * @param[in] lastSample   Absolute last  sample (exclusive) in the visible window.
     */
    void setVisibleSampleRange(int firstSample, int lastSample);

    QSize sizeHint()        const override;
    QSize minimumSizeHint() const override;

signals:
    //=========================================================================================================
    /**
     * Emitted while the user drags vertically on the panel.
     * @param[in] targetFirst  The desired new first-visible-channel index.
     */
    void channelScrollRequested(int targetFirst);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    bool event(QEvent *e) override;   // for QEvent::ToolTip

private:
    QVector<int> effectiveChannelIndices() const;

    QPointer<ChannelDataModel> m_model;
    int  m_firstVisibleChannel = 0;
    int  m_visibleChannelCount = 12;

    QVector<int> m_channelIndices; // empty = identity (all channels)
    bool  m_hideBadChannels = false;
    int   m_visSampleFirst  = 0;
    int   m_visSampleLast   = 0;

    bool  m_dragging       = false;
    int   m_dragStartY     = 0;
    int   m_dragStartFirst = 0;
};

} // namespace DISPLIB

#endif // CHANNELLABELPANEL_H
