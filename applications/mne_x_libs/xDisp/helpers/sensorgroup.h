#ifndef SENSORGROUP_H
#define SENSORGROUP_H

#include <QDomElement>
#include <QStringList>


class SensorGroup
{
public:
    SensorGroup();

    static SensorGroup parseSensorGroup(const QDomElement &sensorGroupElement);

private:
    QString m_sGroupName;

    QString m_iNumChannels;

    QStringList m_qListChannels;
};

#endif // SENSORGROUP_H
