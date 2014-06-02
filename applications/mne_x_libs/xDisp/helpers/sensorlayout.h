#ifndef SENSORLAYOUT_H
#define SENSORLAYOUT_H

#include <QList>
#include <QStringList>
#include <QPointF>
#include <QtXml/QDomElement>

class SensorLayout
{
public:
    SensorLayout();

    static SensorLayout parseSensorLayout(const QDomElement &sensorLayoutElement);


    inline const QString& getName() const;

    inline QStringList fullChNames() const;

    inline QStringList shortChNames() const;

    inline qint32 numChannels() const;


    inline QList<QPointF> loc() const;


private:

    QString m_sName;

    QStringList m_qListFullChannelNames;
    QStringList m_qListShortChannelNames;
    QList<QPointF> m_qListLocations;

};


inline const QString& SensorLayout::getName() const
{
    return m_sName;
}

inline QStringList SensorLayout::fullChNames() const
{
    return m_qListFullChannelNames;
}


inline QStringList SensorLayout::shortChNames() const
{
    return m_qListShortChannelNames;
}

inline qint32 SensorLayout::numChannels() const
{
    return m_qListFullChannelNames.size();
}

inline QList<QPointF> SensorLayout::loc() const
{
    return m_qListLocations;
}



#endif // SENSORLAYOUT_H
