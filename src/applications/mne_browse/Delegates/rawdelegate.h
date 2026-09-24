//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2014-2026 MNE-CPP Authors
 *
 * @file     rawdelegate.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @date     January, 2014
 * @version  2.1.0
 * @brief    This class represents the delegate of the model/view framework of mne_browse application.
 *           Since it is derived from the QAbstractItemDelegate class [1], the virtual functions paint() und sizeHint()
 *           need to be reimplemented.
 *           The paint() function is invoked from the connected QTableView for each table cell for any individual
 *           item with a certain QModelIndex. The task of this function is to paint this cell with the data
 *           that is requested by index.model()->data(index,Qt::DisplayRole); with the corresponding QModelIndex.
 *
 *           [1] http://qt-project.org/doc/qt-5/QAbstractItemDelegate.html
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

    void setActivateEvents(bool activate)         { m_bActivateEvents = activate; }
    void setShowSelectedEventsOnly(bool showOnly) { m_bShowSelectedEventsOnly = showOnly; }
    void setRemoveDC(bool removeDC)               { m_bRemoveDC = removeDC; }
    bool isRemoveDC() const                       { return m_bRemoveDC; }
    int  defaultPlotHeight() const                { return m_iDefaultPlotHeight; }

private:
    QMap<QString,double> m_scaleMap;        /**< Map with all channel types and their current scaling value.*/

    // Plot settings
    int         m_iDefaultPlotHeight;       /**< The height of the plot. */
    bool        m_bShowSelectedEventsOnly;  /**< When true only selected events are plotted. */
    bool        m_bActivateEvents;          /**< Flag for plotting events. */
    bool        m_bRemoveDC;                /**< Flag for DC removal. */

    // Scaling
    double      m_dMaxValue;                /**< Maximum value of the data to plot. */
    double      m_dScaleY;                  /**< Maximum amplitude of plot (max is m_dPlotHeight/2). */
    double      m_dDx;                      /**< pixel difference to the next sample. */
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
};

} // NAMESPACE

#endif // RAWDELEGATE_H
