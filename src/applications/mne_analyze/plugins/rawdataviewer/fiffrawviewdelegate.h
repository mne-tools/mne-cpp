//=============================================================================================================
/**
 * @file     fiffrawviewdelegate.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Simon Heinke <Simon.Heinke@tu-ilmenau.de>;
 *           Gabriel Motta <gbmotta@mgh.harvard.edu>
 * @since    0.1.0
 * @date     January, 2019
 *
 * @section  LICENSE
 *
 * Copyright (C) 2019, Lorenz Esch, Lars Debor, Simon Heinke, Gabriel Motta. All rights reserved.
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
 * @brief    Declaration of the FiffRawViewDelegate Class.
 *
 */

#ifndef FIFFRAWDELEGATE_H
#define FIFFRAWDELEGATE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rawdataviewer_global.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QAbstractItemDelegate>
#include <QPen>

//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace ANSHAREDLIB {
    class ChannelData;
}

//=============================================================================================================
// DEFINE NAMESPACE RAWDATAVIEWERPLUGIN
//=============================================================================================================

namespace RAWDATAVIEWERPLUGIN {

//=============================================================================================================
// RAWDATAVIEWERPLUGIN FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
// DEFINE TYPEDEFS
//=============================================================================================================

//=============================================================================================================
/**
 * FiffRawViewDelegate
 */
class RAWDATAVIEWERSHARED_EXPORT FiffRawViewDelegate : public QAbstractItemDelegate
{
    Q_OBJECT

public:
    typedef QSharedPointer<FiffRawViewDelegate> SPtr;              /**< Shared pointer type for FiffRawViewDelegate. */
    typedef QSharedPointer<const FiffRawViewDelegate> ConstSPtr;   /**< Const shared pointer type for FiffRawViewDelegate. */

    //=========================================================================================================
    /**
     * Creates a new abstract item delegate with the given parent.
     *
     * @param[in] parent     Parent of the delegate.
     */
    FiffRawViewDelegate(QObject *parent = 0);

    //=========================================================================================================
    /**
     * Set the new upper item index color. This is used to only plot the background for the upper, visible item in the QTableView.
     * This is a rather ugly hack in order to cope with QOpenGLWidget's/QtableView's problem when setting a background color.
     *
     * @param[in] iUpperItem  The new upper item index color.
     */
    void setUpperItemIndex(int iUpperItemIndex);

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
    virtual void paint(QPainter *painter,
                       const QStyleOptionViewItem &option,
                       const QModelIndex &index) const override;

    //=========================================================================================================
    /**
     * Item size
     *
     * @param[in] option     Describes the parameters used to draw an item in a view widget.
     * @param[in] index      Used to locate data in a data model.
     */
    virtual QSize sizeHint(const QStyleOptionViewItem &option,
                           const QModelIndex &index) const override;

    //=========================================================================================================
    /**
     * Set the signal color.
     *
     * @param[in] signalColor  The new signal color.
     */
    void setSignalColor(const QColor& signalColor);

private:
    //=========================================================================================================
    /**
     * createPlotPath creates the QPointer path for the data plot.
     *
     * @param[in] option     Describes the parameters used to draw an item in a view widget.
     * @param[in, out] path   The QPointerPath to create for the data plot.
     * @param[in] data       Current data for the given row.
     * @param[in] dDx        pixel difference to the next sample in pixels.
     */
    void createPlotPath(const QStyleOptionViewItem &option,
                        QPainterPath& path,
                        ANSHAREDLIB::ChannelData &data,
                        double dDx,
                        const QModelIndex &index) const;

    //=========================================================================================================
    /**
     * createTimeSpacersPath Creates the QPointer path for the vertical time spacers.
     *
     * @param[in] index      Used to locate data in a data model.
     * @param[in] option     Describes the parameters used to draw an item in a view widget.
     * @param[in, out] path   The QPointerPath to create for the data plot.
     * @param[in] data       Data for the given row.
     */
    void createTimeSpacersPath(const QModelIndex &index,
                               const QStyleOptionViewItem &option,
                               QPainterPath& path,
                               ANSHAREDLIB::ChannelData &data) const;

    //=========================================================================================================
    /**
     * Draws events from the FiffRawView model's member Event Model
     *
     * @param[in] index             Used to locate data in the model.
     * @param[in] option            Describes the parameters used to draw an item in a view widget.
     * @param[in] path              The QPointerPath to create the plot.
     * @param[in] data              Data for the given row.
     * @param[in, out] painter      Used for drawing the events.
     */
    void createEventsPath(const QModelIndex &index,
                               const QStyleOptionViewItem &option,
                               QPainterPath& path,
                               ANSHAREDLIB::ChannelData &data,
                               QPainter* painter) const;

    //=========================================================================================================
    void createScroller(const QModelIndex &index,
                        const QStyleOptionViewItem &option,
                        QPainterPath& path,
                        QPainter* painter) const;

    int         m_iUpperItemIndex;          /**< The current upper item index visible in the QTableView. */

    QPen        m_penGrid;                  /**< Pen for drawing the data grid. */
    QPen        m_penNormal;                /**< Pen for drawing the data when data is plotted normally without freeze on. */
    QPen        m_penNormalSelected;        /**< Pen for drawing the data when data is plotted normally without freeze on and channel is selected. */
    QPen        m_penNormalBad;             /**< Pen for drawing the data when bad data is plotted normally without freeze on. */
    QPen        m_penNormalSelectedBad;     /**< Pen for drawing the data when bad data is plotted normally without freeze on and channel is selected. */

};

} // NAMESPACE RAWDATAVIEWERPLUGIN

#endif // FIFFRAWDELEGATE_H
