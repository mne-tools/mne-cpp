//=============================================================================================================
/**
 * @file     rawdelegate.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @version  1.0
 * @date     January, 2014
 *
 * @section  LICENSE
 *
 * Copyright (C) 2014, Christoph Dinh. All rights reserved.
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
 * @brief    This class represents the delegate of the model/view framework of mne_browse application.
 *           Since it is derived from the QAbstractItemDelegate class [1], the virtual functions paint() und sizeHint()
 *           need to be reimplemented.
 *           The paint() function is invoked from the connected QTableView for each table cell for any individual
 *           item with a certain QModelIndex. The task of this function is to paint this cell with the data
 *           that is requested by index.model()->data(index,Qt::DisplayRole); with the corresponding QModelIndex.
 *
 *           [1] http://qt-project.org/doc/qt-5/QAbstractItemDelegate.html
 *
 */

#ifndef RAWDELEGATE_H
#define RAWDELEGATE_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../Models/rawmodel.h"
#include "../Models/eventmodel.h"

#include "../Utils/types.h"
#include "../Utils/rawsettings.h"

#include "../Windows/scalewindow.h"


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QDebug>
#include <QAbstractItemDelegate>
#include <QPainter>
#include <QPainterPath>
#include <QPointF>
#include <QRect>
#include <QTableView>


//*************************************************************************************************************
//=============================================================================================================
// MNE INCLUDES
//=============================================================================================================

#include <fiff/fiff.h>
#include <mne/mne.h>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/SparseCore>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace MNELIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE MNEBROWSE
//=============================================================================================================

namespace MNEBROWSE
{

//*************************************************************************************************************
//=============================================================================================================
// DEFINE FORWARD DECLARATIONS
//=============================================================================================================

class ScaleWindow;


//=============================================================================================================
/**
 * DECLARE CLASS RawDelegate
 */
class RawDelegate : public QAbstractItemDelegate
{
    Q_OBJECT
public:
    RawDelegate(QObject *parent = 0);

    //=========================================================================================================
    /**
    * Reimplemented virtual functions
    *
    */
    virtual void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    virtual QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;

    //=========================================================================================================
    /**
    * setModelView creates the QPointer path for the data plot.
    *
    * @param[in] model holds a pointer to the event model. This model needs to be set in order to access the event data for plotting.
    * @param[in] eventView holds a pointer to the event view. This view needs to be set in order to access the selected event data for plotting.
    * @param[in] rawView holds a pointer to the raw view. This view needs to be set in order to access the raw view for manual viewport updating.
    */
    void setModelView(EventModel *eventModel, QTableView* eventView, QTableView *rawView);

    //=========================================================================================================
    /**
    * setModelView creates the QPointer path for the data plot.
    *
    * @param[in] scaleMap map with all channel types and their current scaling value.
    */
    void setScaleMap(const QMap<QString, double> &scaleMap);

    QMap<QString,double> m_scaleMap;        /**< Map with all channel types and their current scaling value.*/

    // Plots settings
    int         m_iDefaultPlotHeight;       /**< The height of the plot. */
    bool        m_bShowSelectedEventsOnly;  /**< When true all events are plotted otherwise only plot selected event. */
    bool        m_bActivateEvents;          /**< Flag for plotting events. */
    bool        m_bRemoveDC;                /**< Flag for DC removal. */

    // Scaling
    double      m_dMaxValue;                /**< Maximum value of the data to plot. */
    double      m_dScaleY;                  /**< Maximum amplitude of plot (max is m_dPlotHeight/2). */
    double      m_dDx;                      /**< pixel difference to the next sample. */

private:
    //=========================================================================================================
    /**
    * createPlotPath creates the QPointer path for the data plot.
    *
    * @param[in] index QModelIndex for accessing associated data and model object.
    * @param[in,out] path The QPointerPath to create for the data plot.
    */
    void createPlotPath(const QModelIndex &index, const QStyleOptionViewItem &option, QPainterPath& path, QList<RowVectorPair>& listPairs, double channelMean) const;

    //=========================================================================================================
    /**
    * createGridPath Creates the QPointer path for the grid plot.
    *
    * @param[in,out] path The row vector of the data matrix <1 x nsamples>.
    * @param[in] data The row vector of the data matrix <1 x nsamples>.
    */
    void createGridPath(QPainterPath& path, const QStyleOptionViewItem &option, QList<RowVectorPair>& listPairs) const;

    //=========================================================================================================
    /**
    * plotEvents Plots the events.
    *
    * @param[in] index QModelIndex for accessing associated data and model object.
    * @param[in] painter The painter of the current table item.
    */
    void plotEvents(const QModelIndex &index, const QStyleOptionViewItem &option, QPainter *painter) const;

    //Settings
    qint8           m_nhlines;              /**< Number of horizontal lines for the grid plot. */
    QSettings       m_qSettings;            /**< QSettings variable used to write or read from independent application sessions. */

    //Event model view
    EventModel*     m_pEventModel;           /**< Pointer to the event model. */
    QTableView*     m_pEventView;            /**< Pointer to the event view. */
    QTableView*     m_pRawView;              /**< Pointer to the raw view. */
    ScaleWindow*    m_pScaleWindow;          /**< Pointer to the scale window. */
};

} // NAMESPACE

#endif // RAWDELEGATE_H
