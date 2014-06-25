//=============================================================================================================
/**
* @file     sensormodel.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     May, 2014
*
* @section  LICENSE
*
* Copyright (C) 2014, Christoph Dinh and Matti Hamalainen. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that
* the following conditions are met:
*     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
*       following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
*       the following disclaimer in the documentation and/or other materials provided with the distribution.
*     * Neither the name of the Massachusetts General Hospital nor the names of its contributors may be used
*       to endorse or promote products derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MASSACHUSETTS GENERAL HOSPITAL BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*
* @brief    Implementation of the SensorModel Class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "sensormodel.h"
#include "sensoritem.h"


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace XDISPLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

SensorModel::SensorModel(QObject *parent)
: QAbstractTableModel(parent)
, m_iCurrentLayoutId(0)
{
}


//*************************************************************************************************************

SensorModel::SensorModel(QIODevice* device, QObject *parent)
: QAbstractTableModel(parent)
, m_iCurrentLayoutId(0)
{
    if(!this->read(device))
        qWarning() << "Not able to read sensor layout.";
}


//*************************************************************************************************************

void SensorModel::createSelection()
{
    QList<qint32> listSelection = m_qMapSelection.keys(true);
    emit newSelection(listSelection);
}


//*************************************************************************************************************

int SensorModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    if(m_qListSensorLayouts.size() > 0 && m_iCurrentLayoutId < m_qListSensorLayouts.size())
        return m_qListSensorLayouts[m_iCurrentLayoutId].numChannels();
    else
        return 0;
}


//*************************************************************************************************************

int SensorModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return 4;
}


//*************************************************************************************************************

QVariant SensorModel::data(const QModelIndex &index, int role) const
{
    Q_UNUSED(role)
    if (index.isValid()) {
        qint32 row = index.row();//m_qMapIdxRowSelection[index.row()];

        //******** first column (short ChName) ********
        if(index.column() == 0)// && role == Qt::DisplayRole)
            return QVariant(m_qListSensorLayouts[m_iCurrentLayoutId].shortChNames()[row]);

        //******** second column (full ChName) ********
        if(index.column() == 1)// && role == Qt::DisplayRole)
            return QVariant(m_qListSensorLayouts[m_iCurrentLayoutId].fullChNames()[row]);

        //******** third column (Position) ********
        if(index.column() == 2)
            return QVariant(m_qListSensorLayouts[m_iCurrentLayoutId].loc()[row]);

//            switch(role) {
//                case Qt::DisplayRole: {

        //******** fourth column (selected) ********
        if(index.column() == 3)
        {
            QString fullName = m_qListSensorLayouts[m_iCurrentLayoutId].fullChNames()[row];
            qint32 chNum = m_qMapNameId[fullName];
            return QVariant(m_qMapSelection[chNum]);
        }
    }

    return QVariant();
}


//*************************************************************************************************************

bool SensorModel::read(QIODevice* device)
{
    QDomDocument domDocument;

    QString errorStr;
    int errorLine;
    int errorColumn;

    if (!domDocument.setContent(device, true, &errorStr, &errorLine, &errorColumn))
    {
        qCritical() << QString("Parse error at line %1, column %2:\n%3").arg(errorLine).arg(errorColumn).arg(errorStr);
        return false;
    }

    QDomElement root = domDocument.documentElement();
    if (root.tagName() != "MNEXSensorLayout")
    {
        qCritical() << QString("The file is not a MNE-X Sensor Layout file.");
        return false;
    }

    QString t_sDevice = root.attribute("Device", "No Device");

    //
    // Layouts
    //
    QDomElement childSensorsLayouts = root.firstChildElement("SensorLayouts");
    QDomElement childSensors = childSensorsLayouts.firstChildElement("Sensors");
    while (!childSensors.isNull())
    {
        m_qListSensorLayouts.append(SensorLayout::parseSensorLayout(childSensors));
        childSensors = childSensors.nextSiblingElement("Sensors");
    }

    //
    // Groups
    //
    QDomElement childSensorGroups = root.firstChildElement("SensorGroups");
    QDomElement childGroup = childSensorGroups.firstChildElement("Group");
    while (!childGroup.isNull())
    {
        m_qListSensorGroups.append(SensorGroup::parseSensorGroup(childGroup));
        childGroup = childGroup.nextSiblingElement("Group");
    }

//    qDebug() << root.tagName();//domDocument.toString();
//    qDebug() << t_sDevice;

    return true;
}


//*************************************************************************************************************

void SensorModel::applySensorGroup(int id)
{
    QList<qint32> selection;

    for(qint32 i = 0; i < m_qListSensorGroups[id].getChannelNames().size(); ++i)
        selection.append(m_qMapNameId[m_qListSensorGroups[id].getChannelNames()[i]]);

    emit newSelection(selection);
    silentUpdateSelection(selection);
}


//*************************************************************************************************************

void SensorModel::setCurrentLayout(int id)
{
    beginResetModel();

    int oldLayout = m_iCurrentLayoutId;

    if(id >= 0 && id < m_qListSensorLayouts.size())
        m_iCurrentLayoutId = id;

    endResetModel();

    if(oldLayout != m_iCurrentLayoutId)
        emit newLayout();
}


//*************************************************************************************************************

void SensorModel::mapChannelInfo(const QList<XMEASLIB::RealTimeSampleArrayChInfo>& chInfoList)
{
    m_qMapSelection.clear();
    m_qMapNameId.clear();
    for(qint32 i = 0; i < chInfoList.size(); ++i)
    {
        m_qMapSelection.insert(i,true);
        m_qMapNameId.insert(chInfoList.at(i).getChannelName(), i);
    }
}


//*************************************************************************************************************

void SensorModel::updateChannelState(SensorItem* item)
{
    m_qMapSelection[item->getChNumber()] = item->isSelected();
    createSelection();
}


//*************************************************************************************************************

void SensorModel::silentUpdateSelection(const QList<qint32>& selection)
{
    QMap<qint32, bool>::iterator it;
    for (it = m_qMapSelection.begin(); it != m_qMapSelection.end(); ++it)
        it.value() = false;

    for(qint32 i = 0; i < selection.size(); ++i)
        m_qMapSelection[selection[i]] = true;

    //Update data content
    QModelIndex topLeft = this->index(0,3);

    QModelIndex bottomRight = this->index(m_qMapSelection.size()-1,3);

    QVector<int> roles; roles << Qt::DisplayRole;

    emit dataChanged(topLeft, bottomRight, roles);
}
