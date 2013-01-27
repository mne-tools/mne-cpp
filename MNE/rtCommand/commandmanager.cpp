
//*************************************************************************************************************
//=============================================================================================================
// Includes
//=============================================================================================================

#include "commandmanager.h"
#include "rawcommand.h"


//*************************************************************************************************************
//=============================================================================================================
// Qt Includes
//=============================================================================================================

#include <QVector>
#include <QDebug>
#include <QJsonObject>
#include <QStringList>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace RTCOMMANDLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

CommandManager::CommandManager(const QString test, QObject *parent)
: QObject(parent)
{
    m_sTest = test;
    init();
}


//*************************************************************************************************************

CommandManager::CommandManager(const QByteArray &p_jsonDoc, const QString test, QObject *parent)
: QObject(parent)
{
    m_sTest = test;

    init();

    m_jsonDocumentOrigin = QJsonDocument::fromJson(p_jsonDoc);

    insertCommand(m_jsonDocumentOrigin);
}


//*************************************************************************************************************

void CommandManager::testSlot()
{
    qDebug() << "data Updated received " << m_sTest;
}

//*************************************************************************************************************

CommandManager::~CommandManager()
{
    //Disconnect all connections which are created with the help of this manager.
    this->disconnectAll();

    //Remove commands which where inserted into the static command list


}


//*************************************************************************************************************

void CommandManager::disconnectAll()
{
    //Disconnect Slots
    QMap<QString, QMetaObject::Connection>::Iterator it;
    for(it = m_qMapSlots.begin(); it != m_qMapSlots.end(); ++it)
        QObject::disconnect(it.value());
    //Disconnect Signals
    for(it = m_qMapSignals.begin(); it != m_qMapSignals.end(); ++it)
        QObject::disconnect(it.value());
}


//*************************************************************************************************************

void CommandManager::init()
{
    QObject::connect(&m_commandMap, &CommandMap::commandMapChanged, this, &CommandManager::commandMapChanged);

    QObject::connect(this, &CommandManager::commandMapChanged, this, &CommandManager::testSlot);

}


//*************************************************************************************************************
//ToDo connect all commands inserted in this class by default.
void CommandManager::insertCommand(const QJsonDocument &p_jsonDocument)
{
    QJsonObject t_jsonObjectCommand;

    if(p_jsonDocument.isObject() && p_jsonDocument.object().value(QString("commands")) != QJsonValue::Undefined)
        t_jsonObjectCommand = p_jsonDocument.object().value(QString("commands")).toObject();
    else
        return;

    CommandMap t_qCommandMap;
    QJsonObject::Iterator it;
    for(it = t_jsonObjectCommand.begin(); it != t_jsonObjectCommand.end(); ++it)
        t_qCommandMap.insert(it.key(), Command(it.key(), it.value().toObject()));

    //Do insertion in one step to, have only one dataUpdate emmited;
    //Attention overwrites existing items
    m_commandMap.insert(t_qCommandMap);
}


//*************************************************************************************************************

void CommandManager::insertCommand(const CommandMap &p_commandMap)
{
    m_commandMap.insert(p_commandMap);
}

//*************************************************************************************************************

void CommandManager::insertCommand(const QString &p_sKey, const QString &p_sDescription)
{
    m_commandMap.insert(p_sKey, p_sDescription);
}


//*************************************************************************************************************

bool CommandManager::parse(const QString &p_sInput)
{
    if(p_sInput.size() <= 0)
        return false;
    //Check if JSON format;
    bool isJson  = false;
    if(QString::compare(p_sInput.at(0), QString("{")) == 0)
        isJson = true;

    if(isJson)
    {
        Command parsedCommand;
        qDebug() << "JSON commands recognized";
    }
    else
    {
        Command parsedCommand;

        QStringList t_qCommandList = p_sInput.split(" ");

        if(this->hasCommand(t_qCommandList[0]))
        {
            if(t_qCommandList.size() == 1) //No parameters
                parsedCommand = Command(t_qCommandList[0], QString(""), false);
            else
            {
                // check if number of parameters is right
                if(t_qCommandList.size()-1 == m_commandMap[t_qCommandList[0]].pValues().size())
                {
                    qDebug() << "Parameter parsing";
                    //Parse Parameters
                    for(qint32 i = 1; i < t_qCommandList.size(); ++i)
                    {
                        QVariant::Type t_type = m_commandMap[t_qCommandList[0]].pValues()[i - 1].type();

                        QVariant t_param(t_qCommandList[i]);

                        if(t_param.canConvert(t_type) && t_param.convert(t_type))
                            m_commandMap[t_qCommandList[0]].pValues()[i - 1] = t_param;
                        else
                            return false;
                    }
                }
                else
                    return false;
            }

            m_commandMap[t_qCommandList[0]].verify(parsedCommand);


            return true;
        }
    }

    return false;
}


//*************************************************************************************************************

QJsonObject CommandManager::toJsonObject() const
{
    return m_commandMap.toJsonObject();
}


//*************************************************************************************************************

QString CommandManager::toString() const
{
    return m_commandMap.toString();
}


//*************************************************************************************************************

void CommandManager::update(Subject* pSubject)
{
    CommandParser* pCommandParser = static_cast<CommandParser*>(pSubject);
    qDebug() << "in update method";
    qDebug() << "Received:" << pCommandParser->getRawCommand().command();

    qDebug() << "Number Paremeters:" << pCommandParser->getRawCommand().count();


    RawCommand rawCommand(pCommandParser->getRawCommand());
    QString sCommandName = rawCommand.command();

    if(this->hasCommand(sCommandName))
    {
        Command parsedCommand(sCommandName, QString(""), rawCommand.isJson());

        // check if number of parameters is right
        if(rawCommand.count() == m_commandMap[sCommandName].count())
        {
            qDebug() << "Parameter parsing";
            //Parse Parameters
            for(qint32 i = 1; i < rawCommand.count(); ++i)
            {
                QVariant::Type type = m_commandMap[sCommandName].pValues()[i].type();

                QVariant qVariantParam(rawCommand.pValues()[i]);

                if(qVariantParam.canConvert(type) && qVariantParam.convert(type))
                    m_commandMap[sCommandName].pValues()[i] = qVariantParam;
                else
                    return;
            }
        }
        else
            return;

        m_commandMap[sCommandName].verify(parsedCommand);
    }








//    if(p_sInput.size() <= 0)
//        return false;
//    //Check if JSON format;
//    bool isJson  = false;
//    if(QString::compare(p_sInput.at(0), QString("{")) == 0)
//        isJson = true;

//    if(isJson)
//    {
//        Command parsedCommand;
//        qDebug() << "JSON commands recognized";
//    }
//    else
//    {
//        Command parsedCommand;

//        QStringList t_qCommandList = sInput.split(" ");

//        if(this->hasCommand(t_qCommandList[0]))
//        {
//            if(t_qCommandList.size() == 1) //No parameters
//                parsedCommand = Command(t_qCommandList[0], QString(""), false);
//            else
//            {
//                // check if number of parameters is right
//                if(t_qCommandList.size()-1 == m_commandMap[t_qCommandList[0]].pValues().size())
//                {
//                    qDebug() << "Parameter parsing";
//                    //Parse Parameters
//                    for(qint32 i = 1; i < t_qCommandList.size(); ++i)
//                    {
//                        QVariant::Type t_type = m_commandMap[t_qCommandList[0]].pValues()[i - 1].type();

//                        QVariant t_param(t_qCommandList[i]);

//                        if(t_param.canConvert(t_type) && t_param.convert(t_type))
//                            m_commandMap[t_qCommandList[0]].pValues()[i - 1] = t_param;
//                        else
//                            return false;
//                    }
//                }
//                else
//                    return false;
//            }

//            m_commandMap[t_qCommandList[0]].verify(parsedCommand);


//            return true;
//        }
//    }

//    return false;











}


//*************************************************************************************************************

Command& CommandManager::operator[] (const QString &key)
{
    return m_commandMap[key];
}


//*************************************************************************************************************

const Command& CommandManager::operator[] (const QString &key) const
{
    return m_commandMap[key];
}

