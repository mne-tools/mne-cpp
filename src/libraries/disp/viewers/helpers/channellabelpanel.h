//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file channellabelpanel.h
 * @since 2026
 * @date  April 2026
 * @brief Vertical column of channel-name labels synced with @ref ChannelDataView's row geometry.
 *
 * ChannelLabelPanel listens to the same @ref ChannelDataModel as the
 * central data view and paints each visible channel's label aligned
 * to the row centre. Bad channels are greyed out and label clicks
 * emit a channel-selection signal that the data view consumes.
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
 * @brief Vertical column of channel-name labels synced with @ref ChannelDataView row geometry.
 *
 * Listens to the same @ref ChannelDataModel as the central data
 * view; bad channels are greyed and label clicks emit channel
 * selection signals.
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
     * Enable or disable butterfly mode. When active, lanes show type group labels
     * (e.g. "MEG", "EEG") instead of individual channel names.
     *
     * @param[in] enabled  true = butterfly mode, false = normal per-channel mode.
     */
    void setButterflyMode(bool enabled);

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
    void channelBadToggled(int channelIndex, bool bad);

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
    bool  m_butterflyMode   = false;
    int   m_visSampleFirst  = 0;
    int   m_visSampleLast   = 0;

    bool  m_dragging       = false;
    bool  m_dragActivated  = false;
    int   m_dragStartY     = 0;
    int   m_dragStartFirst = 0;
};

} // namespace DISPLIB

#endif // CHANNELLABELPANEL_H
