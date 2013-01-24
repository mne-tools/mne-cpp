
//*************************************************************************************************************
//=============================================================================================================
// Includes
//=============================================================================================================

#include "commandmap.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace RTCOMMANDLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

CommandMap::CommandMap(QObject *parent)
: QObject(parent)
{

}


//*************************************************************************************************************

bool CommandMap::contains(const QString &p_sKey) const
{
    return m_qMapCommands.contains(p_sKey);
}


//*************************************************************************************************************

void CommandMap::insert(const QString &p_sKey, const QString &p_sDescription)
{
    Command t_command(p_sKey, p_sDescription);
    insert(p_sKey, t_command);
}


//*************************************************************************************************************

void CommandMap::insert(const QString &p_sKey, const Command &p_Command)
{
    m_qMapCommands.insert(p_sKey, p_Command);
    emit commandMapChanged();
}


//*************************************************************************************************************

void CommandMap::insert(const CommandMap &p_qCommandMap)
{
    QMap<QString,Command>::ConstIterator it;
    for(it = p_qCommandMap.m_qMapCommands.begin(); it != p_qCommandMap.m_qMapCommands.end(); ++it)
    {
        if(!m_qMapCommands.contains(it.key()))
            m_qMapCommands.insert(it.key(), it.value());
        else
            printf("Warning: CommandMap contains command %s already. Insertion skipped.\n", it.key().toLatin1().constData());
    }
    emit commandMapChanged();
}


//*************************************************************************************************************

QJsonObject CommandMap::toJsonObject() const
{
    QJsonObject p_jsonCommandsObject;

    QMap<QString, Command>::ConstIterator it;
    for(it = m_qMapCommands.begin(); it != m_qMapCommands.end(); ++it)
        p_jsonCommandsObject.insert(it.key(),QJsonValue(it.value().toJsonObject()));

    return p_jsonCommandsObject;
}

//*************************************************************************************************************

Command& CommandMap::operator[] (const QString &key)
{
    return m_qMapCommands[key];
}


//*************************************************************************************************************

const Command& CommandMap::operator[] (const QString &key) const
{
    return m_qMapCommands[key];
}
