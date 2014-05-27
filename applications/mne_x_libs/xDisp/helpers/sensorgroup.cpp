#include "sensorgroup.h"
#include <QDebug>


SensorGroup::SensorGroup()
{
}

SensorGroup SensorGroup::parseSensorGroup(const QDomElement &sensorGroupElement)
{
    SensorGroup group;

    group.m_sGroupName = sensorGroupElement.attribute("GroupName", "");
    group.m_iNumChannels = sensorGroupElement.attribute("NumChannels", "");
    group.m_qListChannels = sensorGroupElement.text().split(":");

    qDebug() << "group.m_sGroupName" << group.m_sGroupName;
    qDebug() << "group.m_iNumChannels" << group.m_iNumChannels;
    qDebug() << "group.m_qListChannels" << group.m_qListChannels;

    return group;
}
