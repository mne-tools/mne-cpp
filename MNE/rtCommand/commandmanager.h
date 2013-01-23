#ifndef COMMUNICATIONMANAGER_H
#define COMMUNICATIONMANAGER_H

//*************************************************************************************************************
//=============================================================================================================
// Includes
//=============================================================================================================

#include "rtcommand_global.h"
#include "command.h"


//*************************************************************************************************************
//=============================================================================================================
// Qt Includes
//=============================================================================================================

#include <QObject>
#include <QMap>
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
class RTCOMMANDSHARED_EXPORT CommandManager : public QObject
{
Q_OBJECT
public:

    CommandManager(QObject *parent = 0);

    CommandManager(QByteArray &p_jsonDoc, QObject *parent = 0);

    virtual ~CommandManager();

    //=========================================================================================================
    /**
    * Checks if a command is managed;
    *
    * @param p_sCommand     COmmand to check.
    *
    * @return true if part of command manager, false otherwise
    */
    inline bool hasCommand(QString &p_sCommand) const
    {
        return s_mapCommands.contains(p_sCommand);
    }

    //=========================================================================================================
    /**
    * Parses a CLI command or JSON command (list). And emits a command received when its a valid command.
    *
    * @param p_sInput     Input to parse.
    */
    void parse(QString &p_sInput);

    //=========================================================================================================
    /**
    * Creates an object of JSON Command Objects
    *
    * @return JSON Command Objects converted to a JSON Object.
    */
    QJsonObject toJsonObject() const;

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

    //=========================================================================================================
    /**
    * Attention overwrites existing items
    */
    static void insertJsonCommands(QJsonDocument &p_jsonDocument);

private:
    QJsonDocument m_jsonDocumentOrigin;

    static QMap<QString, Command> s_mapCommands;       /**< Holds static map of all available commands. Attention this is allocated statically! Lifetime extends across entire run of the programm. Accessible from all over the programm. */

signals:
    void commandsInserted();//(QStringList)

//    void triggered(Command);
//    void received(Command);

    void cliReply(QString);
    void jsonReply(QString);
};

} // NAMESPACE


#endif // COMMUNICATIONMANAGER_H
