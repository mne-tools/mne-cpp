#ifndef COMMUNICATIONMANAGER_H
#define COMMUNICATIONMANAGER_H

//=============================================================================================================
// Includes
//=============================================================================================================

#include "../communication_global.h"
#include "command.h"
#include "commandparser.h"

#include <utils/generics/observerpattern.h>

//=============================================================================================================
// Qt Includes
//=============================================================================================================

#include <QObject>
#include <QJsonDocument>

//=============================================================================================================
// DEFINE NAMESPACE COMMUNICATIONLIB
//=============================================================================================================

namespace COMMUNICATIONLIB
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 *
 */
class COMMUNICATIONSHARED_EXPORT CommandManager : public QObject, public UTILSLIB::IObserver
{
    Q_OBJECT

public:
    explicit CommandManager(bool p_bIsActive = true, QObject *parent = 0);

    explicit CommandManager(const QByteArray &p_qByteArrayJsonDoc, bool p_bIsActive = true, QObject *parent = 0);

    explicit CommandManager(const QJsonDocument &p_jsonDoc, bool p_bIsActive = true, QObject *parent = 0);

    virtual ~CommandManager();

    //=========================================================================================================
    /**
     * Clears the command manager
     */
    void clear();

    //=========================================================================================================
    /**
     * Returns the lookup table of all available commands.
     *
     * @return the command lookup table.
     */
    inline QMap<QString, Command>& commandMap();

    //=========================================================================================================
    /**
     * Checks if a command is managed;
     *
     * @param p_sCommand     COmmand to check.
     *
     * @return true if part of command manager, false otherwise.
     */
    inline bool hasCommand(const QString &p_sCommand) const;

    //=========================================================================================================
    /**
     * Inserts commands encoded in a json document.
     * Attention existing items are overwritten.
     *
     * @param p_jsonDocument    JSON document containing commands.
     */
    void insert(const QJsonDocument &p_jsonDocument);

    //=========================================================================================================
    /**
     * Inserts a single command.
     * Attention existing items are overwritten.
     *
     * @param p_jsonDocument    JSON document containing commands.
     */
    void insert(const QString &p_sKey, const QString &p_sDescription);

    //=========================================================================================================
    /**
     * Inserts a new command and emmits dataChanged signal.
     *
     * @param p_sKey     Command key word.
     * @param p_command  Command content. Attention CommandManager takes ownership of that command by reseting commad's parent;.
     */
    void insert(const QString &p_sKey, const Command &p_command);

    //=========================================================================================================
    /**
     * Returns if CommandManager is active. If true, this manager parses incomming commands.
     *
     * @return if true, incomming commands are parsed.
     */
    inline bool isActive() const;

    //=========================================================================================================
    /**
     * Sets the activation status of the CommandManager.
     *
     * @param[in] status the new activation status of the CommandManager.
     */
    inline void setStatus(bool status);

    //=========================================================================================================
    /**
     * Updates the IObserver (CommandManager) when a new command was received.
     *
     * @param[in] p_pSubject  pointer to the subject (CommandParser) to which observer (CommandManager) is attached to.
     */
    virtual void update(UTILSLIB::Subject* p_pSubject);

    //=========================================================================================================
    /**
     * Subscript operator [] to access commands by command name
     *
     * @param[in] key    the command key word.
     *
     * @return Command object related to command key word.
     */
    Command& operator[] (const QString &key);

    //=========================================================================================================
    /**
     * Subscript operator [] to access commands by command name
     *
     * @param key    the command key word.
     *
     * @return Command object related to command key word.
     */
    const Command operator[] (const QString &key) const;

private:
    //=========================================================================================================
    /**
     * Initializes the command manager by connecting internal signal/slots
     */
    void init();

    bool m_bIsActive;

    QJsonDocument m_jsonDocumentOrigin;

    QMetaObject::Connection m_conReplyChannel;      /**< The reply channel of the command manager. */

    QMap<QString, Command> m_qMapCommands;          /**< Holds a map as an internal lookuptable of available commands. */

signals:
    void commandMapChanged();//(QStringList)

    //=========================================================================================================
    /**
     * Is emitted when a command is ready to send
     *
     * @param p_command  Command which should be send.
     */
    void triggered(Command p_command);

    //=========================================================================================================
    /**
     * Is triggered when a reply is available. Commands are the emmiters of this signal -> access trough parent.
     *
     * @param p_sReply   the plain or JSON formatted reply.
     *@param p_command   Command which send the response
     */
    void response(QString p_sReply, Command p_command);
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline QMap<QString, Command>& CommandManager::commandMap()
{
    return m_qMapCommands;
}

//=============================================================================================================

inline bool CommandManager::hasCommand(const QString &p_sCommand) const
{
    return m_qMapCommands.contains(p_sCommand);
}

//=============================================================================================================

inline bool CommandManager::isActive() const
{
    return m_bIsActive;
}

//=============================================================================================================

inline void CommandManager::setStatus(bool status)
{
    m_bIsActive = status;
}
} // NAMESPACE

#endif // COMMUNICATIONMANAGER_H
