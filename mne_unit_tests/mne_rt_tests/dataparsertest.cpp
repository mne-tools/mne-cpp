#include "dataparsertest.h"

DataParserTest::DataParserTest(QObject *parent)
: QObject(parent)
, m_commandManager()
{
    QMap<QString, QVariant> t_qMap;
    QList<QString> t_qIdDescription;

    t_qMap.insert("id", QVariant(QVariant::String));
    t_qIdDescription.append("ID/Alias");

    m_commandManager.insert("measinfo", Command("measinfo", "sends the measurement info to the specified FiffStreamClient.", t_qMap, t_qIdDescription));
    m_commandManager.insert("meas", Command("meas", "adds specified FiffStreamClient to raw data buffer receivers. If acquisition is not already strated, it is triggered.", t_qMap, t_qIdDescription));
    m_commandManager.insert("stop", Command("stop", "removes specified FiffStreamClient from raw data buffer receivers.", t_qMap, t_qIdDescription));
    t_qMap.clear();t_qIdDescription.clear();
    m_commandManager.insert(QString("stop-all"), QString("stops the whole acquisition process."));

    m_commandManager.insert(QString("conlist"), QString("prints and sends all available connectors"));

    t_qMap.insert("ConID", QVariant(QVariant::Int));
    t_qIdDescription.append("Connector ID");
    m_commandManager.insert("conlist", Command("conlist", "prints and sends all available connectors", t_qMap, t_qIdDescription));

    m_commandManager.insert(QString("help"), QString("prints and sends this list"));

    m_commandManager.insert(QString("close"), QString("closes mne_rt_server"));


    m_commandManager.connectSlot(QString("help"), this, &DataParserTest::helpReceived);

//    QObject::connect(&m_commandManager["help"], &Command::received, this, &DataParserTest::helpReceived);
}
