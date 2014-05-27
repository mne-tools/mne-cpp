#include "sensorlayout.h"

#include <QDebug>

SensorLayout::SensorLayout()
{
}


SensorLayout SensorLayout::parseSensorLayout(const QDomElement &sensorLayoutElement)
{
    SensorLayout layout;

    qint32 t_iNumChannels = sensorLayoutElement.attribute("NumChannels", 0).toInt();

    layout.m_sName = sensorLayoutElement.attribute("Type", "");

    QDomElement childSensor = sensorLayoutElement.firstChildElement("Sensor");
    while (!childSensor.isNull()) {
        QString chName = layout.m_sName.isEmpty() ? childSensor.attribute("ChannelNumber") : QString("%1 %2").arg(layout.m_sName).arg(childSensor.attribute("ChannelNumber"));
        layout.m_qListChannels.append(chName);
        layout.m_qListShortChannelNames.append(childSensor.attribute("ChannelNumber"));
        float plot_x = childSensor.attribute("plot_x").toFloat()*5; //mm to pixel
        float plot_y = childSensor.attribute("plot_y").toFloat()*5; //mm to pixel
        layout.m_qListLocations.append(QPointF(plot_x,plot_y));
        childSensor = childSensor.nextSiblingElement("Sensor");
    }

//    qDebug() << "layout.m_qListChannels" << layout.m_qListChannels;
//    qDebug() << "layout.m_qListLocations" << layout.m_qListLocations;

//    if(t_iNumChannels == layout.m_qListChannels.size())
        return layout;
//    else
//    {
//        qWarning() << "Number of channel inconsistency!";
//        return SensorLayout();
//    }
}
