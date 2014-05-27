#include "sensormodel.h"
#include "sensoritem.h"
#include <QDebug>

SensorModel::SensorModel(QObject *parent)
: QAbstractTableModel(parent)
, m_iCurrentLayoutId(0)
{
}


SensorModel::SensorModel(QIODevice* device, QObject *parent)
: QAbstractTableModel(parent)
, m_iCurrentLayoutId(0)
{
    if(!this->read(device))
        qWarning() << "Not able to read sensor layout.";
}



void SensorModel::createSelection()
{
    QList<qint32> listSelection = m_qMapSelection.keys(true);
    emit newSelection(listSelection);
}


int SensorModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    if(m_qListSensorLayouts.size() > 0 && m_iCurrentLayoutId < m_qListSensorLayouts.size())
        return m_qListSensorLayouts[m_iCurrentLayoutId].numChannels();
    else
        return 0;
}

int SensorModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return 4;
}

QVariant SensorModel::data(const QModelIndex &index, int role) const
{
    if (index.isValid()) {
        qint32 row = index.row();//m_qMapIdxRowSelection[index.row()];

        //******** first column (short ChName) ********
        if(index.column() == 0)// && role == Qt::DisplayRole)
            return QVariant(m_qListSensorLayouts[m_iCurrentLayoutId].shortChNames()[row]);

        //******** second column (full ChName) ********
        if(index.column() == 1)// && role == Qt::DisplayRole)
            return QVariant(m_qListSensorLayouts[m_iCurrentLayoutId].shortChNames()[row]);

        //******** third column (Position) ********
        if(index.column() == 2)
            return QVariant(m_qListSensorLayouts[m_iCurrentLayoutId].loc()[row]);

//            switch(role) {
//                case Qt::DisplayRole: {

        //******** fourth column (selected) ********
        if(index.column() == 3)
            return QVariant(true);
    }

    return QVariant();
}



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




    qDebug() << root.tagName();//domDocument.toString();
    qDebug() << t_sDevice;




    return true;
}



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



void SensorModel::mapChannelInfo(const QList<XMEASLIB::RealTimeSampleArrayChInfo>& chInfoList)
{
    m_qMapSelection.clear();
    m_qMapNameId.clear();
    m_qMapIdName.clear();
    for(qint32 i = 0; i < chInfoList.size(); ++i)
    {
        m_qMapSelection.insert(i,true);
        m_qMapNameId.insert(chInfoList.at(i).getChannelName(), i);
        m_qMapIdName.insert(i, chInfoList.at(i).getChannelName());
    }


    qDebug() << "m_qMapSelection" << m_qMapSelection;
}


void SensorModel::updateChannelState(SensorItem* item)
{
    qDebug() << "SensorModel::updateChannelState" << item->isSelected();
}

