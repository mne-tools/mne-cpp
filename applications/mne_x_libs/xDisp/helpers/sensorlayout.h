#ifndef SENSORLAYOUT_H
#define SENSORLAYOUT_H

#include <QList>
#include <QStringList>
#include <QPointF>
#include <QDomElement>

class SensorLayout
{
public:
    SensorLayout();

    static SensorLayout parseSensorLayout(const QDomElement &sensorLayoutElement);

    inline QStringList channels() const;

    inline QStringList shortChNames() const;

    inline qint32 numChannels() const;


    inline QList<QPointF> loc() const;


private:

    QString m_sType;

    QStringList m_qListChannels;
    QStringList m_qListShortChannelNames;
    QList<QPointF> m_qListLocations;

};


inline QStringList SensorLayout::channels() const
{
    return m_qListChannels;
}


inline QStringList SensorLayout::shortChNames() const
{
    return m_qListShortChannelNames;
}

inline qint32 SensorLayout::numChannels() const
{
    return m_qListChannels.size();
}

inline QList<QPointF> SensorLayout::loc() const
{
    return m_qListLocations;
}



#endif // SENSORLAYOUT_H
