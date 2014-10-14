//=============================================================================================================
/**
* @file     eventmodel.h
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
*           Jens Haueisen <jens.haueisen@tu-ilmenau.de>
* @version  1.0
* @date     January, 2014
*
* @section  LICENSE
*
* Copyright (C) 2014, Lorenz Esch, Christoph Dinh, Matti Hamalainen and Jens Haueisen. All rights reserved.
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
* @brief    This class represents the model of the model/view framework of mne_browse_raw_qt application.
*           It is derived from QAbstractTableModel so the virtual functions rowCount(),columnCount() and data()
*           needed to be reimplemented. The delegate requests the data for any individual table cell by
*           invoking data(QModelIndex index, int role) and a certain role. DisplayRole is the standard role
*           for requesting the plain data. Other roles such as BackgroundRole are requested to fill a cell
*           with a certain background color with respect to the individual index.
*           For further information see [1].
*
*           The way how the data is organized is totally up to the model. In our case, the raw and processed
*           data is stored in stored in the two matrices m_data[i] and m_procData[i] (both are QList that contains MatrixXdR), respectively. (nchans x m_iWindowSize)
*           The data is loaded and displayed blockwise. If the user scrolls close (meaning distanced smaller than
*           m_iReloadPos) to the loaded edge, the subsequent block is loaded from the fiff file. The maximum number
*           of loaded window blocks is determined by the parameter m_maxWindows. If m_maxWindows is reached and another
*           block is to be loaded, the first or last block (depending on whether the user scrolls to the right or left edge)
*           is removed from m_data, pretty much like a circular buffer. The logic of the reloading is managed by the
*           slot updateScrollPos, which obtains the value from the horizontal QScrollBar being part of the connected TableView.
*
*           In order to not freeze the GUI when reloading new data or filtering data, the RawModel class makes heavy use
*           of the QtConcurrent features. [2]
*           Therefore, the methods updateOperatorsConcurrently() and readSegment() is run in a background-thread. Once the results
*           are ready the m_operatorFutureWatcher and m_reloadFutureWatcher emits a signal that is connect to the slots
*           insertProcessedData() and insertReloadedData(), respectively.
*
*           MNEOperators such as FilterOperators are stored in m_Operators. The MNEOperators that are applied to any
*           individual channel are stored in the QMap m_assignedOperators.
*
*           [1] http://qt-project.org/doc/qt-5/QAbstractTableModel.html
*           [2] http://qt-project.org/doc/qt-5.0/qtconcurrent/qtconcurrent-index.html
*
*/

#ifndef EVEMODEL_H
#define EVEMODEL_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../Utils/rawsettings.h"


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QAbstractTableModel>
#include <QBrush>
#include <QSettings>
#include <QVector>
#include <QPair>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// MNE INCLUDES
//=============================================================================================================

#include <fiff/fiff.h>
#include <mne/mne.h>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace MNELIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE MNEBrowseRawQt
//=============================================================================================================

namespace MNEBrowseRawQt
{

//=============================================================================================================
/**
* DECLARE CLASS EventModel
*/
class EventModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    EventModel(QObject *parent);
    EventModel(QFile& qFile, QObject *parent);

    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    bool insertRows(int position, int span, const QModelIndex & parent = QModelIndex());
    bool removeRows(int position, int span, const QModelIndex & parent = QModelIndex());
    Qt::ItemFlags flags(const QModelIndex & index) const;
    bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole);

    //=========================================================================================================
    /**
    * loadEventData loads fiff event data file
    *
    * @param p_IODevice fiff data event file to read from
    */
    bool loadEventData(QFile& qFile);

    //=========================================================================================================
    /**
    * saveEventData saves events to a fiff event data file
    *
    * @param p_IODevice fiff data event file to save to
    */
    bool saveEventData(QFile& qFile);

    //=========================================================================================================
    /**
    * setFiffInfo sets the fiff info variabel
    *
    * @param fiffInfo fiff infp variabel
    */
    void setFiffInfo(FiffInfo& fiffInfo);

    //=========================================================================================================
    /**
    * setFirstLastSample sets the first/last sample of the loaded fiff data file
    *
    * @param firstSample first sample value
    * @param lastSample last sample value
    */
    void setFirstLastSample(int firstSample, int lastSample);

    //=========================================================================================================
    /**
    * setCurrentMarkerPos sets the current marker position
    *
    * @param markerPos marker position in samples
    */
    void setCurrentMarkerPos(int markerPos);

    //=========================================================================================================
    /**
    * getFiffInfo returns the fiffinfo
    *
    */
    FiffInfo getFiffInfo();

    //=========================================================================================================
    /**
    * getFirstLastSample returns the first/last sample in form of a QPair
    *
    */
    QPair<int, int> getFirstLastSample();

    //=========================================================================================================
    /**
    * setEventFilterType sets the event filter type
    *
    * @param eventType the event type which is to be filtered
    */
    void setEventFilterType(const QString eventType);

    //=========================================================================================================
    /**
    * getEventTypeList returns the event type list
    *
    */
    QStringList getEventTypeList();

    //=========================================================================================================
    /**
    * clearModel clears all model's members
    */
    void clearModel();

    bool            m_bFileloaded;      /**< true when a Fiff event file is loaded */

private:
    QVector<int>    m_dataSamples;              /**< Vector that holds the sample alues for each loaded event */
    QVector<int>    m_dataTypes;                /**< Vector that holds the type alues for each loaded event */
    QVector<int>    m_dataIsUserEvent;          /**< Vector that holds the flag whether the event is user defined or loaded from file */

    QVector<int>    m_dataSamples_Filtered;     /**< Filtered Vector that holds the sample alues for each loaded event */
    QVector<int>    m_dataTypes_Filtered;       /**< Filtered Vector that holds the type alues for each loaded event */
    QVector<int>    m_dataIsUserEvent_Filtered; /**< Filtered Vector that holds the flag whether the event is user defined or loaded from file */

    FiffInfo        m_fiffInfo;                 /**< fiff info of whole fiff file */

    int             m_iFirstSample;             /**< holds the first/starting sample of the fiff data file */
    int             m_iLastSample;              /**< holds the last/ending sample of the fiff data file */
    int             m_iCurrentMarkerPos;        /**< holds the current marker position */
    QSettings       m_qSettings;                /**< setting paramter to access globally defined values. see rawsettings.cpp and rawsettings.h */
    QString         m_sFilterEventType;         /**< holds the event txype which is to be filtered*/

    QStringList     m_eventTypeList;            /**< holds all loaded event types */

signals:
    void updateEventTypes();

};

} // NAMESPACE

#endif // EVEMODEL_H


