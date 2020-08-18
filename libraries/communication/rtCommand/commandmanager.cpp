
//=============================================================================================================
// Includes
//=============================================================================================================

#include "commandmanager.h"
#include "rawcommand.h"

//=============================================================================================================
// Qt Includes
//=============================================================================================================

#include <QVector>
#include <QDebug>
#include <QJsonObject>
#include <QStringList>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace COMMUNICATIONLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

CommandManager::CommandManager(bool p_bIsActive, QObject *parent)
: QObject(parent)
, m_bIsActive(p_bIsActive)
{
    init();
}

//=============================================================================================================

CommandManager::CommandManager(const QByteArray &p_qByteArrayJsonDoc, bool p_bIsActive, QObject *parent)
: QObject(parent)
, m_bIsActive(p_bIsActive)
{
    init();

    m_jsonDocumentOrigin = QJsonDocument::fromJson(p_qByteArrayJsonDoc);

    insert(m_jsonDocumentOrigin);
}

//=============================================================================================================

CommandManager::CommandManager(const QJsonDocument &p_jsonDoc, bool p_bIsActive, QObject *parent)
: QObject(parent)
, m_bIsActive(p_bIsActive)
, m_jsonDocumentOrigin(p_jsonDoc)
{
    init();

    insert(m_jsonDocumentOrigin);
}

//=============================================================================================================

CommandManager::~CommandManager()
{
    //Disconnect all connections which are created with the help of this manager.
//    this->disconnectAll();

    //Remove commands which where inserted into the static command list
}

//=============================================================================================================

void CommandManager::clear()
{
    m_qMapCommands.clear();
}

//=============================================================================================================

void CommandManager::init()
{
}

//=============================================================================================================
//ToDo connect all commands inserted in this class by default.
void CommandManager::insert(const QJsonDocument &p_jsonDocument)
{
    QJsonObject t_jsonObjectCommand;

    //Switch to command object
    if(p_jsonDocument.isObject() && p_jsonDocument.object().value(QString("commands")) != QJsonValue::Undefined)
        t_jsonObjectCommand = p_jsonDocument.object().value(QString("commands")).toObject();
    else
        return;

    QJsonObject::Iterator it;
    for(it = t_jsonObjectCommand.begin(); it != t_jsonObjectCommand.end(); ++it)
    {
        if(!m_qMapCommands.contains(it.key()))
            m_qMapCommands.insert(it.key(), Command(it.key(), it.value().toObject(), true, this));
        else
            qWarning("Warning: CommandMap contains command %s already. Insertion skipped.\n", it.key().toUtf8().constData());
    }

    emit commandMapChanged();
}

//=============================================================================================================

void CommandManager::insert(const QString &p_sKey, const QString &p_sDescription)
{
    Command t_command(p_sKey, p_sDescription, false, this);
    insert(p_sKey, t_command);
}

//=============================================================================================================

void CommandManager::insert(const QString &p_sKey, const Command &p_command)
{
    Command t_command(p_command);
    t_command.setParent(this);
    m_qMapCommands.insert(p_sKey, t_command);
    emit commandMapChanged();
}

//=============================================================================================================

void CommandManager::update(UTILSLIB::Subject* p_pSubject)
{
    // If Manager is not active do not parse commands
    if(!m_bIsActive)
        return;

    CommandParser* t_pCommandParser = static_cast<CommandParser*>(p_pSubject);

    RawCommand t_rawCommand(t_pCommandParser->getRawCommand());
    QString t_sCommandName = t_rawCommand.command();

    if(!this->hasCommand(t_sCommandName))
        return;

    // check if number of parameters is right
    if(t_rawCommand.count() >= m_qMapCommands[t_sCommandName].count())
    {
        m_qMapCommands[t_sCommandName].isJson() = t_rawCommand.isJson();

        //Parse Parameters
        for(quint32 i = 0; i < m_qMapCommands[t_sCommandName].count(); ++i)
        {
            QVariant::Type t_type = m_qMapCommands[t_sCommandName][i].type();

            QVariant t_qVariantParam(t_rawCommand.pValues()[i]);

            if(t_qVariantParam.canConvert(t_type) && t_qVariantParam.convert(t_type))
                m_qMapCommands[t_sCommandName][i] = t_qVariantParam;
            else
                return;
        }

        m_qMapCommands[t_sCommandName].execute();
    }
}

//=============================================================================================================

Command& CommandManager::operator[] (const QString &key)
{
    return m_qMapCommands[key];
}

//=============================================================================================================

const Command CommandManager::operator[] (const QString &key) const
{
    return m_qMapCommands[key];
}
