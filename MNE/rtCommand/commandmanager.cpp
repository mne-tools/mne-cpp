
//*************************************************************************************************************
//=============================================================================================================
// Includes
//=============================================================================================================

#include "commandmanager.h"


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
    //ToDo Remove here commands which where inserted into the static command list
}


//*************************************************************************************************************

void CommandManager::init()
{
    QObject::connect(&s_commandMap, &CommandMap::commandMapChanged, this, &CommandManager::commandMapChanged);

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
    s_commandMap.insert(t_qCommandMap);
}


//*************************************************************************************************************

void CommandManager::insertCommand(const CommandMap &p_commandMap)
{
    s_commandMap.insert(p_commandMap);
}

//*************************************************************************************************************

void CommandManager::insertCommand(const QString &p_sKey, const QString &p_sDescription)
{
    s_commandMap.insert(p_sKey, p_sDescription);
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
            qDebug() << "Command found";
            return true;
        }
    }

    return false;
}


//*************************************************************************************************************

QJsonObject CommandManager::toJsonObject() const
{
    return s_commandMap.toJsonObject();
}


//*************************************************************************************************************

QString CommandManager::toString() const
{
    return s_commandMap.toString();
}


//*************************************************************************************************************

Command& CommandManager::operator[] (const QString &key)
{
    return s_commandMap[key];
}


//*************************************************************************************************************

const Command& CommandManager::operator[] (const QString &key) const
{
    return s_commandMap[key];
}

//*************************************************************************************************************
//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

CommandMap CommandManager::s_commandMap;
