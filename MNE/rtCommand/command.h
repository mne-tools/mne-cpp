

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
#include <QList>
#include <QString>
#include <QVariant>
#include <QJsonObject>
#include <QSharedPointer>
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

    //=========================================================================================================
    /**
    * Default constructor.
    *
    * @param[in] parent                 Parent QObject (optional)
    */
    Command(QObject *parent = 0);

    //=========================================================================================================
    /**
    * Constructor which parses a command stored in a json object
    *
    * @param[in] p_sCommand         Command
    * @param[in] p_qCommandContent  Content encapsulated in a JsonObject
    */
    explicit Command(QString &p_sCommand, QJsonObject &p_qCommandContent);

    //=========================================================================================================
    /**
    * Constructor which parses a command stored in a json object
    *
    * @param[in] p_sCommand                 Command
    * @param[in] p_sDescription             Command description
    * @param[in] p_mapParameters            Parameter names + values/types.
    * @param[in] p_vecParameterDescriptions Parameter descriptions;
    */
    explicit Command(const QString &p_sCommand, const QString &p_sDescription,
                     const QMap<QString, QVariant> &p_mapParameters, const QList<QString> &p_vecParameterDescriptions);

    //=========================================================================================================
    /**
    * Copy constructor.
    *
    * @param[in] p_Command      Command to be copied
    */
    Command(const Command &p_Command);

    //=========================================================================================================
    /**
    * Destroys the command
    */
    virtual ~Command();

    //=========================================================================================================
    /**
    * Short command for this request
    *
    * @return Short command representation.
    */
    inline QString command() const
    {
        return m_sCommand;
    }

    //=========================================================================================================
    /**
     * Gets the help text or description of this command.
     *
     * @return  Help text.
     */
    inline QString description() const
    {
        return m_sDescription;
    }

    //=========================================================================================================
    /**
    * Returns the number of parameters.
    *
    * @return number of parameters.
    */
    inline quint32 count() const
    {
        return m_mapParameters.size();
    }

    //=========================================================================================================
    /**
    * Get parameter descriptions
    *
    * @return parameter descriptions
    */
    inline QList<QString> pDescriptions() const
    {
        return m_vecParamDescriptions;
    }

    //=========================================================================================================
    /**
    * Get parameter names
    *
    * @return parameter names
    */
    inline QList<QString> pNames() const
    {
        return m_mapParameters.keys();
    }

    //=========================================================================================================
    /**
    * Returns parameter values
    *
    * @return parameter values
    */
    inline QList<QVariant> pValues() const
    {
        return m_mapParameters.values();
    }

    //=========================================================================================================
    /**
    * Creates a JSON Command Object
    *
    * @return Command converted to a JSON Object.
    */
    QJsonObject toJsonObject() const;

    //=========================================================================================================
    /**
    * Assignment Operator
    *
    * @param[in] rhs     Command which should be assigned.
    */
    Command& operator= (const Command &rhs);

    //=========================================================================================================
    /**
    * Subscript operator []
    *
    * @param key    the parameter name.
    *
    * @return Parameter value related to the parameter name.
    */
    QVariant& operator[] (const QString &key);

    //=========================================================================================================
    /**
    * Subscript operator []
    *
    * @param key    the parameter name.
    *
    * @return Parameter value related to the parameter name.
    */
    const QVariant operator[] (const QString &key) const;

signals:


public:
    QString             m_sCommand;
    QString             m_sDescription;
    QMap<QString, QVariant> m_mapParameters;
    QList<QString>    m_vecParamDescriptions;
};

} // NAMESPACE

#endif // COMMAND_H
