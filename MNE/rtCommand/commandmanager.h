#ifndef COMMUNICATIONMANAGER_H
#define COMMUNICATIONMANAGER_H

//*************************************************************************************************************
//=============================================================================================================
// Includes
//=============================================================================================================

#include "rtcommand_global.h"
#include "command.h"
#include "commandmap.h"
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
    explicit CommandManager(const QString test = QString("test"), QObject *parent = 0);

    explicit CommandManager(const QByteArray &p_jsonDoc, const QString test = QString("test"),  QObject *parent = 0);

    virtual ~CommandManager();

    //=========================================================================================================
    /**
    * This creates a managed slot connection to a received signal of a specified command.
    * Even its possible to connect a slot directly to a command signal - it'shighly recommended to use this managed connection.
    *
    * @param p_sCommand     Command to connect to.
    * @param receiver       Object which provides the slot.
    * @param slot           Member function to connect commands signal to.
    *
    * @return true if successfull, false otherwise
    */
    template <typename Func2>
    bool connectSlot(QString &p_sCommand, const typename QtPrivate::FunctionPointer<Func2>::Object *receiver, Func2 slot)
    {
        if(!this->hasCommand(p_sCommand))
            return false;

        QMetaObject::Connection qConnection = QObject::connect(&m_commandMap[p_sCommand], &Command::received, receiver, slot);
        m_qMapSlots.insertMulti(p_sCommand, qConnection);

        return true;
    }

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
    bool connectSignal(QString &p_sCommand, const typename QtPrivate::FunctionPointer<Func1>::Object *sender, Func1 signal)
    {
        if(!this->hasCommand(p_sCommand))
            return false;

        QMetaObject::Connection t_qConnection = QObject::connect(sender, signal, &s_commandMap[p_sCommand], &Command::send);
        m_qMapSignals.insertMulti(p_sCommand, t_qConnection);

        return true;
    }

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
    inline bool hasCommand(const QString &p_sCommand) const
    {
        return m_commandMap.contains(p_sCommand);
    }

    //=========================================================================================================
    /**
    * Parses a CLI command or JSON command (list). And emits a command received when its a valid command.
    *
    * @param p_sInput     Input to parse.
    */
    bool parse(const QString &p_sInput);

    //=========================================================================================================
    /**
    * Insert commands containing in a json document.
    * Attention existing items are overwritten.
    *
    * @param p_jsonDocument    JSON document containing commands.
    */
    void insertCommand(const QJsonDocument &p_jsonDocument);

    //=========================================================================================================
    /**
    * Insert commands of a json document.
    * Attention existing items are overwritten.
    *
    * @param p_commandMap   command map which should be inserted.
    */
    void insertCommand(const CommandMap &p_commandMap);

    //=========================================================================================================
    /**
    * Insert a single command.
    * Attention existing items are overwritten.
    *
    * @param p_jsonDocument    JSON document containing commands.
    */
    void insertCommand(const QString &p_sKey, const QString &p_sDescription);

    //=========================================================================================================
    /**
    * Creates an object of JSON Command Objects
    *
    * @return JSON Command Objects converted to a JSON Object.
    */
    QJsonObject toJsonObject() const;

    //=========================================================================================================
    /**
    * Formats commands for e.g. command line output.
    *
    * @return Commands with parameters and descriptions.
    */
    QString toString() const;

    //=========================================================================================================
    /**
    * Updates the IObserver (CommandManager) when a new command was received.
    *
    * @param [in] pSubject  pointer to the subject (CommandParser) to which observer (CommandManager) is attached to.
    */
    virtual void update(Subject* pSubject);

    //=========================================================================================================
    /**
    * Subscript operator []
    *
    * @param key    the command key word.
    *
    * @return Command object related to command key word.
    */
    Command& operator[] (const QString &key);

    //=========================================================================================================
    /**
    * Subscript operator []
    *
    * @param key    the command key word.
    *
    * @return Command object related to command key word.
    */
    const Command& operator[] (const QString &key) const;

private:
    void testSlot();

    QString m_sTest;

    void init();

    QJsonDocument m_jsonDocumentOrigin;

    QMap<QString, QMetaObject::Connection> m_qMapSlots;
    QMap<QString, QMetaObject::Connection> m_qMapSignals;

    CommandMap m_commandMap;     /**< Holds map as an internal lookuptable of available commands. */

signals:
    void commandMapChanged();//(QStringList)

//    void triggered(Command);
//    void received(Command);

    void cliReply(QString);
    void jsonReply(QString);
};

} // NAMESPACE

#endif // COMMUNICATIONMANAGER_H
