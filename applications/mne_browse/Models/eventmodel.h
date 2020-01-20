//=============================================================================================================
/**
 * @file     eventmodel.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  dev
 * @date     January, 2014
 *
 * @section  LICENSE
 *
 * Copyright (C) 2014, Christoph Dinh, Lorenz Esch. All rights reserved.
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
 * @brief    This class represents the event model of the model/view framework of mne_browse application.
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
// DEFINE NAMESPACE MNEBROWSE
//=============================================================================================================

namespace MNEBROWSE
{

//=============================================================================================================
/**
 * DECLARE CLASS EventModel
 */
class EventModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     * Constructors
     */
    EventModel(QObject *parent);
    EventModel(QFile& qFile, QObject *parent);

    //=========================================================================================================
    /**
     * Destructor
     */
    virtual ~EventModel();

    //=========================================================================================================
    /**
     * Reimplemented virtual functions
     *
     */
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
     * @param fiffInfo fiff info variabel
     */
    void setFiffInfo(FiffInfo::SPtr& pFiffInfo);

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
    FiffInfo::SPtr getFiffInfo() const;

    //=========================================================================================================
    /**
     * getFirstLastSample returns the first/last sample in form of a QPair
     *
     */
    QPair<int, int> getFirstLastSample() const;

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
    QStringList getEventTypeList() const;

    //=========================================================================================================
    /**
     * getEventTypeColors returns the event type colors
     *
     */
    const QMap<int, QColor> & getEventTypeColors();

    //=========================================================================================================
    /**
     * clearModel clears all model's members
     *
     */
    void clearModel();

    //=========================================================================================================
    /**
     * adds a new event type
     *
     * @param [in] eventType the type to be added
     * @param [in] typeColor the type color to be added
     */
    void addNewEventType(const QString &eventType, const QColor &typeColor);

    bool            m_bFileloaded;              /**< True when a Fiff event file is loaded. */

private:
    QVector<int>        m_dataSamples;              /**< Vector that holds the sample alues for each loaded event. */
    QVector<int>        m_dataTypes;                /**< Vector that holds the type alues for each loaded event. */
    QVector<int>        m_dataIsUserEvent;          /**< Vector that holds the flag whether the event is user defined or loaded from file. */

    QMap<int, QColor>   m_eventTypeColor;           /**< Colors for all event types. */

    QVector<int>        m_dataSamples_Filtered;     /**< Filtered Vector that holds the sample alues for each loaded event. */
    QVector<int>        m_dataTypes_Filtered;       /**< Filtered Vector that holds the type alues for each loaded event. */
    QVector<int>        m_dataIsUserEvent_Filtered; /**< Filtered Vector that holds the flag whether the event is user defined or loaded from file. */

    FiffInfo::SPtr      m_pFiffInfo;                /**< Fiff info of whole fiff file. */

    int                 m_iFirstSample;             /**< The first/starting sample of the fiff data file. */
    int                 m_iLastSample;              /**< The last/ending sample of the fiff data file. */
    int                 m_iCurrentMarkerPos;        /**< The current marker position. */
    QSettings           m_qSettings;                /**< Setting paramter to access globally defined values. see rawsettings.cpp and rawsettings.h. */
    QString             m_sFilterEventType;         /**< The event txype which is to be filtered.*/

    QStringList         m_eventTypeList;            /**< All currently loaded event types. */

signals:
    //=========================================================================================================
    /**
     * updateEventTypes is emmited whenever the list of stored event type chnges
     *
     * @param currentFilterType the current set filter event type
     */
    void updateEventTypes(const QString& currentFilterType);
};

} // NAMESPACE

#endif // EVEMODEL_H


