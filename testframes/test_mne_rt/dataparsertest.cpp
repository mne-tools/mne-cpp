#include "dataparsertest.h"

DataParserTest::DataParserTest(QObject *parent)
: QObject(parent)
, m_commandManager()
{
    QStringList     t_qParamNames;
    QList<QVariant> t_qParamValues;
    QStringList     t_qParamDescription;

    t_qParamNames.push_back("id");
    t_qParamValues.push_back(QVariant(QVariant::String));
    t_qParamDescription.append("ID/Alias");

    m_commandManager.insert("measinfo", Command("measinfo", "sends the measurement info to the specified FiffStreamClient.", t_qParamNames, t_qParamValues, t_qParamDescription));
    m_commandManager.insert("meas", Command("meas", "adds specified FiffStreamClient to raw data buffer receivers. If acquisition is not already strated, it is triggered.", t_qParamNames, t_qParamValues, t_qParamDescription));
    m_commandManager.insert("stop", Command("stop", "removes specified FiffStreamClient from raw data buffer receivers.", t_qParamNames, t_qParamValues, t_qParamDescription));
    t_qParamNames.clear();t_qParamValues.clear();t_qParamDescription.clear();
    m_commandManager.insert(QString("stop-all"), QString("stops the whole acquisition process."));

    m_commandManager.insert(QString("conlist"), QString("prints and sends all available connectors"));

    t_qParamNames.push_back("ConID");
    t_qParamValues.push_back(QVariant(QVariant::Int));
    t_qParamDescription.append("Connector ID");
    m_commandManager.insert("conlist", Command("conlist", "prints and sends all available connectors", t_qParamNames, t_qParamValues, t_qParamDescription));

    m_commandManager.insert(QString("help"), QString("prints and sends this list"));

    m_commandManager.insert(QString("close"), QString("closes mne_rt_server"));


    QObject::connect(&m_commandManager["help"], &Command::executed, this, &DataParserTest::helpReceived);
}
