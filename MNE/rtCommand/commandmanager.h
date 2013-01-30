#ifndef COMMUNICATIONMANAGER_H
#define COMMUNICATIONMANAGER_H

//*************************************************************************************************************
//=============================================================================================================
// Includes
//=============================================================================================================

#include "rtcommand_global.h"
#include "command.h"
#include "commandparser.h"

#include <generics/observerpattern.h>


//*************************************************************************************************************
//=============================================================================================================
// Qt Includes
//=============================================================================================================

#include <QObject>
#include <QJsonDocument>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE RTCOMMANDLIB
//=============================================================================================================

namespace RTCOMMANDLIB
{


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


//=============================================================================================================
/**
*
*/
class RTCOMMANDSHARED_EXPORT CommandManager : public QObject, public IObserver
{
    Q_OBJECT
public:
    explicit CommandManager(bool p_bIsActive = true, QObject *parent = 0);

    explicit CommandManager(const QByteArray &p_qByteArrayJsonDoc, bool p_bIsActive = true, QObject *parent = 0);

    explicit CommandManager(const QJsonDocument &p_jsonDoc, bool p_bIsActive = true, QObject *parent = 0);


    virtual ~CommandManager();

    //=========================================================================================================
    /**
    * Returns the lookup table of all available commands.
    *
    * @return the command lookup table
    */
    inline QMap<QString, Command>& commandMap()
    {
        return m_qMapCommands;
    }

    //=========================================================================================================
    /**
    * This creates a managed slot connection to a signal of a specified command executed.
    * Even its possible to connect a slot directly to a command signal - it'shighly recommended to use this managed connection.
    *
    * @param p_sCommand     Command to connect to.
    * @param receiver       Object which provides the slot.
    * @param slot           Member function to connect commands signal to.
    *
    * @return true if successfull, false otherwise
    */
    template <typename Func2>
    bool connectSlot(QString &p_sCommand, const typename QtPrivate::FunctionPointer<Func2>::Object *receiver, Func2 slot);

    //=========================================================================================================
    /**
    * This creates a managed signal connection to trigger a specified command.
    * Even its possible to connect a signal directly to a command slot - it'shighly recommended to use this managed connection.
    *
    * @param p_sCommand     Command to connect to.
    * @param sender         Object which provides the signal.
    * @param signal         Member function to connect commands signal to.
    *
    * @return true if successfull, false otherwise
    */
    template <typename Func1>
    bool connectSignal(QString &p_sCommand, const typename QtPrivate::FunctionPointer<Func1>::Object *sender, Func1 signal);

    //=========================================================================================================
    /**
    * Disconnects all managed signals and slots.
    */
    void disconnectAll();

    //=========================================================================================================
    /**
    * Checks if a command is managed;
    *
    * @param p_sCommand     COmmand to check.
    *
    * @return true if part of command manager, false otherwise
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
    * @param p_command  Command content. Attention CommandManager takes ownership of that command by reseting commad's parent;
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
    * Register reply Channel
    *
    * @param p_sCommand     Command to connect to.
    * @param receiver       Object which provides the slot.
    * @param slot           Member function to connect commands signal to.
    *
    * @return true if successfull, false otherwise
    */
    template <typename Func2>
    bool registerResponseChannel(const typename QtPrivate::FunctionPointer<Func2>::Object *receiver, Func2 slot);

    //=========================================================================================================
    /**
    * Sets the activation status of the CommandManager.
    *
    * @param [in] status the new activation status of the CommandManager.
    */
    inline void setStatus(bool status);

//    //=========================================================================================================
//    /**
//    * Creates an object of JSON Command Objects
//    *
//    * @return JSON Command Objects converted to a JSON Object.
//    */
//    QJsonObject toJsonObject() const;

//    //=========================================================================================================
//    /**
//    * Formats commands for e.g. command line output.
//    *
//    * @return Commands with parameters and descriptions.
//    */
//    QString toString() const;

    //=========================================================================================================
    /**
    * Updates the IObserver (CommandManager) when a new command was received.
    *
    * @param [in] p_pSubject  pointer to the subject (CommandParser) to which observer (CommandManager) is attached to.
    */
    virtual void update(Subject* p_pSubject);

    //=========================================================================================================
    /**
    * Subscript operator [] to access commands by command name
    *
    * @param key    the command key word.
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

    QJsonDocument m_jsonDocumentOrigin;

    bool m_bIsActive;

    QMap<QString, QMetaObject::Connection> m_qMapSlots;
    QMap<QString, QMetaObject::Connection> m_qMapSignals;


    QMetaObject::Connection m_conReplyChannel;


    QMap<QString, Command> m_qMapCommands;       /**< Holds a map as an internal lookuptable of available commands. */

signals:
    void commandMapChanged();//(QStringList)

//    void triggered(Command);
//    void received(Command);

    //=========================================================================================================
    /**
    * Is triggered when a reply is available. Commands are the emmiters of this signal -> access trough parent.
    *
    * @param p_sReply   the plain or JSON formatted reply
    */
    void response(QString p_sReply);
};

//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS & TEMPLATES
//=============================================================================================================

template <typename Func2>
bool CommandManager::connectSlot(QString &p_sCommand, const typename QtPrivate::FunctionPointer<Func2>::Object *receiver, Func2 slot)
{
    if(!this->hasCommand(p_sCommand))
        return false;

    QMetaObject::Connection qConnection = QObject::connect(&m_qMapCommands[p_sCommand], &Command::executed, receiver, slot);
    m_qMapSlots.insertMulti(p_sCommand, qConnection);

    return true;
}


//*************************************************************************************************************

template <typename Func1>
bool CommandManager::connectSignal(QString &p_sCommand, const typename QtPrivate::FunctionPointer<Func1>::Object *sender, Func1 signal)
{
    if(!this->hasCommand(p_sCommand))
        return false;

    QMetaObject::Connection t_qConnection = QObject::connect(sender, signal, &s_commandMap[p_sCommand], &Command::send);
    m_qMapSignals.insertMulti(p_sCommand, t_qConnection);

    return true;
}


//*************************************************************************************************************

template <typename Func2>
bool CommandManager::registerResponseChannel(const typename QtPrivate::FunctionPointer<Func2>::Object *receiver, Func2 slot)
{
    QObject::disconnect(m_conReplyChannel);
    m_conReplyChannel = QObject::connect(this, &CommandManager::response, receiver, slot);
    return true;
}


//*************************************************************************************************************

inline bool CommandManager::hasCommand(const QString &p_sCommand) const
{
    return m_qMapCommands.contains(p_sCommand);
}


//*************************************************************************************************************

inline bool CommandManager::isActive() const
{
    return m_bIsActive;
}


//*************************************************************************************************************

inline void CommandManager::setStatus(bool status)
{
    m_bIsActive = status;
}

} // NAMESPACE

#endif // COMMUNICATIONMANAGER_H
