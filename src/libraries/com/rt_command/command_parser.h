//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file command_parser.h
 * @since 2026
 * @date  March 2026
 * @brief Front-end tokeniser that turns incoming JSON or CLI payloads into @ref COMLIB::RawCommand objects and notifies attached managers.
 *
 * @ref COMLIB::CommandParser owns the @c Subject side of the observer
 * pattern in @c utils/generics: every @ref COMLIB::CommandManager that
 * wants to react to a slice of the command vocabulary attaches itself,
 * and the parser then fans every accepted request out to all of them.
 * That decoupling lets multiple subsystems (data acquisition, source
 * estimation, GUI plumbing) each register only the commands they know
 * how to handle without growing a single monolithic dispatcher.
 *
 * Two wire dialects are accepted on the same entry point. CLI input is
 * whitespace-tokenised into a command keyword plus string arguments;
 * JSON input is decoded with @c QJsonDocument and may contain either a
 * single command object or an array of commands, allowing batched
 * requests in one round-trip. In either case the result is the same
 * untyped @ref RawCommand handed to the observers — the type-aware
 * @ref Command is constructed downstream once a manager has identified
 * which schema applies.
 *
 * @c parse() returns @c true when at least one observer claimed the
 * input and additionally records the names of the recognised commands
 * in @c p_qListCommandsParsed; the @c response(QString, Command) signal
 * is the reverse channel used by managers to push reply text back to
 * whichever socket originally produced the request.
 */

#ifndef COMMANDPARSER_H
#define COMMANDPARSER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../com_global.h"
#include "raw_command.h"
#include "command.h"

#include <utils/generics/observerpattern.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QObject>
#include <QVector>
#include <QMultiMap>

//=============================================================================================================
// DEFINE NAMESPACE COMLIB
//=============================================================================================================

namespace COMLIB
{

//=============================================================================================================
/**
 * @brief Observer-pattern subject that decodes JSON or CLI command strings into @ref RawCommand and notifies attached managers.
 *
 * Single entry point @c parse() accepts either dialect, builds a
 * @ref RawCommand, and broadcasts it to every attached
 * @ref CommandManager; the @c response signal carries replies back from
 * whichever manager handled the request so the caller (typically a
 * @ref RtCmdClient) can forward them to the originating socket.
 */
class COMSHARED_EXPORT CommandParser : public QObject, public UTILSLIB::Subject
{
    Q_OBJECT

public:

    //=========================================================================================================
    /**
     * Default constructor
     *
     * @param[in] parent     Parent QObject (optional).
     */
    explicit CommandParser(QObject *parent = 0);

    //=========================================================================================================
    /**
     * Checks if a command exists
     *
     * @param[in] p_sCommand     Command to check.
     *
     * @return true if command exists, false otherwise.
     */
    bool exists(const QString& p_sCommand);

    //=========================================================================================================
    /**
     * Parses a CLI command or JSON command (list) and notifies all attached observers (command managers)
     *
     * @param[in] p_sInput               Input to parse.
     * @param[out] p_qListCommandsParsed  List of parsed commands.
     */
    bool parse(const QString &p_sInput, QStringList &p_qListCommandsParsed);

    //=========================================================================================================
    /**
     * Returns the stored RawCommand
     *
     * @return the stored RawCommand.
     */
    inline RawCommand& getRawCommand();

signals:
    //=========================================================================================================
    /**
     * Response channel which is used by attached observers (command managers) to send data back to subject
     *
     *@param[in] p_sResponse     Observer response/data.
     *@param[in] p_command       Command which send the response
     */
    void response(QString p_sResponse, Command p_command);

private:
    RawCommand m_rawCommand;
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

RawCommand& CommandParser::getRawCommand()
{
    return m_rawCommand;
}
} // NAMESPACE

#endif // COMMANDPARSER_H
