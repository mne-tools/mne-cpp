//=============================================================================================================
/**
 * @file     annotationmodel.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Gabriel Motta <gbmotta@mgh.harvard.edu>
 * @since    0.1.0
 * @date     March, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020, Christoph Dinh, Lorenz Esch, Gabriel Motta. All rights reserved.
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
 * @brief    Definition of the AnnotationModel Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "annotationmodel.h"
#include "fiffrawviewmodel.h"
#include <mne/mne.h>
#include <iomanip>
#include <iostream>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QBrush>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QFileDialog>
#include <QBuffer>
#include <QMessageBox>

//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace ANSHAREDLIB;

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

#define ALLGROUPS 9999

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

EventModel::EventModel(QObject* parent)
: AbstractModel(parent)
, m_iIndexCount(0)
, m_iSamplePos(0)
, m_iFirstSample(0)
, m_iSelectedCheckState(0)
, m_iSelectedAnn(0)
, m_iLastTypeAdded(0)
, m_fFreq(600)
, m_sFilterEventType("All")
{
    qInfo() << "[AnnotationModel::AnnotationModel] CONSTRUCTOR";
    initModel();

}

//=============================================================================================================

EventModel::EventModel(QSharedPointer<FiffRawViewModel> pFiffModel,
                QObject* parent)
: AbstractModel(parent)
, m_iIndexCount(0)
, m_iSamplePos(0)
, m_iFirstSample(0)
, m_iSelectedCheckState(0)
, m_iSelectedAnn(0)
, m_iLastTypeAdded(0)
, m_fFreq(600)
, m_sFilterEventType("All")
, m_pFiffModel(pFiffModel)
{
    initModel();
}

//=============================================================================================================

EventModel::EventModel(const QString &sFilePath,
                                 const QByteArray& byteLoadedData,
                                 float fSampFreq,
                                 int iFirstSampOffst,
                                 QObject* parent)
: AbstractModel(parent)
, m_iIndexCount(0)
, m_iSamplePos(0)
, m_iFirstSample(0)
, m_iSelectedCheckState(0)
, m_iSelectedAnn(0)
, m_iLastTypeAdded(0)
, m_fFreq(600)
, m_sFilterEventType("All")

{
    Q_UNUSED(byteLoadedData)
    Q_UNUSED(fSampFreq)
    Q_UNUSED(iFirstSampOffst)

    initModel();
    initFromFile(sFilePath);
}

//=============================================================================================================

EventModel::~EventModel()
{
    for(EventGroup* eventGroup : m_mAnnotationHub){
        if(eventGroup){
            delete eventGroup;
        }
    }
}

//=============================================================================================================

QStringList EventModel::getEventTypeList() const
{
    return m_eventTypeList;
}

//=============================================================================================================

bool EventModel::insertRows(int position,
                                 int span,
                                 const QModelIndex & parent)
{
    Q_UNUSED(parent);

    if(!m_selectedEventGroups.size()){
        return false;
    }

    m_EventManager.addEvent(m_iSamplePos, m_selectedEventGroups.front());

    beginInsertRows(QModelIndex(), position, position+span-1);

    endInsertRows();

//    Update filtered event data
//    setEventFilterType(m_sFilterEventType);

    eventsUpdated();

    return true;
}

//=============================================================================================================

void EventModel::setSamplePos(int iSamplePos)
{
    m_iSamplePos = iSamplePos;
}

//=============================================================================================================

int EventModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_EventManager.getEventsInGroups(m_selectedEventGroups)->size();
}

//=============================================================================================================

int EventModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    return 2;
}

//=============================================================================================================

QVariant EventModel::data(const QModelIndex &index,
                               int role) const
{
    if(role == Qt::TextAlignmentRole)
        return QVariant(Qt::AlignCenter | Qt::AlignVCenter);

    if(role != Qt::DisplayRole && role != Qt::BackgroundRole)
        return QVariant();

    auto events = m_EventManager.getEventsInGroups(m_selectedEventGroups);

    if(index.row() >= events->size())
        return QVariant();

    if (index.isValid()) {
        //******** first column (sample index) ********
        if(index.column()==0) {
            switch(role) {
            case Qt::DisplayRole:{
                return QVariant((*events)[index.row()].sample - m_iFirstSample);
//                    return QVariant(m_dataSamplesFiltered.at(index.row())-m_iFirstSample);
            }
            case Qt::BackgroundRole:{
                QBrush brush;
                if(m_selectedEventGroups.size() < 2){
                    brush.setStyle(Qt::SolidPattern);
                    brush.setColor(Qt::white);
                } else {
                    auto groupColor = m_EventManager.getGroup(events->at(index.row()).groupId).color;
                    brush.setStyle(Qt::SolidPattern);
                    brush.setColor(QColor(groupColor.r, groupColor.g, groupColor.b));
                }
                QColor colorTemp = brush.color();
                colorTemp.setAlpha(110);
                brush.setColor(colorTemp);
                return QVariant(brush);
            }
            }
        }

        //******** second column (event time plot) ********
        if(index.column()==1){
            switch(role) {
            case Qt::DisplayRole: {
                int iSample = (*events)[index.row()].sample - m_iFirstSample;
                float fTime = static_cast<float>(iSample) / m_fFreq;
                return QVariant(fTime);
//                    int time = ((m_dataSamplesFiltered.at(index.row()) - m_iFirstSample) / m_fFreq) * 1000;
//                    return QVariant((double)time / 1000);
            }
            case Qt::BackgroundRole:
                QBrush brush;
                if(m_selectedEventGroups.size() < 2){
                    brush.setStyle(Qt::SolidPattern);
                    brush.setColor(Qt::white);
                } else {
                    auto groupColor = m_EventManager.getGroup(events->at(index.row()).groupId).color;
                    brush.setStyle(Qt::SolidPattern);
                    brush.setColor(QColor(groupColor.r, groupColor.g, groupColor.b));
                }

                QColor colorTemp = brush.color();
                colorTemp.setAlpha(110);
                brush.setColor(colorTemp);
                return QVariant(brush);
            }
        }

        //******** third column (event type plot) ********
        if(index.column()==2) {
            switch(role) {
                case Qt::DisplayRole:
                    return QVariant(/*(*events)[index.row()].id*/);

                case Qt::BackgroundRole: {
                    auto groupColor = m_EventManager.getGroup(events->at(index.row()).groupId).color;
                    QBrush brush;
                    brush.setStyle(Qt::SolidPattern);
                    brush.setColor(QColor(groupColor.r, groupColor.g, groupColor.b));

                    QColor colorTemp = brush.color();
                    colorTemp.setAlpha(110);
                    brush.setColor(colorTemp);
                    return QVariant(brush);
                }
            }
        }

    }
    return QVariant();
}

//=============================================================================================================

bool EventModel::setData(const QModelIndex &index,
                              const QVariant &value,
                              int role)
{
    if(index.row() >= rowCount() || index.column() >= columnCount())
        return false;

    if(role == Qt::EditRole) {
        int column = index.column();
        auto events = m_EventManager.getEventsInGroups(m_selectedEventGroups);
        switch(column) {
            case 0: //sample values
                m_EventManager.moveEvent(events->at(index.row()).id, value.toInt() + m_iFirstSample);
                //m_dataSamples[index.row()] = value.toInt() + m_iFirstSample;
                break;

            case 1: //time values
                m_EventManager.moveEvent(events->at(index.row()).id, value.toDouble() * m_fFreq + m_iFirstSample);
                //m_dataSamples[index.row()] = value.toDouble() * m_fFreq + m_iFirstSample;
                break;

//            case 2: //type
//                QString string = value.toString();
//                m_dataTypes[index.row()] = string.toInt();
//                break;
        }
    }

    emit dataChanged(index, index);

    return true;
}

//=============================================================================================================

void EventModel::setEventFilterType(const QString eventType)
{
    m_sFilterEventType = eventType;

    //Clear filtered event data
    m_dataSamplesFiltered.clear();
    m_dataTypesFiltered.clear();
    m_dataIsUserEventFiltered.clear();
    m_dataGroupFiltered.clear();

    //Fill filtered event data depending on the user defined event filter type
    if(eventType == "All") {
        m_dataSamplesFiltered = m_dataSamples;
        m_dataTypesFiltered = m_dataTypes;
        m_dataIsUserEventFiltered = m_dataIsUserEvent;
        m_dataGroupFiltered = m_dataGroup;
    }
    else {
        for(int i = 0; i<m_dataSamples.size(); i++) {
            if(m_dataTypes[i] == eventType.toInt()) {
                m_dataSamplesFiltered.append(m_dataSamples[i]);
                m_dataTypesFiltered.append(m_dataTypes[i]);
                m_dataIsUserEventFiltered.append(m_dataIsUserEvent[i]);
                m_dataGroupFiltered.append(m_dataGroup[i]);
            }
        }
        m_iLastTypeAdded = eventType.toInt();
    }

    emit dataChanged(createIndex(0,0), createIndex(m_dataSamplesFiltered.size(), 0));
    emit headerDataChanged(Qt::Vertical, 0, m_dataSamplesFiltered.size());
}

//=============================================================================================================

Qt::ItemFlags EventModel::flags(const QModelIndex &index) const
{
    Q_UNUSED(index);
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
}

//=============================================================================================================

QVariant EventModel::headerData(int section,
                                     Qt::Orientation orientation,
                                     int role) const
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
//            case 2: //event type column
//                return QVariant("Group");
            }
    }
    else if(orientation == Qt::Vertical) {
        return QString(" %1 ").arg(section);
    }

    return QVariant();
}

//=============================================================================================================

bool EventModel::removeRows(int position,
                                 int span,
                                 const QModelIndex &parent)
{
    Q_UNUSED(parent);

    beginRemoveRows(QModelIndex(), position, position+span-1);

    auto events = m_EventManager.getEventsInGroups(m_selectedEventGroups);

    for (int i = 0; i < span; ++i) {
        m_EventManager.deleteEvent(events->at(position + i).id);
    } 

    endRemoveRows();

    return true;
}


//=============================================================================================================

void EventModel::setFirstLastSample(int firstSample,
                                         int lastSample)
{
    m_iFirstSample = firstSample;
    m_iLastSample = lastSample;
}

//=============================================================================================================

QPair<int,int> EventModel::getFirstLastSample() const
{
    QPair<int, int> pair(m_iFirstSample, m_iLastSample);
    return pair;
}

//=============================================================================================================

float EventModel::getSampleFreq() const
{
    return m_fFreq;
}

//=============================================================================================================

void EventModel::setSampleFreq(float fFreq)
{
    m_fFreq = fFreq;
}

//=============================================================================================================

int EventModel::getNumberOfEvents() const
{
    return rowCount();
}

//=============================================================================================================

int EventModel::getEvent(int iIndex) const
{
    return m_EventManager.getAllEvents()->at(iIndex).sample;
}

//=============================================================================================================

void EventModel::addNewAnnotationType(const QString &eventType,
                                           const QColor &typeColor)
{
    m_eventTypeColor[eventType.toInt()] = typeColor;

    if(!m_eventTypeList.contains(eventType))
        m_eventTypeList<<eventType;

    m_iLastTypeAdded = eventType.toInt();

    emit updateEventTypes(eventType);
}

//=============================================================================================================

QMap<int, QColor>& EventModel::getTypeColors()
{
    return m_eventTypeColor;
}

//=============================================================================================================

QMap<int, QColor>& EventModel::getGroupColors()
{
    return m_eventGroupColor;
}

//=============================================================================================================

void EventModel::setSelectedAnn(int iSelected)
{
    m_iSelectedAnn = iSelected;
}

//=============================================================================================================

void EventModel::setShowSelected(int iSelectedState)
{
    m_iSelectedCheckState = iSelectedState;
}

//=============================================================================================================

int EventModel::getShowSelected()
{
    return m_iSelectedCheckState;
}

//=============================================================================================================

int EventModel::getSelectedAnn()
{
    return m_iSelectedAnn;
}

//=============================================================================================================

float EventModel::getFreq()
{
    return m_fFreq;
}

//=============================================================================================================

bool EventModel::saveToFile(const QString& sPath)
{
    #ifdef WASMBUILD
    //QBuffer* bufferOut = new QBuffer;
    QByteArray* bufferOut = new QByteArray;

    QTextStream out(bufferOut, QIODevice::ReadWrite);
    auto events = m_EventManager.getEventsInGroups(m_selectedEventGroups);
    for (auto event : *events){
        out << "  " << event.sample << "   " << QString::number(static_cast<float>(event.sample - m_pFiffModel->absoluteFirstSample()) / this->getFreq(), 'f', 4) << "          0         1" << endl;
    }

    // Wee need to call the QFileDialog here instead of the data load plugin since we need access to the QByteArray
    QFileDialog::saveFileContent(bufferOut->data(), "events.eve");

   // bufferOut->deleteLater();

    return true;
    #else
    qInfo() << "AnnotationSettingsView::saveToFile";

    QFile file(sPath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "[AnnotationModel::saveToFile] Unable to access file.";
        return false;
    }

    QTextStream out(&file);
    auto events = m_EventManager.getEventsInGroups(m_selectedEventGroups);
    for (auto event : *events){
        out << "  " << event.sample << "   " << QString::number(static_cast<float>(event.sample - m_pFiffModel->absoluteFirstSample()) / this->getFreq(), 'f', 4) << "          0         1" << endl;
//        out << "  " << iEvent << "   " << QString::number(static_cast<float>(iEvent - m_pFiffModel->absoluteFirstSample()) / this->getFreq(), 'f', 4) << "          1         0" << endl;
    }

    return true;
    #endif
}

//=============================================================================================================

void EventModel::setLastType(int iType)
{
    m_iLastTypeAdded = iType;
}

//=============================================================================================================

void EventModel::updateFilteredSample(int iIndex,
                                           int iSample)
{
    m_dataSamplesFiltered[iIndex] = iSample + m_iFirstSample;
}

//=============================================================================================================

void EventModel::updateFilteredSample(int iSample)
{
    m_dataSamplesFiltered[m_iSelectedAnn] = iSample + m_iFirstSample;
}

//=============================================================================================================

void EventModel::clearSelected()
{
    m_dataSelectedRows.clear();
}

//=============================================================================================================

void EventModel::appendSelected(int iSelectedIndex)
{
    m_dataSelectedRows.append(iSelectedIndex);
}

//=============================================================================================================

MatrixXi EventModel::getAnnotationMatrix(int iGroup)
{
    MatrixXi matEventDataMatrix;


    if(iGroup == 9999){
        //Current selecting in Event Plugin

        auto events = m_EventManager.getEventsInGroups(m_selectedEventGroups);

        matEventDataMatrix.resize(events->size(), 3);
        for (int i = 0; i < events->size(); i++){
            matEventDataMatrix(i,0) = events->at(i).sample;
            matEventDataMatrix(i,1) = 0;
            matEventDataMatrix(i,2) = 1;
        }
    } else {
        auto events = m_EventManager.getEventsInGroup(iGroup);

        matEventDataMatrix.resize(events->size(), 3);
        for (int i = 0; i < events->size(); i++){
            matEventDataMatrix(i,0) = events->at(i).sample;
            matEventDataMatrix(i,1) = 0;
            matEventDataMatrix(i,2) = 1;
        }
    }

    return matEventDataMatrix;
}

//=============================================================================================================

int EventModel::createGroup(const QString& sGroupName,
                                 bool bIsUserMade,
                                 int iType,
                                 const QColor &typeColor)
{
    EventGroup* newEvent = new EventGroup();

    *newEvent = {m_iIndexCount,                 //groupNumber
                 iType,                         //groupType
                 sGroupName,                    //groupName
                 bIsUserMade,                   //isUserMade
                 QVector<int>(),                //dataSamples
                 QVector<int>(),                //dataTypes
                 QVector<int>(),                //dataIsUserEvent
                 QVector<int>(),                //dataSamples_Filtered
                 QVector<int>(),                //dataTypes_Filtered
                 QVector<int>()};               //dataIsUserEvent_Filtered

    m_mAnnotationHub.insert(m_iIndexCount,
                            newEvent);

    m_eventGroupColor[m_iIndexCount] = typeColor;

    return m_iIndexCount++;
}

//=============================================================================================================

void EventModel::switchGroup(int iGroupIndex)
{
    beginResetModel();

    if ((!m_dataSamples.isEmpty()) && (m_iSelectedGroup != ALLGROUPS)){
        saveGroup();
    }

    m_dataSamples = m_mAnnotationHub[iGroupIndex]->dataSamples;
    m_dataTypes = m_mAnnotationHub[iGroupIndex]->dataTypes;
    m_dataIsUserEvent = m_mAnnotationHub[iGroupIndex]->dataIsUserEvent;

    m_dataSamplesFiltered = m_mAnnotationHub[iGroupIndex]->dataSamples_Filtered;
    m_dataTypesFiltered = m_mAnnotationHub[iGroupIndex]->dataTypes_Filtered;
    m_dataIsUserEventFiltered = m_mAnnotationHub[iGroupIndex]->dataIsUserEvent_Filtered;

    m_iSelectedGroup = m_mAnnotationHub[iGroupIndex]->groupNumber;
    m_bIsUserMade = m_mAnnotationHub[iGroupIndex]->isUserMade;
    m_iType = m_mAnnotationHub[iGroupIndex]->groupType;

    m_dataGroup.clear();
    for(int i = 0; i < m_mAnnotationHub[iGroupIndex]->dataSamples.size(); i++){
        m_dataGroup.append(m_iSelectedGroup);
    }

    endResetModel();
}

//=============================================================================================================

bool EventModel::isUserMade()
{
    return m_bIsUserMade;
}

//=============================================================================================================

int EventModel::getHubSize()
{
    return m_mAnnotationHub.size();
}

//=============================================================================================================

bool EventModel::getHubUserMade(int iIndex)
{
    return m_mAnnotationHub[iIndex]->isUserMade;
}

//=============================================================================================================

void EventModel::showAll(bool bSet)
{
    beginResetModel();

    if (bSet) {
        if ((!m_dataSamples.isEmpty()) && (m_iSelectedGroup != ALLGROUPS)){
            saveGroup();
        }

        m_iSelectedGroup = ALLGROUPS;

        resetSelection();
        loadAllGroups();
    }

    endResetModel();
}

//=============================================================================================================

void EventModel::loadAllGroups()
{
    for (EventGroup* e : m_mAnnotationHub) {
        m_dataSamples.append(e->dataSamples);
        m_dataTypes.append(e->dataTypes);
        m_dataIsUserEvent.append(e->dataIsUserEvent);

        m_dataSamplesFiltered.append(e->dataSamples_Filtered);
        m_dataTypesFiltered.append(e->dataTypes_Filtered);
        m_dataIsUserEventFiltered = e->dataIsUserEvent_Filtered;

        for(int i = 0; i < e->dataSamples.size(); i++){
            m_dataGroup.append(e->groupNumber);
        }
    }
}

//=============================================================================================================

void EventModel::resetSelection()
{
    m_dataSamples.clear();
    m_dataTypes.clear();
    m_dataIsUserEvent.clear();

    m_dataSamplesFiltered.clear();
    m_dataTypesFiltered.clear();
    m_dataIsUserEventFiltered.clear();

    m_dataGroup.clear();
}

//=============================================================================================================

void EventModel::hideAll()
{
    beginResetModel();
    resetSelection();
    endResetModel();
}

//=============================================================================================================

int EventModel::getIndexCount(){
    return m_iIndexCount;
}

//=============================================================================================================

void EventModel::removeGroup(int iGroupIndex)
{
    beginResetModel();
    resetSelection();
    m_mAnnotationHub.remove(iGroupIndex);
    endResetModel();
}

//=============================================================================================================

int EventModel::currentGroup(int iIndex)
{
    if (m_iSelectedCheckState){
        return m_dataGroup[m_dataSelectedRows.at(iIndex)];
    } else {
        return m_dataGroup[iIndex];
    }
}

//=============================================================================================================

void EventModel::pushGroup(QListWidgetItem *item)
{
    m_dataStoredGroups.push(item);
}

//=============================================================================================================

QListWidgetItem* EventModel::popGroup()
{
    if(!m_dataStoredGroups.isEmpty()){
        return m_dataStoredGroups.pop();
    } else {
        return Q_NULLPTR;
    }
}

//=============================================================================================================

int EventModel::getGroupStackSize()
{
    return m_dataStoredGroups.size();
}

//=============================================================================================================

void EventModel::setGroupColor(const QColor& groupColor)
{
    for(int group : m_selectedEventGroups){

        int red, green, blue;
        groupColor.getRgb(&red, &green, &blue);
        m_EventManager.setGroupColor(group, EVENTSLIB::RgbColor(red, green, blue));
    }
    emit eventGroupsUpdated();
}

//=============================================================================================================

void EventModel::setGroupName(int iGroupIndex,
                                   const QString &sGroupName)
{
    m_EventManager.renameGroup(iGroupIndex, sGroupName.toStdString());
    emit eventGroupsUpdated();
}

//=============================================================================================================

void EventModel::setSelectedGroupName(const QString &currentText)
{
    m_EventManager.renameGroup(m_selectedEventGroups.front(), currentText.toStdString());
}

//=============================================================================================================

QString EventModel::getGroupName(int iMapKey)
{
    if(!m_mAnnotationHub.contains(iMapKey)){
        qWarning() << "[AnnotationModel::getGroupName] Attempting to get name of group with invalid key.";
        return "NAME NOT FOUND";
    }

    return m_mAnnotationHub[iMapKey]->groupName;
}

//=============================================================================================================

QString EventModel::getGroupNameFromList(int iListIndex)
{
    if(m_mAnnotationHub.keys().size() <= iListIndex){
        qWarning() << "[AnnotationModel::getGroupNameFromList] Attempting to get name of group with invalid key.";
        return "NAME NOT FOUND";
    }

    return m_mAnnotationHub[m_mAnnotationHub.keys()[iListIndex]]->groupName;
}
//=============================================================================================================

int EventModel::getIndexFromName(const QString &sGroupName)
{
    for(EventGroup* group : m_mAnnotationHub){
        if(group->groupName == sGroupName){
            return group->groupNumber;
        }
    }
    return 9999;
}

//=============================================================================================================

void EventModel::saveGroup()
{
    m_mAnnotationHub[m_iSelectedGroup]->dataSamples = m_dataSamples;
    m_mAnnotationHub[m_iSelectedGroup]->dataTypes = m_dataTypes;
    m_mAnnotationHub[m_iSelectedGroup]->dataIsUserEvent = m_dataIsUserEvent;

    m_mAnnotationHub[m_iSelectedGroup]->dataSamples_Filtered = m_dataSamplesFiltered;
    m_mAnnotationHub[m_iSelectedGroup]->dataTypes_Filtered = m_dataTypesFiltered;
    m_mAnnotationHub[m_iSelectedGroup]->dataIsUserEvent_Filtered = m_dataIsUserEventFiltered;
}

//=============================================================================================================

void EventModel::setFiffModel(QSharedPointer<FiffRawViewModel> pModel)
{
    m_pFiffModel = pModel;
}

//=============================================================================================================

QSharedPointer<FiffRawViewModel> EventModel::getFiffModel()
{
    return m_pFiffModel;
}

//=============================================================================================================

void EventModel::initModel()
{
//    m_eventTypeList<<"0";

//    m_eventTypeColor[0] = QColor(Qt::black);
//    m_eventTypeColor[1] = QColor(Qt::black);
//    m_eventTypeColor[2] = QColor(Qt::magenta);
//    m_eventTypeColor[3] = QColor(Qt::green);
//    m_eventTypeColor[4] = QColor(Qt::red);
//    m_eventTypeColor[5] = QColor(Qt::cyan);
//    m_eventTypeColor[32] = QColor(Qt::yellow);
//    m_eventTypeColor[998] = QColor(Qt::darkBlue);
//    m_eventTypeColor[999] = QColor(Qt::darkCyan);

//    m_eventGroupColor[0] = QColor(Qt::black);
//    m_eventGroupColor[1] = QColor(Qt::black);
//    m_eventGroupColor[2] = QColor(Qt::magenta);
//    m_eventGroupColor[3] = QColor(Qt::green);
//    m_eventGroupColor[4] = QColor(Qt::red);
//    m_eventGroupColor[5] = QColor(Qt::cyan);
//    m_eventGroupColor[32] = QColor(Qt::yellow);
//    m_eventGroupColor[998] = QColor(Qt::darkBlue);
//    m_eventGroupColor[999] = QColor(Qt::darkCyan);

    //m_EventManager.initSharedMemory();
    m_bIsInit = true;
}

//=============================================================================================================

void EventModel::initFromFile(const QString& sFilePath)
{
    QFileInfo fileInfo(sFilePath);

    Eigen::MatrixXi eventList;

    if(fileInfo.exists() && (fileInfo.completeSuffix() == "eve")){
        QFile file(sFilePath);       
        MNELIB::MNE::read_events_from_ascii(file, eventList); 
    } else if(fileInfo.exists() && (fileInfo.completeSuffix() == "fif")){
        QFile file(sFilePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)){
            return;
        }
        MNELIB::MNE::read_events_from_fif(file, eventList);
    } else {
        return;
    }

    addGroup(fileInfo.baseName(),
             QColor("red"));

    for(int i = 0; i < eventList.size(); i++){
        addEvent(eventList(i,0));
    }

}

//=============================================================================================================

void EventModel::applyOffset(int iFirstSampleOffset)
{
    for (int i = 0; i < m_dataSamples.size(); i++){
        if (iFirstSampleOffset <= m_dataSamples[i]){
            m_dataSamples[i] -= iFirstSampleOffset;
        } else {
            qWarning() << "[AnnotationModel::applyOffset] Offset greater than event sample";
        }
    }

    //Update data to be diplayed
    setEventFilterType(m_sFilterEventType);
}

//=============================================================================================================

void EventModel::addEvent(int iSample)
{
    setSamplePos(iSample);
    insertRow(0, QModelIndex());
}

//=============================================================================================================

void EventModel::addGroup(QString sName, QColor color)
{
    qDebug() << "EventModel::addGroup";

    int red, green, blue;
    color.getRgb(&red, &green, &blue);

    auto newGroup = m_EventManager.addGroup(sName.toStdString(), EVENTSLIB::RgbColor(red, green, blue));

    m_selectedEventGroups.clear();
    m_selectedEventGroups.push_back(newGroup.id);

    emit eventGroupsUpdated();
    emit eventsUpdated();
}

//=============================================================================================================

std::unique_ptr<std::vector<EVENTSLIB::Event> > EventModel::getEventsToDraw(int iBegin, int iEnd) const
{
    return m_EventManager.getEventsBetween(iBegin, iEnd, m_selectedEventGroups);
}

//=============================================================================================================

void EventModel::eventsUpdated()
{
    emit dataChanged(createIndex(0,0), createIndex(rowCount(), columnCount()));
    emit headerDataChanged(Qt::Vertical, 0, m_EventManager.getAllEvents()->size());
}

//=============================================================================================================

std::unique_ptr<std::vector<EVENTSLIB::EventGroup> > EventModel::getGroupsToDraw() const
{
    return m_EventManager.getAllGroups();
}

//=============================================================================================================

void EventModel::clearSelectedGroups()
{
    m_selectedEventGroups.clear();
    eventsUpdated();
}

//=============================================================================================================

void EventModel::addToSelectedGroups(int iGroupId)
{
    m_selectedEventGroups.push_back(iGroupId);
    eventsUpdated();
}

//=============================================================================================================

QColor EventModel::getGroupColor(int iGroupId) const
{
    auto group = m_EventManager.getGroup(iGroupId);
    auto color = group.color;

    return QColor(color.r, color.g, color.b);
}

//=============================================================================================================

const std::vector<idNum> EventModel::getSelectedGroups() const
{
    return m_selectedEventGroups;
}

//=============================================================================================================

void EventModel::deleteSelectedGroups()
{
    if (m_selectedEventGroups.size()){
        for (int groupId : m_selectedEventGroups){
            m_EventManager.deleteEventsInGroup(groupId);
            m_EventManager.deleteGroup(groupId);
        }

        emit eventGroupsUpdated();
        emit eventsUpdated();
    }
}
