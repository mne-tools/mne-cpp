//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file raw_command.h
 * @since 2026
 * @date  March 2026
 * @brief Untyped intermediate produced by @ref COMLIB::CommandParser before type information is bound to parameters.
 *
 * @ref COMLIB::RawCommand is the first object the parser builds when a
 * command arrives on @ref RtCmdClient. At that point the wire payload
 * has been tokenised into a command keyword and a list of string
 * arguments, but the parser does not yet know which @ref Command
 * descriptor (if any) those arguments belong to — the parameter types,
 * names and descriptions live in whichever @ref CommandManager ends up
 * accepting the request. Keeping that pre-resolution state in its own
 * class avoids forcing every observer to deal with half-populated
 * @c Command objects.
 *
 * The class also implements @ref UTILSLIB::ICommand so it can be
 * notified through the same dispatch path as a fully-typed @c Command;
 * the @c executed(QList<QString>) signal hands the raw parameter list
 * to whichever @ref CommandManager recognises the keyword, which is
 * responsible for converting the strings into the appropriate
 * @c QVariant values and re-emitting them as a typed @c Command.
 *
 * @c isJson() records whether the original payload was JSON or CLI, so
 * the reply produced by the eventual handler can be sent back in the
 * same dialect the caller used.
 */

#ifndef RAWCOMMAND_H
#define RAWCOMMAND_H

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
#include <QVariant>

//=============================================================================================================
// DEFINE NAMESPACE COMLIB
//=============================================================================================================

namespace COMLIB
{

//=============================================================================================================
/**
 * @brief Tokenised but unresolved command: a keyword plus a list of raw string arguments awaiting type binding.
 *
 * Emitted by @ref CommandParser before any @ref CommandManager has
 * claimed the request. Observers turn the @c QList<QString> arguments
 * into typed @ref Command parameters once the matching schema is
 * located. Retains the JSON-vs-CLI dialect flag so replies travel back
 * in the same encoding the caller used.
 */
class COMSHARED_EXPORT RawCommand : public QObject, public UTILSLIB::ICommand
{
Q_OBJECT

public:

    //=========================================================================================================
    /**
     * Default constructor.
     *
     * @param[in] parent             Parent QObject (optional).
     */
    explicit RawCommand(QObject *parent = 0);

    //=========================================================================================================
    /**
     * Constructor which parses a command stored in a json object
     *
     * @param[in] p_sCommand         Command.
     * @param[in] p_bIsJson          If is received/should be send as JSON (optional, default true).
     * @param[in] parent             Parent QObject (optional).
     */
    explicit RawCommand(const QString &p_sCommand, bool p_bIsJson = true, QObject *parent = 0);

    //=========================================================================================================
    /**
     * Copy constructor.
     *
     * @param[in] p_rawCommand   RawCommand which should be copied.
     */
    explicit RawCommand(const RawCommand &p_rawCommand);

    //=========================================================================================================
    /**
     * Command name
     *
     * @return short command representation.
     */
    inline QString command() const;

    //=========================================================================================================
    /**
     * Returns the number of parameters.
     *
     * @return number of parameters.
     */
    inline quint32 count() const;

    virtual void execute();

    //=========================================================================================================
    /**
     * Returns whether the received command was in Json format.
     *
     * @return true if received command was in Json format, false otherwise.
     */
    inline bool isJson() const;

    //=========================================================================================================
    /**
     * Returns parameter values
     *
     * @return parameter values.
     */
    inline QList<QString>& pValues();

    //=========================================================================================================
    /**
     * Assignment Operator
     *
     * @param[in] rhs    RawCommand which should be assigned.
     */
    RawCommand& operator= (const RawCommand &rhs);

signals:
    //=========================================================================================================
    /**
     * Signal which is emitted when command patterns execute method is processed.
     *
     * @param[in] p_qListParameters    Parameter List.
     */
    void executed(QList<QString> p_qListParameters);

private:
    QString m_sCommand;
    bool m_bIsJson;

    QList<QString> m_qListRawParameters;    /**< Raw parameters. Their type is not specified jet.*/
};

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

QString RawCommand::command() const
{
    return m_sCommand;
}

//=============================================================================================================

quint32 RawCommand::count() const
{
    return m_qListRawParameters.size();
}

//=============================================================================================================

bool RawCommand::isJson() const
{
    return m_bIsJson;
}

//=============================================================================================================

QList<QString>& RawCommand::pValues()
{
    return m_qListRawParameters;
}
} // NAMESPACE

#endif // RAWCOMMAND_H
