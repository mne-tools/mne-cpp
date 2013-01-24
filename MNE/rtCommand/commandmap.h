#ifndef COMMANDMAP_H
#define COMMANDMAP_H

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
#include <QJsonObject>


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
class RTCOMMANDSHARED_EXPORT CommandMap : public QObject
{
    Q_OBJECT
public:
    CommandMap(QObject *parent = 0);

    //=========================================================================================================
    /**
    * Returns true if the map contains a command item with key p_sCommand; otherwise returns false.
    *
    *@param p_sKey  Command key word.
    */
    bool contains(const QString &p_sKey) const;

    //=========================================================================================================
    /**
    * Insert a command without parameters
    * Attention existing items are overwritten.
    *
    * @param p_sKey             Command key word.
    * @param p_sDescription     Command description.
    */
    void insert(const QString &p_sKey, const QString &p_sDescription);

    //=========================================================================================================
    /**
    * Inserts a new command and emmits dataChanged signal.
    *
    * @param p_sKey     Command key word.
    * @param p_Command  Command content.
    */
    void insert(const QString &p_sKey, const Command &p_Command);

    //=========================================================================================================
    /**
    * Inserts a map of new commands and emmits dataChanged signal.
    *
    * @param p_qCommandMap    the command key word.
    */
    void insert(const CommandMap &p_qCommandMap);

    //=========================================================================================================
    /**
    * Returns number of available commands.
    *
    * @return number of available commands.
    */
    inline qint32 size()
    {
        return m_qMapCommands.size();
    }

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

signals:
    void commandMapChanged();

private:
    QMap<QString, Command> m_qMapCommands;       /**< Holds static map of all available commands. Attention this is allocated statically! Lifetime extends across entire run of the programm. Accessible from all over the programm. */

};

} // NAMESPACE

#endif // COMMANDMAP_H
