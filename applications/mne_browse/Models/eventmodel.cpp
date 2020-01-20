//=============================================================================================================
/**
 * @file     eventmodel.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  dev
 * @date     July, 2014
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
 *
 */

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "eventmodel.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEBROWSE;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

EventModel::EventModel(QObject *parent)
: QAbstractTableModel(parent)
, m_iFirstSample(0)
, m_bFileloaded(false)
, m_sFilterEventType("All")
, m_pFiffInfo(new FiffInfo)
, m_iLastSample(0)
, m_iCurrentMarkerPos(0)
{
    //Create default event type color map
    m_eventTypeColor[1] = QColor(Qt::black);
    m_eventTypeColor[2] = QColor(Qt::magenta);
    m_eventTypeColor[3] = QColor(Qt::green);
    m_eventTypeColor[4] = QColor(Qt::red);
    m_eventTypeColor[5] = QColor(Qt::cyan);
    m_eventTypeColor[32] = QColor(Qt::yellow);
    m_eventTypeColor[998] = QColor(Qt::darkBlue);
    m_eventTypeColor[999] = QColor(Qt::darkCyan);

    //m_eventTypeList<<"1"<<"2"<<"3"<<"4"<<"4"<<"5"<<"32"<<"998"<<"999";
}


//*************************************************************************************************************

EventModel::EventModel(QFile &qFile, QObject *parent)
: QAbstractTableModel(parent)
, m_iFirstSample(0)
, m_bFileloaded(false)
, m_sFilterEventType("All")
, m_pFiffInfo(new FiffInfo)
{
    //Create default event type color map
    m_eventTypeColor[1] = QColor(Qt::black);
    m_eventTypeColor[2] = QColor(Qt::magenta);
    m_eventTypeColor[3] = QColor(Qt::green);
    m_eventTypeColor[4] = QColor(Qt::red);
    m_eventTypeColor[5] = QColor(Qt::cyan);
    m_eventTypeColor[32] = QColor(Qt::yellow);
    m_eventTypeColor[998] = QColor(Qt::darkBlue);
    m_eventTypeColor[999] = QColor(Qt::darkCyan);

    loadEventData(qFile);
}


//*************************************************************************************************************
//virtual functions
EventModel::~EventModel()
{

}


//*************************************************************************************************************
//virtual functions
int EventModel::rowCount(const QModelIndex & /*parent*/) const
{
    //Always return filtered events so that the qTableView gets the correct number of rows which are to be displayed
    if(!m_dataSamples_Filtered.size()==0)
        return m_dataSamples_Filtered.size();
    else
        return 0;
}


//*************************************************************************************************************

int EventModel::columnCount(const QModelIndex & /*parent*/) const
{
    return 3;
}


//*************************************************************************************************************

QVariant EventModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(role != Qt::DisplayRole && role != Qt::TextAlignmentRole)
        return QVariant();

    if(role==Qt::TextAlignmentRole)
        return Qt::AlignHCenter + Qt::AlignVCenter;

    if(orientation == Qt::Horizontal) {
        switch(section) {
            case 0: //sample column
                return QVariant("Sample");
            case 1: //time value column
                return QVariant("Time (s)");
            case 2: //event type column
                return QVariant("Type");
            }
    }
    else if(orientation == Qt::Vertical) {
        return QString("Event %1").arg(section);
    }

    return QVariant();
}


//*************************************************************************************************************

QVariant EventModel::data(const QModelIndex &index, int role) const
{
    if(role == Qt::TextAlignmentRole)
        return QVariant(Qt::AlignCenter | Qt::AlignVCenter);

    if(role != Qt::DisplayRole && role != Qt::BackgroundRole)
        return QVariant();

    if(index.row()>=m_dataSamples_Filtered.size())
        return QVariant();

    if (index.isValid()) {
        //******** first column (sample index) ********
        if(index.column()==0) {
            switch(role) {
                case Qt::DisplayRole:
                    return QVariant(m_dataSamples_Filtered.at(index.row())-m_iFirstSample);

                case Qt::BackgroundRole:
                    //Paint different background if event was set by user
                    if(m_dataIsUserEvent_Filtered.at(index.row()) == 1) {
                        QBrush brush;
                        brush.setStyle(Qt::SolidPattern);
                        QColor colorTemp(Qt::red);
                        colorTemp.setAlpha(15);
                        brush.setColor(colorTemp);
                        return QVariant(brush);
                    }
            }
        }

        //******** second column (event time plot) ********
        if(index.column()==1){
            switch(role) {
                case Qt::DisplayRole: {
                    int time = ((m_dataSamples_Filtered.at(index.row()) - m_iFirstSample) / m_pFiffInfo->sfreq) * 100;

                    return QVariant((double)time / 100);
                }

                case Qt::BackgroundRole:
                    //Paint different background if event was set by user
                    if(m_dataIsUserEvent_Filtered.at(index.row()) == 1) {
                        QBrush brush;
                        brush.setStyle(Qt::SolidPattern);
                        QColor colorTemp(Qt::red);
                        colorTemp.setAlpha(15);
                        brush.setColor(colorTemp);
                        return QVariant(brush);
                    }
            }
        }

        //******** third column (event type plot) ********
        if(index.column()==2) {
            switch(role) {
                case Qt::DisplayRole:
                    return QVariant(m_dataTypes_Filtered.at(index.row()));

                case Qt::BackgroundRole: {
                    QBrush brush;
                    brush.setStyle(Qt::SolidPattern);
                    brush.setColor(m_eventTypeColor.value(m_dataTypes_Filtered.at(index.row()), Qt::black));

                    QColor colorTemp = brush.color();
                    colorTemp.setAlpha(EVENT_MARKER_OPACITY);
                    brush.setColor(colorTemp);
                    return QVariant(brush);
                }
            }
        }

    } // end index.valid() check

    return QVariant();
}


//*************************************************************************************************************

bool EventModel::insertRows(int position, int span, const QModelIndex & parent)
{
    Q_UNUSED(parent);

    if(m_dataSamples.isEmpty()) {
        m_dataSamples.insert(0, m_iCurrentMarkerPos);
        m_dataTypes.insert(0, 1);
        m_dataIsUserEvent.insert(0, 1);
    }
    else {
        for (int i = 0; i < span; ++i) {
            for(int t = 0; t<m_dataSamples.size(); t++) {
                if(m_dataSamples[t] >= m_iCurrentMarkerPos) {
                    m_dataSamples.insert(t, m_iCurrentMarkerPos);

                    if(m_sFilterEventType == "All")
                        m_dataTypes.insert(t, 1);
                    else
                        m_dataTypes.insert(t, m_sFilterEventType.toInt());

                    m_dataIsUserEvent.insert(t, 1);
                    break;
                }

                if(t == m_dataSamples.size()-1) {
                    m_dataSamples.append(m_iCurrentMarkerPos);

                    if(m_sFilterEventType == "All")
                        m_dataTypes.append(1);
                    else
                        m_dataTypes.append(m_sFilterEventType.toInt());

                    m_dataIsUserEvent.append(1);
                }
            }
        }
    }

    beginInsertRows(QModelIndex(), position, position+span-1);

    endInsertRows();

    //Update filtered event data
    setEventFilterType(m_sFilterEventType);

    return true;
}


//*************************************************************************************************************

bool EventModel::removeRows(int position, int span, const QModelIndex & parent)
{
    Q_UNUSED(parent);

    for (int i = 0; i < span; ++i) {
        //Only user events can be deleted
        if(m_dataIsUserEvent[position] == 1) {
            m_dataSamples.removeAt(position);
            m_dataTypes.removeAt(position);
            m_dataIsUserEvent.removeAt(position);
        }
    }

    beginRemoveRows(QModelIndex(), position, position+span-1);

    endRemoveRows();

    //Update filtered event data
    setEventFilterType(m_sFilterEventType);

    return true;
}


//*************************************************************************************************************

Qt::ItemFlags EventModel::flags(const QModelIndex & index) const
{
    //Return editable mode only for user events an when event type filtering is deactivated
    if(m_dataIsUserEvent_Filtered[index.row()] == 1 && m_sFilterEventType == "All")
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
    else
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}


//*************************************************************************************************************

bool EventModel::setData(const QModelIndex & index, const QVariant & value, int role)
{
    if(index.row() >= m_dataSamples.size() || index.column() >= columnCount())
        return false;

    if(role == Qt::EditRole) {
        int column = index.column();
        switch(column) {
            case 0: //sample values
                m_dataSamples[index.row()] = value.toInt() + m_iFirstSample;
                break;

            case 1: //time values
                m_dataSamples[index.row()] = value.toDouble() * m_pFiffInfo->sfreq + m_iFirstSample;
                break;

            case 2: //type
                QString string = value.toString();
                m_dataTypes[index.row()] = string.toInt();
                break;
        }
    }

    //Update filtered event data
    setEventFilterType(m_sFilterEventType);

    return true;
}


//*************************************************************************************************************

bool EventModel::loadEventData(QFile& qFile)
{
    beginResetModel();
    clearModel();

    // Read events
    MatrixXi events;

    if(!MNE::read_events(qFile, events)) {
        qDebug() << "Error while reading events.";
        return false;
    }

    //std::cout << events << endl;

    qDebug() << QString("Events read from %1").arg(qFile.fileName());

    //set loaded fiff event data
    for(int i = 0; i < events.rows(); i++) {
        m_dataSamples.append(events(i,0));
        m_dataTypes.append(events(i,2));
        m_dataIsUserEvent.append(0);
    }

    //Set filtered events to original
    m_dataSamples_Filtered = m_dataSamples;
    m_dataTypes_Filtered = m_dataTypes;
    m_dataIsUserEvent_Filtered = m_dataIsUserEvent;

    //Create type string list
    m_eventTypeList.clear();
    for(int i = 0; i<m_dataTypes.size(); i++)
        if(!m_eventTypeList.contains(QString().number(m_dataTypes[i])))
            m_eventTypeList<<QString().number(m_dataTypes[i]);

    emit updateEventTypes("All");

    endResetModel();

    m_bFileloaded = true;

    return true;
}


//*************************************************************************************************************

bool EventModel::saveEventData(QFile& qFile)
{
    Q_UNUSED(qFile);

    beginResetModel();
    clearModel();

    //TODO: Save events to file

    endResetModel();
    return true;
}


//*************************************************************************************************************

void EventModel::setFiffInfo(FiffInfo::SPtr& pFiffInfo)
{
    m_pFiffInfo = pFiffInfo;
}


//*************************************************************************************************************

void EventModel::setFirstLastSample(int firstSample, int lastSample)
{
    m_iFirstSample = firstSample;
    m_iLastSample = lastSample;
}


//*************************************************************************************************************

void EventModel::setCurrentMarkerPos(int markerPos)
{
    //add m_iFirstSample because m_iFirstSample gets subtracted when data is asked from this model
    m_iCurrentMarkerPos = markerPos + m_iFirstSample;
}


//*************************************************************************************************************

FiffInfo::SPtr EventModel::getFiffInfo() const
{
    return m_pFiffInfo;
}


//*************************************************************************************************************

QPair<int, int> EventModel::getFirstLastSample() const
{
    QPair<int, int> pair(m_iFirstSample, m_iLastSample);
    return pair;
}


//*************************************************************************************************************

void EventModel::setEventFilterType(const QString eventType)
{
    m_sFilterEventType = eventType;

    //Clear filtered event data
    m_dataSamples_Filtered.clear();
    m_dataTypes_Filtered.clear();
    m_dataIsUserEvent_Filtered.clear();

    //Fill filtered event data depending on the user defined event filter type
    if(eventType == "All") {
        m_dataSamples_Filtered = m_dataSamples;
        m_dataTypes_Filtered = m_dataTypes;
        m_dataIsUserEvent_Filtered = m_dataIsUserEvent;
    }
    else {
        for(int i = 0; i<m_dataSamples.size(); i++) {
            if(m_dataTypes[i] == eventType.toInt()) {
                m_dataSamples_Filtered.append(m_dataSamples[i]);
                m_dataTypes_Filtered.append(m_dataTypes[i]);
                m_dataIsUserEvent_Filtered.append(m_dataIsUserEvent[i]);
            }
        }
    }

    emit dataChanged(createIndex(0,0), createIndex(m_dataSamples_Filtered.size(), 0));
    emit headerDataChanged(Qt::Vertical, 0, m_dataSamples_Filtered.size());
}


//*************************************************************************************************************

QStringList EventModel::getEventTypeList() const
{
    return m_eventTypeList;
}


//*************************************************************************************************************

const QMap<int, QColor> & EventModel::getEventTypeColors()
{
    return m_eventTypeColor;
}


//*************************************************************************************************************

void EventModel::clearModel()
{
    beginResetModel();
    //clear event data model structure
    m_dataSamples.clear();
    m_dataTypes.clear();
    m_dataIsUserEvent.clear();

    m_dataSamples_Filtered.clear();
    m_dataTypes_Filtered.clear();
    m_dataIsUserEvent_Filtered.clear();

    m_bFileloaded = false;

    endResetModel();

    qDebug("EventModel cleared.");
}


//*************************************************************************************************************

void EventModel::addNewEventType(const QString &eventType, const QColor &typeColor)
{
    //Add type color
    m_eventTypeColor[eventType.toInt()] = typeColor;

    //Add event type to event type list
    if(!m_eventTypeList.contains(eventType))
        m_eventTypeList<<eventType;

    emit updateEventTypes(eventType);
}
