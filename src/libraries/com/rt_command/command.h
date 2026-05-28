//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file command.h
 * @since March 2026
 * @brief Typed, parameter-bearing command object that round-trips between mne-cpp clients and @c mne_rt_server.
 *
 * @ref COMLIB::Command is the unit of conversation on the
 * @c mne_rt_server control channel after parsing has assigned types
 * (compare @ref COMLIB::RawCommand, which carries only string tokens).
 * Each instance holds a short command keyword (e.g. @c bufsize,
 * @c selch, @c start), a human-readable description, a list of named
 * parameters with both their @c QVariant values and their per-parameter
 * descriptions, and a flag selecting JSON or CLI serialisation when the
 * object is pushed out through @ref RtCmdClient.
 *
 * Two construction paths matter. The data path: an incoming JSON object
 * is parsed into a fully-populated @c Command by the @c QJsonObject
 * constructor, so the server’s self-described command list (returned by
 * @c requestCommands()) can be replayed locally without hard-coding the
 * schema. The authoring path: client code constructs an empty
 * @c Command with name, description and parameter declarations, fills
 * @c pValues() per call site, then forwards it to
 * @ref RtCmdClient::sendCommandJSON. @c toStringReadySend() and
 * @c toJsonObject() / @c toStringList() format the same object for the
 * different transports.
 *
 * @c Command implements @ref UTILSLIB::ICommand from the generic command
 * pattern so it can be dispatched through @ref CommandManager without
 * the manager needing to know whether the receiver is local or remote;
 * the @c triggered / @c received signals are how the manager routes
 * execution back to subscribers.
 */

#ifndef COMMAND_H
#define COMMAND_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../com_global.h"

#include <utils/generics/commandpattern.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QObject>
#include <QList>
#include <QString>
#include <QVariant>
#include <QJsonObject>
#include <QSharedPointer>
#include <QDebug>
#include <QPair>
#include <QStringList>

//=============================================================================================================
// DEFINE NAMESPACE COMLIB
//=============================================================================================================

namespace COMLIB
{

static QVariant defaultVariant;

//=============================================================================================================
/**
 * @brief Named, parameterised command exchanged with @c mne_rt_server; supports both JSON and CLI serialisation.
 *
 * Holds the command keyword, description, parallel lists of parameter
 * names / values / descriptions, and a JSON-vs-CLI flag. Constructible
 * either from a parsed @c QJsonObject (when echoing the server’s
 * advertised command list) or from explicit string/value lists (when
 * authoring a request). Implements @ref UTILSLIB::ICommand so it can be
 * dispatched through @ref CommandManager and surfaces parameter access
 * by both name (@c operator[](QString)) and index
 * (@c operator[](qint32)).
 */
class COMSHARED_EXPORT Command: public QObject, public UTILSLIB::ICommand
{
Q_OBJECT

public:
    typedef QSharedPointer<Command> SPtr;
    typedef QSharedPointer<const Command> ConstSPtr;

    //=========================================================================================================
    /**
     * Constructs a Command
     *
     * @param[in] p_bIsJson      If is received/should be send as JSON (optional, default true).
     * @param[in] parent         Parent QObject (optional).
     */
    explicit Command(bool p_bIsJson = true, QObject *parent = 0);

    //=========================================================================================================
    /**
     * Constructor which parses a command stored in a json object
     *
     * @param[in] p_sCommand         Command.
     * @param[in] p_qCommandContent  Content encapsulated in a JsonObject.
     * @param[in] p_bIsJson          If is received/should be send as JSON (optional, default true).
     * @param[in] parent             Parent QObject (optional).
     */
    explicit Command(const QString &p_sCommand, const QJsonObject &p_qCommandContent, bool p_bIsJson = true, QObject *parent = 0);

    //=========================================================================================================
    /**
     * Constructs a command without parameters
     *
     * @param[in] p_sCommand         Command.
     * @param[in] p_sDescription     Command description.
     * @param[in] p_bIsJson          If is received/should be send as JSON (optional, default true).
     * @param[in] parent             Parent QObject (optional).
     */
    explicit Command(const QString &p_sCommand, const QString &p_sDescription, bool p_bIsJson = true, QObject *parent = 0);

    //=========================================================================================================
    /**
     * Constructor which assembles a command from single parts
     *
     * @param[in] p_sCommand         Command.
     * @param[in] p_sDescription     Command description.
     * @param[in] p_qListParamNames  Parameter names.
     * @param[in] p_qListParamValues Parameter values/types.
     * @param[in] p_bIsJson          If is received/should be send as JSON (optional, default true).
     * @param[in] parent             Parent QObject (optional).
     */
    explicit Command(const QString &p_sCommand, const QString &p_sDescription,
                     const QStringList &p_qListParamNames, const QList<QVariant> &p_qListParamValues, bool p_bIsJson = true, QObject *parent = 0);

    //=========================================================================================================
    /**
     * Constructor which assembles a command from single parts
     *
     * @param[in] p_sCommand                 Command.
     * @param[in] p_sDescription             Command description.
     * @param[in] p_qListParamNames          Parameter names.
     * @param[in] p_qListParamValues         Parameter values/types.
     * @param[in] p_vecParameterDescriptions Parameter descriptions;.
     */
    explicit Command(const QString &p_sCommand, const QString &p_sDescription,
                     const QStringList &p_qListParamNames, const QList<QVariant> &p_qListParamValues, const QStringList &p_vecParameterDescriptions, bool p_bIsJson = true, QObject *parent = 0);

    //=========================================================================================================
    /**
     * Copy constructor.
     *
     * @param[in] p_Command      Command to be copied.
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
    inline QString command() const;

    //=========================================================================================================
    /**
     * Returns the number of parameters.
     *
     * @return number of parameters.
     */
    inline quint32 count() const;

    //=========================================================================================================
    /**
     * Gets the help text or description of this command.
     *
     * @return  Help text.
     */
    inline QString description() const;

    //=========================================================================================================
    /**
     * Inherited by ICommand
     *
     * @return  emits received.
     */
    virtual void execute();

    //=========================================================================================================
    /**
     * If received command was Json fomratted or triggered command should be Json formatted.
     *
     * @return Json formatted.
     */
    inline bool& isJson();

    //=========================================================================================================
    /**
     * Get parameter descriptions
     *
     * @return parameter descriptions.
     */
    inline QList<QString> pDescriptions() const;

    //=========================================================================================================
    /**
     * Get parameter names
     *
     * @return parameter names.
     */
    inline QList<QString> pNames() const;

    //=========================================================================================================
    /**
     * Returns parameter values
     *
     * @return parameter values.
     */
    inline QList<QVariant>& pValues();

    //=========================================================================================================
    /**
     * Inherited command reply channel.
     *
     * @param[in] p_sReply   command reply.
     */
    void reply(const QString &p_sReply);

    //=========================================================================================================
    /**
     * Sender slot which emmits triggered signal
     */
    void send();

    //=========================================================================================================
    /**
     * Creates a JSON Command Object
     *
     * @return Command converted to a JSON Object.
     */
    QJsonObject toJsonObject() const;

    //=========================================================================================================
    /**
     * Creates a StringList with three items. First item is the command, second the parameter list and thrid the description.
     *
     * @return Command as a StringList.
     */
    QStringList toStringList() const;

    //=========================================================================================================
    /**
     * Creates a string JSON formatted ready send command.
     *
     * @return Command as a JSON formatted string which contains parameter values too.
     */
    QString toStringReadySend() const;

    //=========================================================================================================
    /**
     * Assignment Operator
     *
     * @param[in] rhs     Command which should be assigned.
     */
    Command& operator= (const Command &rhs);

    //=========================================================================================================
    /**
     * Subscript operator [] to access parameter values by name
     *
     * @param[in] key    the parameter name.
     *
     * @return Parameter value related to the parameter name.
     */
    QVariant& operator[] (const QString &key);

    //=========================================================================================================
    /**
     * Subscript operator [] to access parameter values by index
     *
     * @param[in] idx    the parameter index.
     *
     * @return Parameter value related to the parameter index.
     */
    QVariant& operator[] (qint32 idx);

    //=========================================================================================================
    /**
     * Subscript operator []
     *
     * @param[in] key    the parameter name.
     *
     * @return Parameter value related to the parameter name.
     */
    const QVariant operator[] (const QString &key) const;

signals:
    //=========================================================================================================
    /**
     * Signal which is emitted when command patterns execute method is processed.
     *
     * @param[in] p_command  the executed command.
     */
    void executed(Command p_command);

public:
    QString             m_sCommand;                 /**< The command keyword. */
    QString             m_sDescription;             /**< Human-readable description of the command. */
    QStringList         m_qListParamNames;          /**< Parameter names (positional order). */
    QList<QVariant>     m_qListParamValues;         /**< Current parameter values (same order as names). */
    QStringList         m_qListParamDescriptions;   /**< Per-parameter description strings. */
    bool                m_bIsJson;                  /**< True when the command was parsed from JSON. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline QString Command::command() const
{
    return m_sCommand;
}

//=============================================================================================================

inline quint32 Command::count() const
{
    return m_qListParamValues.size();
}

//=============================================================================================================

inline QString Command::description() const
{
    return m_sDescription;
}

//=============================================================================================================

inline bool& Command::isJson()
{
    return m_bIsJson;
}

//=============================================================================================================

inline QList<QString> Command::pDescriptions() const
{
    return m_qListParamDescriptions;
}

//=============================================================================================================

inline QList<QString> Command::pNames() const
{
    return m_qListParamNames;
}

//=============================================================================================================

inline QList<QVariant>& Command::pValues()
{
    return m_qListParamValues;
}
} // NAMESPACE

#endif // COMMAND_H
