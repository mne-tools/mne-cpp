//=============================================================================================================
/**
 * @file     rtfiffrawviewdelegate.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Juan Garcia-Prieto <juangpc@gmail.com>
 * @since    0.1.0
 * @date     May, 2014
 *
 * @section  LICENSE
 *
 * Copyright (C) 2014, Lorenz Esch, Christoph Dinh, Gabriel Motta, Juan Garcia-Prieto. All rights reserved.
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
 * @brief    Declaration of the RtFiffRawViewDelegate Class.
 *
 */

#ifndef RTFIFFRAWVIEWDELEGATE_H
#define RTFIFFRAWVIEWDELEGATE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../disp_global.h"
#include "../scalingview.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QAbstractItemDelegate>
#include <QPen>
#include <QPainterPath>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace RTPROCESSINGLIB{
    class EventList;
}

//=============================================================================================================
// DEFINE NAMESPACE DISPLIB
//=============================================================================================================

namespace DISPLIB
{

//=============================================================================================================
// DISPLIB FORWARD DECLARATIONS
//=============================================================================================================

class RtFiffRawView;

//=============================================================================================================
// DEFINE TYPEDEFS
//=============================================================================================================

typedef QPair<const double*,qint32> RowVectorPair;

//=============================================================================================================
/**
 * DECLARE CLASS RtFiffRawViewDelegate
 *
 * @brief The RtFiffRawViewDelegate class represents a RTMSA delegate which creates the plot paths
 */
class DISPSHARED_EXPORT RtFiffRawViewDelegate : public QAbstractItemDelegate
{
    Q_OBJECT

public:
    typedef QSharedPointer<RtFiffRawViewDelegate> SPtr;              /**< Shared pointer type for RtFiffRawViewDelegate. */
    typedef QSharedPointer<const RtFiffRawViewDelegate> ConstSPtr;   /**< Const shared pointer type for RtFiffRawViewDelegate. */

    //=========================================================================================================
    /**
     * Creates a new abstract item delegate with the given parent.
     *
     * @param[in] parent     Parent of the delegate.
     */
    RtFiffRawViewDelegate(RtFiffRawView* parent = 0);

    //=========================================================================================================
    /**
     * Initializes painter path variables to fit number of channels in the model/view.
     *
     * @param[in] model     model.
     */
    void initPainterPaths(const QAbstractTableModel *model);

    //=========================================================================================================
    /**
     * Use the painter and style option to render the item specified by the item index.
     *
     * (sizeHint() must be implemented also)
     *
     * @param[in] painter    Low-level painting on widgets and other paint devices.
     * @param[in] option     Describes the parameters used to draw an item in a view widget.
     * @param[in] index      Used to locate data in a data model.
     */
    virtual void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;

    //=========================================================================================================
    /**
     * Item size
     *
     * @param[in] option     Describes the parameters used to draw an item in a view widget.
     * @param[in] index      Used to locate data in a data model.
     */
    virtual QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;

    //=========================================================================================================
    /**
     * markerMoved is called whenever user moves the mouse inside of the table view viewport
     *
     * @param[in] position   The current mouse position.
     * @param[in] activeRow  The current row which the mouse is moved over.
     */
    void markerMoved(QPoint position, int activeRow);

    //=========================================================================================================
    /**
     * Set the signal color.
     *
     * @param[in] signalColor  The new signal color.
     */
    void setSignalColor(const QColor& signalColor);

    //=========================================================================================================
    /**
     * Returns the signal color.
     *
     * @return  The current signal color.
     */
    QColor getSignalColor();

    //=========================================================================================================
    /**
     * Set the new upper item index color. This is used to only plot the background for the upper, visible item in the QTableView.
     * This is a rather ugly hack in order to cope with QOpenGLWidget's/QtableView's problem when setting a background color.
     *
     * @param[in] iUpperItem  The new upper item index color.
     */
    void setUpperItemIndex(int iUpperItemIndex);

private:
    //=========================================================================================================
    /**
     * createPlotPath creates the QPointer path for the data plot.
     *
     * @param[in] index      Used to locate data in a data model.
     * @param[in] option     Describes the parameters used to draw an item in a view widget.
     * @param[in, out] path   The QPointerPath to create for the data plot.
     * @param[in] data       Current data for the given row.
     */
    void createPlotPath(const QModelIndex &index,
                        const QStyleOptionViewItem &option,
                        QPainterPath& path,
                        const DISPLIB::RowVectorPair &data) const;

    //=========================================================================================================
    /**
     * createCurrentPositionMarkerPath Creates the QPointer path for the current marker position plot.
     *
     * @param[in] index      Used to locate data in a data model.
     * @param[in] option     Describes the parameters used to draw an item in a view widget.
     * @param[in, out] path   The QPointerPath to create for the data plot.
     */
    void createCurrentPositionMarkerPath(const QModelIndex &index, const QStyleOptionViewItem &option, QPainterPath& path) const;

    //=========================================================================================================
    /**
     * createGridPath Creates the QPointer path for the grid plot.
     *
     * @param[in] index      Used to locate data in a data model.
     * @param[in] option     Describes the parameters used to draw an item in a view widget.
     * @param[in, out] path   The QPointerPath to create for the data plot.
     * @param[in] data       Data for the given row.
     */
    void createGridPath(const QModelIndex &index, const QStyleOptionViewItem &option, QPainterPath& path, RowVectorPair &data) const;

    //=========================================================================================================
    /**
     * createTimeSpacersPath Creates the QPointer path for the vertical time spacers.
     *
     * @param[in] index      Used to locate data in a data model.
     * @param[in] option     Describes the parameters used to draw an item in a view widget.
     * @param[in, out] path   The QPointerPath to create for the data plot.
     * @param[in] data       Data for the given row.
     */
    void createTimeSpacersPath(const QModelIndex &index, const QStyleOptionViewItem &option, QPainterPath& path, RowVectorPair &data) const;

    //=========================================================================================================
    /**
     * createTriggerPath Creates the QPointer path for the trigger line plot.
     *
     * @param[in] painter    Low-level painting on widgets and other paint devices.
     * @param[in] index      Used to locate data in a data model.
     * @param[in] option     Describes the parameters used to draw an item in a view widget.
     * @param[in, out] path   The QPointerPath to create for the data plot.
     * @param[in] data       Data for the given row.
     */
    void createTriggerPath(QPainter *painter, const QModelIndex &index, const QStyleOptionViewItem &option, QPainterPath& path, RowVectorPair &data) const;

    //=========================================================================================================
    /**
     * createTriggerThresholdPath Creates the QPointer path for the trigger threshold line plot.
     *
     * @param[in] index      Used to locate data in a data model.
     * @param[in] option     Describes the parameters used to draw an item in a view widget.
     * @param[in, out] path   The QPointerPath to create for the data plot.
     * @param[in] data       Data for the given row.
     * @param[in] textPosition Position of the text.
     */
    void createTriggerThresholdPath(const QModelIndex &index, const QStyleOptionViewItem &option, QPainterPath& path, RowVectorPair &data, QPointF &textPosition) const;

    //=========================================================================================================
    /**
     * createMarkerPath Creates the QPointer path for the marker plot.
     *
     * @param[in] option     Describes the parameters used to draw an item in a view widget.
     * @param[in, out] path   The QPointerPath to create for the data plot.
     */
    void createMarkerPath(const QModelIndex &index, const QStyleOptionViewItem &option, QPainterPath& path) const;

    //=========================================================================================================
    /**
     * Calc Point to plot given current path y values and x increments.
     *
     * @param[in] path  The QPointerPath to retrieve current position of plot.
     * @param[in] dx    The X increment.
     * @param[in] y     The new y value to plot.
     * @param[in] ybase   The y offset to apply.
     * @param[in] yscale   The y scaling factor to apply.
     */
    inline QPointF calcPoint(QPainterPath& path, const double dx, const double y, const double ybase, const double yScale) const;

    //=========================================================================================================
    /**
     * Allows to access the parent Object (FiffRawView) sampling frequency member and returns the sampling period.
     *
     */
    inline double retrieveSamplingPeriod() const;

    RtFiffRawView*      m_pParent;          /**< Pointer to parent class. **/
    QPoint              m_markerPosition;   /**< Current mouse position used to draw the marker in the plot. */
    QList<QPainterPath> m_painterPaths;     /**< List of all current painter paths for each row. */

    double              m_dMaxValue;        /**< Maximum value of the data to plot. */
    double              m_dScaleY;          /**< Maximum amplitude of plot (max is m_dPlotHeight/2). */
    int                 m_iActiveRow;       /**< The current row which the mouse is moved over. */
    int                 m_iUpperItemIndex;  /**< The current upper item index visible in the QTableView. */

    QPen        m_penMarker;                /**< Pen for drawing the data marker. */
    QPen        m_penGrid;                  /**< Pen for drawing the data grid. */
    QPen        m_penTimeSpacers;           /**< Pen for drawing the time spacer. */
    QPen        m_penFreeze;                /**< Pen for drawing the data when freeze is on. */
    QPen        m_penFreezeSelected;        /**< Pen for drawing the data when freeze is on and channel is selected. */
    QPen        m_penFreezeBad;             /**< Pen for drawing the bad data when freeze is on. */
    QPen        m_penFreezeSelectedBad;     /**< Pen for drawing the bad data when freeze is on and channel is selected. */
    QPen        m_penNormal;                /**< Pen for drawing the data when data is plotted normally without freeze on. */
    QPen        m_penNormalSelected;        /**< Pen for drawing the data when data is plotted normally without freeze on and channel is selected. */
    QPen        m_penNormalBad;             /**< Pen for drawing the data when bad data is plotted normally without freeze on. */
    QPen        m_penNormalSelectedBad;     /**< Pen for drawing the data when bad data is plotted normally without freeze on and channel is selected. */

    QMap<double,QColor> m_mapTriggerColors; /**< Colors per trigger. */
};
} // NAMESPACE

#endif // RTFIFFRAWVIEWDELEGATE_H
