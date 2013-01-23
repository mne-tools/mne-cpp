
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


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace RTCOMMANDLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

CommandManager::CommandManager(QObject *parent)
: QObject(parent)
{

}


//*************************************************************************************************************

CommandManager::CommandManager(QByteArray &p_jsonDoc, QObject *parent)
: QObject(parent)
{
    m_jsonDocumentOrigin = QJsonDocument::fromJson(p_jsonDoc);

    insertJsonCommands(m_jsonDocumentOrigin);
}


//*************************************************************************************************************

CommandManager::~CommandManager()
{
    //ToDo Remove here commands which where inserted into the static command list
}


//*************************************************************************************************************
//ToDo connect all commands inserted in this class by default.
void CommandManager::insertJsonCommands(QJsonDocument &p_jsonDocument)
{
    QJsonObject t_jsonObjectCommand;

    if(p_jsonDocument.isObject() && p_jsonDocument.object().value(QString("commands")) != QJsonValue::Undefined)
        t_jsonObjectCommand = p_jsonDocument.object().value(QString("commands")).toObject();
    else
        return;

    QJsonObject::Iterator it;
    for(it = t_jsonObjectCommand.begin(); it != t_jsonObjectCommand.end(); ++it)
        s_mapCommands.insert(it.key(), Command(it.key(), it.value().toObject()));//Attention overwrites existing items
}


//*************************************************************************************************************

void parse(QString &p_sInput)
{
    if(p_sInput.size() <= 0)
        return;
    //Check if JSON format;
    bool isJson  = false;
    if(QString::compare(p_sInput.at(0), QString("{")) == 0)
        isJson = true;

    Command parsedCommand;
}


//*************************************************************************************************************

QJsonObject CommandManager::toJsonObject() const
{
    QJsonObject p_jsonCommandsObject;

    QMap<QString, Command>::Iterator it;
    for(it = s_mapCommands.begin(); it != s_mapCommands.end(); ++it)
        p_jsonCommandsObject.insert(it.key(),QJsonValue(it.value().toJsonObject()));

    return p_jsonCommandsObject;
}


//*************************************************************************************************************

Command& CommandManager::operator[] (const QString &key)
{
    return s_mapCommands[key];
}


//*************************************************************************************************************

const Command& CommandManager::operator[] (const QString &key) const
{
    return s_mapCommands[key];
}



//*************************************************************************************************************
//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

QMap<QString, Command> CommandManager::s_mapCommands;
