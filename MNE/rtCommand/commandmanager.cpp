
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

CommandManager::CommandManager(QObject *parent)
: QObject(parent)
{
    init();
}


//*************************************************************************************************************

CommandManager::CommandManager(const QByteArray &p_jsonDoc, QObject *parent)
: QObject(parent)
{
    init();

    m_jsonDocumentOrigin = QJsonDocument::fromJson(p_jsonDoc);

    insert(m_jsonDocumentOrigin);
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

}


//*************************************************************************************************************
//ToDo connect all commands inserted in this class by default.
void CommandManager::insert(const QJsonDocument &p_jsonDocument)
{
    QJsonObject t_jsonObjectCommand;

    if(p_jsonDocument.isObject() && p_jsonDocument.object().value(QString("commands")) != QJsonValue::Undefined)
        t_jsonObjectCommand = p_jsonDocument.object().value(QString("commands")).toObject();
    else
        return;

    QJsonObject::Iterator it;
    for(it = t_jsonObjectCommand.begin(); it != t_jsonObjectCommand.end(); ++it)
    {
        if(!m_qMapCommands.contains(it.key()))
            m_qMapCommands.insert(it.key(), Command(it.key(), it.value().toObject()));
        else
            printf("Warning: CommandMap contains command %s already. Insertion skipped.\n", it.key().toLatin1().constData());
    }

    emit commandMapChanged();
}


//*************************************************************************************************************

void CommandManager::insert(const QString &p_sKey, const QString &p_sDescription)
{
    Command t_command(p_sKey, p_sDescription);
    insert(p_sKey, t_command);
}


//*************************************************************************************************************

void CommandManager::insert(const QString &p_sKey, const Command &p_Command)
{
    m_qMapCommands.insert(p_sKey, p_Command);
    emit commandMapChanged();
}


////*************************************************************************************************************

//bool CommandManager::parse(const QString &p_sInput)
//{
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

//        QStringList t_qCommandList = p_sInput.split(" ");

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
//}


//*************************************************************************************************************

QJsonObject CommandManager::toJsonObject() const
{
    QJsonObject p_jsonCommandsObject;

    QMap<QString, Command>::ConstIterator it;
    for(it = m_qMapCommands.begin(); it != m_qMapCommands.end(); ++it)
        p_jsonCommandsObject.insert(it.key(),QJsonValue(it.value().toJsonObject()));

    return p_jsonCommandsObject;
}


//*************************************************************************************************************

QString CommandManager::toString() const
{
    QString p_sOutput("");

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

        p_sOutput.append(t_sCommand);
    }

    return p_sOutput;
}


//*************************************************************************************************************

void CommandManager::update(Subject* p_pSubject)
{
    CommandParser* t_pCommandParser = static_cast<CommandParser*>(p_pSubject);

    RawCommand t_rawCommand(t_pCommandParser->getRawCommand());
    QString t_sCommandName = t_rawCommand.command();

    if(!this->hasCommand(t_sCommandName))//ToDo is Active
        return;

    // check if number of parameters is right
    if(t_rawCommand.count() == m_qMapCommands[t_sCommandName].count())
    {
        m_qMapCommands[t_sCommandName].isJson() = t_rawCommand.isJson();

        //Parse Parameters
        for(quint32 i = 1; i < t_rawCommand.count(); ++i)
        {
            QVariant::Type t_type = m_qMapCommands[t_sCommandName].pValues()[i].type();

            QVariant t_qVariantParam(t_rawCommand.pValues()[i]);

            if(t_qVariantParam.canConvert(t_type) && t_qVariantParam.convert(t_type))
                m_qMapCommands[t_sCommandName].pValues()[i] = t_qVariantParam;
            else
                return;
        }

        m_qMapCommands[t_sCommandName].execute();
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
    return m_qMapCommands[key];
}


//*************************************************************************************************************

const Command& CommandManager::operator[] (const QString &key) const
{
    return m_qMapCommands[key];
}

