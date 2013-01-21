

#ifndef ICOMMAND_H
#define ICOMMAND_H


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QObject>
#include <QSharedPointer>
#include <QVariant>



namespace RTCOMMUNICATIONLIB
{
/**
 * Interface for a command request. This is usually sent from client to server.
 */
class ICommand: public QObject
{
Q_OBJECT
public:
    typedef QSharedPointer<ICommand> SPtr;
    typedef QSharedPointer<const ICommand> ConstSPtr;


    virtual ~ICommand()
    {
    }

    //=========================================================================================================
    /**
     * Gets the short command for this request. Can be used to identify the concrete implementation.
     *
     * @return Short command representation.
     */
    virtual QString getCommand() const = 0;

    //=========================================================================================================
    /**
     * Gets the help text or description of this command.
     *
     * @return  Help text.
     */
    virtual QString getHelpText() const = 0;

    //=========================================================================================================
    /**
     * Creates a ready-to-send object for the client. This method hides the JSON serialization.
     *
     * @return Request data to send to the server.
     */
    virtual QByteArray getCommandToJSON() const = 0;

    // In your implementation you should add some getter and setter for arguments.

signals:


private:
    QString             m_sCommandName;
    QList<QVariant>     m_listParameters;


};

} // NAMESPACE

#endif // ICOMMAND_H
