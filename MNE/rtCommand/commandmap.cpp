
//*************************************************************************************************************
//=============================================================================================================
// Includes
//=============================================================================================================

#include "commandmap.h"
#include <math.h>


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

QString CommandMap::toString() const
{
    QString t_sOutput("");

    QMap<QString, Command>::ConstIterator it;
    for(it = m_qMapCommands.begin(); it != m_qMapCommands.end(); ++it)
    {
        QStringList t_sCommandList = it.value().toStringList();
        QString t_sCommand;
        t_sCommand.append(QString("\t%1").arg(t_sCommandList[0]));

        for(qint32 i = 0; i < 2 - (int)floor((double)t_sCommandList[0].size()/8.0); ++i)
            t_sCommand.append(QString("\t"));
        t_sCommand.append(t_sCommandList[1]);

        for(qint32 i = 0; i < 3 - (int)floor((double)t_sCommandList[1].size()/8.0); ++i)
            t_sCommand.append(QString("\t"));
        t_sCommand.append(QString("%1\n\r").arg(t_sCommandList[2]));

        t_sOutput.append(t_sCommand);
    }

    return t_sOutput;
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
