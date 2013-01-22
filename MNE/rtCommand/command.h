

#ifndef COMMAND_H
#define COMMAND_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtcommand_global.h"


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QObject>
#include <QString>
#include <QVariant>
#include <QJsonObject>
#include <QSharedPointer>
#include <QMap>
#include <QDebug>
#include <QPair>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE RTCOMMANDLIB
//=============================================================================================================

namespace RTCOMMANDLIB
{
/**
 * Interface for a command request. This is usually sent from client to server.
 */
class RTCOMMANDSHARED_EXPORT Command: public QObject
{
Q_OBJECT
public:
    typedef QSharedPointer<Command> SPtr;
    typedef QSharedPointer<const Command> ConstSPtr;


    Command();


    Command(const Command &p_Command);


    virtual ~Command();


    //=========================================================================================================
    /**
    * Parses a command.
    *
    * @param[in] p_sCommandName     Command name
    * @param[in] p_qCommandContent  Content
    *
    * @return Parsed command
    */
    static Command fromQJsonObject(QString &p_sCommandName, QJsonObject &p_qCommandContent);


//    //=========================================================================================================
//    /**
//     * Gets the short command for this request. Can be used to identify the concrete implementation.
//     *
//     * @return Short command representation.
//     */
//    virtual QString getCommand() const = 0;

//    //=========================================================================================================
//    /**
//     * Gets the help text or description of this command.
//     *
//     * @return  Help text.
//     */
//    virtual QString getHelpText() const = 0;

//    //=========================================================================================================
//    /**
//     * Creates a ready-to-send object for the client. This method hides the JSON serialization.
//     *
//     * @return Request data to send to the server.
//     */
//    virtual QByteArray getCommandToJSON() const = 0;

    // In your implementation you should add some getter and setter for arguments.

    Command& operator= (const Command &rh)
    {
        if (this != &rh) // protect against invalid self-assignment
        {
            m_sCommandName = rh.m_sCommandName;
            m_sDescription = rh.m_sDescription;
            m_vecParamNames = rh.m_vecParamNames;
            m_vecParamDescription = rh.m_vecParamDescription;
            m_vecParamValue = rh.m_vecParamValue;
        }
        // to support chained assignment operators (a=b=c), always return *this
        return *this;
    }

signals:
    void triggered(Command);
    void received(Command);

public:
    QString             m_sCommandName;
    QString             m_sDescription;
    QVector<QString>    m_vecParamNames;
    QVector<QString>    m_vecParamDescription;
    QVector<QVariant>   m_vecParamValue;
};

} // NAMESPACE

#endif // COMMAND_H
