#ifndef SENSORGROUP_H
#define SENSORGROUP_H

#include <QtXml/QDomElement>
#include <QStringList>


class SensorGroup
{
public:
    SensorGroup();

    static SensorGroup parseSensorGroup(const QDomElement &sensorGroupElement);

    inline const QString& getGroupName() const;

private:
    QString m_sGroupName;

    QString m_iNumChannels;

    QStringList m_qListChannels;
};

inline const QString& SensorGroup::getGroupName() const
{
    return m_sGroupName;
}

#endif // SENSORGROUP_H
