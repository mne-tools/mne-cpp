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

    return group;
}
