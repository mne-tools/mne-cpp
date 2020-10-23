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

AnnotationModel::AnnotationModel(QObject* parent)
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

AnnotationModel::AnnotationModel(QSharedPointer<FiffRawViewModel> pFiffModel,
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

AnnotationModel::AnnotationModel(const QString &sFilePath,
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
    initModel();
    initFromFile(sFilePath);
}

//=============================================================================================================

AnnotationModel::~AnnotationModel()
{
    for(EventGroup* eventGroup : m_mAnnotationHub){
        if(eventGroup){
            delete eventGroup;
        }
    }
}

//=============================================================================================================

QStringList AnnotationModel::getEventTypeList() const
{
    return m_eventTypeList;
}

//=============================================================================================================

bool AnnotationModel::insertRows(int position,
                                 int span,
                                 const QModelIndex & parent)
{
    Q_UNUSED(parent);

    if (m_iSelectedGroup == ALLGROUPS){
        return false;
    }

    if(m_dataSamples.isEmpty()) {
        m_dataSamples.insert(0, m_iSamplePos);
        m_dataTypes.insert(0, m_iType);
        m_dataIsUserEvent.insert(0, 1);
        m_dataGroup.insert(0, m_iSelectedGroup);
    }
    else {
        for (int i = 0; i < span; ++i) {
            for(int t = 0; t<m_dataSamples.size(); t++) {
                if(m_dataSamples[t] >= m_iSamplePos) {
                    m_dataSamples.insert(t, m_iSamplePos);

                    if(m_sFilterEventType == "All")
                        m_dataTypes.insert(t, m_iType);
                    else
                        m_dataTypes.insert(t, m_sFilterEventType.toInt());

                    m_dataIsUserEvent.insert(t, 1);
                    m_dataGroup.insert(t, m_iSelectedGroup);
                    break;
                }

                if(t == m_dataSamples.size()-1) {
                    m_dataSamples.append(m_iSamplePos);

                    if(m_sFilterEventType == "All")
                        m_dataTypes.append(m_iType);
                    else
                        m_dataTypes.append(m_sFilterEventType.toInt());

                    m_dataIsUserEvent.append(1);
                    m_dataGroup.append(m_iSelectedGroup);
                    break;
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

//=============================================================================================================

void AnnotationModel::setSamplePos(int iSamplePos)
{
    m_iSamplePos = iSamplePos;
}

//=============================================================================================================

int AnnotationModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_dataSamplesFiltered.size();
}

//=============================================================================================================

int AnnotationModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 3;
}

//=============================================================================================================

QVariant AnnotationModel::data(const QModelIndex &index,
                               int role) const
{
    if(role == Qt::TextAlignmentRole)
        return QVariant(Qt::AlignCenter | Qt::AlignVCenter);

    if(role != Qt::DisplayRole && role != Qt::BackgroundRole)
        return QVariant();

    if(index.row()>=m_dataSamplesFiltered.size())
        return QVariant();

    if (index.isValid()) {
        //******** first column (sample index) ********
        if(index.column()==0) {
            switch(role) {
                case Qt::DisplayRole:
                    return QVariant(m_dataSamplesFiltered.at(index.row())-m_iFirstSample);

            case Qt::BackgroundRole:
                //Paint different background if event was set by user
//                if(m_dataIsUserEventFiltered.at(index.row()) == 1) {
//                    QBrush brush;
//                    brush.setStyle(Qt::SolidPattern);
//                    QColor colorTemp(Qt::red);
//                    colorTemp.setAlpha(15);
//                    brush.setColor(colorTemp);
//                    return QVariant(brush);
//                }
                QBrush brush;
                brush.setStyle(Qt::SolidPattern);
                brush.setColor(Qt::white);

                QColor colorTemp = brush.color();
                colorTemp.setAlpha(110);
                brush.setColor(colorTemp);
                return QVariant(brush);
            }
        }

        //******** second column (event time plot) ********
        if(index.column()==1){
            switch(role) {
                case Qt::DisplayRole: {
                    int time = ((m_dataSamplesFiltered.at(index.row()) - m_iFirstSample) / m_fFreq) * 1000;

                    return QVariant((double)time / 1000);
                }

            case Qt::BackgroundRole:
                //Paint different background if event was set by user
//                if(m_dataIsUserEventFiltered.at(index.row()) == 1) {
//                    QBrush brush;
//                    brush.setStyle(Qt::SolidPattern);
//                    QColor colorTemp(Qt::red);
//                    colorTemp.setAlpha(15);
//                    brush.setColor(colorTemp);
//                    return QVariant(brush);
//                }
                QBrush brush;
                brush.setStyle(Qt::SolidPattern);
                brush.setColor(Qt::white);

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
                    return QVariant(m_dataTypesFiltered.at(index.row()));

                case Qt::BackgroundRole: {
                    QBrush brush;
                    brush.setStyle(Qt::SolidPattern);
                    brush.setColor(Qt::white);

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

bool AnnotationModel::setData(const QModelIndex &index,
                              const QVariant &value,
                              int role)
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
                m_dataSamples[index.row()] = value.toDouble() * m_fFreq + m_iFirstSample;
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

//=============================================================================================================

void AnnotationModel::setEventFilterType(const QString eventType)
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

Qt::ItemFlags AnnotationModel::flags(const QModelIndex &index) const
{
    //Return editable mode only for user events an when event type filtering is deactivated
    if(m_dataIsUserEventFiltered[index.row()] == 1 && m_sFilterEventType == "All")
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
    else
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable;}

//=============================================================================================================

QVariant AnnotationModel::headerData(int section,
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
            case 2: //event type column
                return QVariant("Type");
            }
    }
    else if(orientation == Qt::Vertical) {
        return QString(" %1 ").arg(section);
    }

    return QVariant();
}

//=============================================================================================================

bool AnnotationModel::removeRows(int position,
                                 int span,
                                 const QModelIndex &parent)
{
    Q_UNUSED(parent);

    beginRemoveRows(QModelIndex(), position, position+span-1);

    for (int i = 0; i < span; ++i) {
        //Only user events can be deleted
        if(m_dataIsUserEvent[position] == 1) {
            m_dataSamples.removeAt(position);
            m_dataTypes.removeAt(position);
            m_dataIsUserEvent.removeAt(position);
        }
    } 

    endRemoveRows();

    //Update filtered event data
    setEventFilterType(m_sFilterEventType);

    return true;
}


//=============================================================================================================

void AnnotationModel::setFirstLastSample(int firstSample,
                                         int lastSample)
{
    m_iFirstSample = firstSample;
    m_iLastSample = lastSample;
}

//=============================================================================================================

QPair<int,int> AnnotationModel::getFirstLastSample() const
{
    QPair<int, int> pair(m_iFirstSample, m_iLastSample);
    return pair;
}

//=============================================================================================================

float AnnotationModel::getSampleFreq() const
{
    return m_fFreq;
}

//=============================================================================================================

void AnnotationModel::setSampleFreq(float fFreq)
{
    m_fFreq = fFreq;
}

//=============================================================================================================

int AnnotationModel::getNumberOfAnnotations() const
{
    if (m_iSelectedCheckState){
        return m_dataSelectedRows.size();
    } else {
        return m_dataSamplesFiltered.size();
    }
}

//=============================================================================================================

int AnnotationModel::getAnnotation(int iIndex) const
{
    if (m_iSelectedCheckState){
        return m_dataSamplesFiltered.at(m_dataSelectedRows.at(iIndex));
    } else {
        return m_dataSamplesFiltered.at(iIndex);
    }
}

//=============================================================================================================

void AnnotationModel::addNewAnnotationType(const QString &eventType,
                                           const QColor &typeColor)
{
    m_eventTypeColor[eventType.toInt()] = typeColor;

    if(!m_eventTypeList.contains(eventType))
        m_eventTypeList<<eventType;

    m_iLastTypeAdded = eventType.toInt();

    emit updateEventTypes(eventType);
}

//=============================================================================================================

QMap<int, QColor>& AnnotationModel::getTypeColors()
{
    return m_eventTypeColor;
}

//=============================================================================================================

QMap<int, QColor>& AnnotationModel::getGroupColors()
{
    return m_eventGroupColor;
}

//=============================================================================================================

void AnnotationModel::setSelectedAnn(int iSelected)
{
    m_iSelectedAnn = iSelected;
}

//=============================================================================================================

void AnnotationModel::setShowSelected(int iSelectedState)
{
    m_iSelectedCheckState = iSelectedState;
}

//=============================================================================================================

int AnnotationModel::getShowSelected()
{
    return m_iSelectedCheckState;
}

//=============================================================================================================

int AnnotationModel::getSelectedAnn()
{
    return m_iSelectedAnn;
}

//=============================================================================================================

float AnnotationModel::getFreq()
{
    return m_fFreq;
}

//=============================================================================================================

bool AnnotationModel::saveToFile(const QString& sPath)
{
    #ifdef WASMBUILD
    //QBuffer* bufferOut = new QBuffer;
    QByteArray* bufferOut = new QByteArray;

    QTextStream out(bufferOut, QIODevice::ReadWrite);
    for(int i = 0; i < this->getNumberOfAnnotations(); i++) {
        out << "  " << this->getAnnotation(i) << "   " << QString::number(static_cast<float>(this->getAnnotation(i)) / this->getFreq(), 'f', 4) << "          0         1" << endl;
        out << "  " << this->getAnnotation(i) << "   " << QString::number(static_cast<float>(this->getAnnotation(i)) / this->getFreq(), 'f', 4) << "          1         0" << endl;
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
    for(int i = 0; i < this->getNumberOfAnnotations(); i++) {
        int iAnnotation = this->getAnnotation(i);
        out << "  " << iAnnotation << "   " << QString::number(static_cast<float>(iAnnotation - m_pFiffModel->absoluteFirstSample()) / this->getFreq(), 'f', 4) << "          0         1" << endl;
        out << "  " << iAnnotation << "   " << QString::number(static_cast<float>(iAnnotation - m_pFiffModel->absoluteFirstSample()) / this->getFreq(), 'f', 4) << "          1         0" << endl;
    }
    return true;
    #endif
}

//=============================================================================================================

void AnnotationModel::setLastType(int iType)
{
    m_iLastTypeAdded = iType;
}

//=============================================================================================================

void AnnotationModel::updateFilteredSample(int iIndex,
                                           int iSample)
{
    m_dataSamplesFiltered[iIndex] = iSample + m_iFirstSample;
}

//=============================================================================================================

void AnnotationModel::updateFilteredSample(int iSample)
{
    m_dataSamplesFiltered[m_iSelectedAnn] = iSample + m_iFirstSample;
}

//=============================================================================================================

void AnnotationModel::clearSelected()
{
    m_dataSelectedRows.clear();
}

//=============================================================================================================

void AnnotationModel::appendSelected(int iSelectedIndex)
{
    m_dataSelectedRows.append(iSelectedIndex);
}

//=============================================================================================================

MatrixXi AnnotationModel::getAnnotationMatrix(int iGroup)
{
    MatrixXi matEventDataMatrix;

    if(iGroup == 9999){
        //Current selecting in Event Plugin
        matEventDataMatrix.resize(getNumberOfAnnotations(), 3);
        for (int i = 0; i < getNumberOfAnnotations(); i++){
            matEventDataMatrix(i,0) = getAnnotation(i);
            matEventDataMatrix(i,1) = 0;
            matEventDataMatrix(i,2) = 1;
        }
    } else {
        //User selection on dropdown
        if(iGroup == m_iSelectedGroup){
            saveGroup();
        }
        matEventDataMatrix.resize(m_mAnnotationHub[iGroup]->dataSamples_Filtered.size(), 3);
        for (int i = 0; i < m_mAnnotationHub[iGroup]->dataSamples_Filtered.size(); i++){
            matEventDataMatrix(i,0) = m_mAnnotationHub[iGroup]->dataSamples_Filtered[i];
            matEventDataMatrix(i,1) = 0;
            matEventDataMatrix(i,2) = 1;
        }

    }

    return matEventDataMatrix;
}

//=============================================================================================================

int AnnotationModel::createGroup(const QString& sGroupName,
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

void AnnotationModel::switchGroup(int iGroupIndex)
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

bool AnnotationModel::isUserMade()
{
    return m_bIsUserMade;
}

//=============================================================================================================

int AnnotationModel::getHubSize()
{
    return m_mAnnotationHub.size();
}

//=============================================================================================================

bool AnnotationModel::getHubUserMade(int iIndex)
{
    return m_mAnnotationHub[iIndex]->isUserMade;
}

//=============================================================================================================

void AnnotationModel::showAll(bool bSet)
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

void AnnotationModel::loadAllGroups()
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

void AnnotationModel::resetSelection()
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

void AnnotationModel::hideAll()
{
    beginResetModel();
    resetSelection();
    endResetModel();
}

//=============================================================================================================

int AnnotationModel::getIndexCount(){
    return m_iIndexCount;
}

//=============================================================================================================

void AnnotationModel::removeGroup(int iGroupIndex)
{
    beginResetModel();
    resetSelection();
    m_mAnnotationHub.remove(iGroupIndex);
    endResetModel();
}

//=============================================================================================================

int AnnotationModel::currentGroup(int iIndex)
{
    //return m_dataGroup[iIndex];
    if (m_iSelectedCheckState){
        return m_dataGroup[m_dataSelectedRows.at(iIndex)];
    } else {
        return m_dataGroup[iIndex];
    }
}

//=============================================================================================================

void AnnotationModel::pushGroup(QListWidgetItem *item)
{
    m_dataStoredGroups.push(item);
}

//=============================================================================================================

QListWidgetItem* AnnotationModel::popGroup()
{
    if(!m_dataStoredGroups.isEmpty()){
        return m_dataStoredGroups.pop();
    } else {
        return Q_NULLPTR;
    }
}

//=============================================================================================================

int AnnotationModel::getGroupStackSize()
{
    return m_dataStoredGroups.size();
}

//=============================================================================================================

void AnnotationModel::setGroupColor(int iGroupIndex,
                                    const QColor& groupColor)
{
    m_eventGroupColor[iGroupIndex] = groupColor;
}

//=============================================================================================================

void AnnotationModel::setGroupName(int iGroupIndex,
                                   const QString &sGroupName)
{
    m_mAnnotationHub[iGroupIndex]->groupName = sGroupName;
}

//=============================================================================================================

QString AnnotationModel::getGroupName(int iMapKey)
{
    if(!m_mAnnotationHub.contains(iMapKey)){
        qWarning() << "[AnnotationModel::getGroupName] Attempting to get name of group with invalid key.";
        return "NAME NOT FOUND";
    }

    return m_mAnnotationHub[iMapKey]->groupName;
}

//=============================================================================================================

QString AnnotationModel::getGroupNameFromList(int iListIndex)
{
    if(m_mAnnotationHub.keys().size() <= iListIndex){
        qWarning() << "[AnnotationModel::getGroupNameFromList] Attempting to get name of group with invalid key.";
        return "NAME NOT FOUND";
    }

    return m_mAnnotationHub[m_mAnnotationHub.keys()[iListIndex]]->groupName;
}
//=============================================================================================================

int AnnotationModel::getIndexFromName(const QString &sGroupName)
{
    for(EventGroup* group : m_mAnnotationHub){
        if(group->groupName == sGroupName){
            return group->groupNumber;
        }
    }
    return 9999;
}

//=============================================================================================================

void AnnotationModel::saveGroup()
{
    m_mAnnotationHub[m_iSelectedGroup]->dataSamples = m_dataSamples;
    m_mAnnotationHub[m_iSelectedGroup]->dataTypes = m_dataTypes;
    m_mAnnotationHub[m_iSelectedGroup]->dataIsUserEvent = m_dataIsUserEvent;

    m_mAnnotationHub[m_iSelectedGroup]->dataSamples_Filtered = m_dataSamplesFiltered;
    m_mAnnotationHub[m_iSelectedGroup]->dataTypes_Filtered = m_dataTypesFiltered;
    m_mAnnotationHub[m_iSelectedGroup]->dataIsUserEvent_Filtered = m_dataIsUserEventFiltered;
}

//=============================================================================================================

void AnnotationModel::setFiffModel(QSharedPointer<FiffRawViewModel> pModel)
{
    m_pFiffModel = pModel;
}

//=============================================================================================================

QSharedPointer<FiffRawViewModel> AnnotationModel::getFiffModel()
{
    return m_pFiffModel;
}

//=============================================================================================================

void AnnotationModel::initModel()
{
    m_eventTypeList<<"0";

    m_eventTypeColor[0] = QColor(Qt::black);
    m_eventTypeColor[1] = QColor(Qt::black);
    m_eventTypeColor[2] = QColor(Qt::magenta);
    m_eventTypeColor[3] = QColor(Qt::green);
    m_eventTypeColor[4] = QColor(Qt::red);
    m_eventTypeColor[5] = QColor(Qt::cyan);
    m_eventTypeColor[32] = QColor(Qt::yellow);
    m_eventTypeColor[998] = QColor(Qt::darkBlue);
    m_eventTypeColor[999] = QColor(Qt::darkCyan);

    m_eventGroupColor[0] = QColor(Qt::black);
    m_eventGroupColor[1] = QColor(Qt::black);
    m_eventGroupColor[2] = QColor(Qt::magenta);
    m_eventGroupColor[3] = QColor(Qt::green);
    m_eventGroupColor[4] = QColor(Qt::red);
    m_eventGroupColor[5] = QColor(Qt::cyan);
    m_eventGroupColor[32] = QColor(Qt::yellow);
    m_eventGroupColor[998] = QColor(Qt::darkBlue);
    m_eventGroupColor[999] = QColor(Qt::darkCyan);

    m_bIsInit = true;
}

//=============================================================================================================

void AnnotationModel::initFromFile(const QString& sFilePath)
{
    QFileInfo fileInfo(sFilePath);

    if(fileInfo.exists() && (fileInfo.completeSuffix() == "eve")){
        QFile file(sFilePath);

        int iGroupIndex = createGroup(fileInfo.baseName(),
                    false,
                    1,
                    QColor("red"));

        switchGroup(iGroupIndex);

        QListWidgetItem* newItem = new QListWidgetItem(fileInfo.baseName());
        newItem->setData(Qt::UserRole, QVariant(iGroupIndex));
        newItem->setData(Qt::DecorationRole, QColor("red"));
        newItem->setFlags (newItem->flags () | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        pushGroup(newItem);

        Eigen::MatrixXi eventList;
        MNELIB::MNE::read_events_from_ascii(file, eventList);

        for(int i = 0; i < eventList.size(); i++){
            setSamplePos(eventList(i,0));
            insertRow(0, QModelIndex());
        }

    } else if(fileInfo.exists() && (fileInfo.completeSuffix() == "fif")){
        QFile file(sFilePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)){
            return;
        }
        Eigen::MatrixXi eventlist;
        MNELIB::MNE::read_events_from_fif(file, eventlist);
    }
}

//=============================================================================================================

void AnnotationModel::applyOffset(int iFirstSampleOffset)
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
