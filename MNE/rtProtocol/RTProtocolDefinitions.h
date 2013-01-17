/**
 * Specification of the basic data types for the transfer protocol.
 *
 * @author  Christof Pieloth
 */

#ifndef RTPROTOCOLDEFINITIONS_H_
#define RTPROTOCOLDEFINITIONS_H_

#include <list>
#include <QString>

namespace RTSTREAMING
{
typedef QString CommandRequestT; /**< Type for a complete command request packet */

typedef CommandRequestT CommandT; /**< Type for a request command. */
typedef CommandRequestT CommandArgT; /**< Type for an argument of a request command. */
typedef std::list<CommandArgT> CommandArgListT; /**< Type for an argument list of a request command. */

// TODO test newline and termination characters
const CommandRequestT ARG_SEPARATOR = " "; /** Separator for arguments. */
const CommandRequestT DESC_SEPARATOR = "\t"; /** Separator for description text. */
const CommandRequestT NEWLINE = "\n"; /** Telnet newline character. */
const CommandRequestT ETX = "\x0D\x0A"; /**< Telnet "message" termination: CRLF */

typedef quint8 DataTypeT; /**< Type for packet header field "Data Type". */
typedef unsigned char DataT; /**< Type for user data (byte). */
typedef quint64 SizeT; /**< Type for packet header field "Data length". */

const DataT EOT = 0x04; /**< Termination byte for user data sequence. */

/**
 * Creates a request packet out of the command and the argument list.
 * Low-level API to create a command request.
 *
 * @param   cmd Command for request.
 * @param   args A list of arguments for the request.
 *
 * @return  Ready-to-send request packet
 */
CommandRequestT createCommandRequest(const CommandT& cmd,
        const CommandArgListT& args);

/**
 * Splits an incoming CommandRequestT packet to the CommandT and CommandArgListT parts.
 * Low-level API to parse a command request.
 *
 * @param   req Request to parse.
 * @param   cmd Pointer to store the command.
 * @param   args Pointer to store argument list. Remaining items are lost!
 *
 * @return  true on success, false on error e.g. wrong structure.
 */
bool parseCommandRequest(const CommandRequestT& req, CommandT* cmd,
        CommandArgListT* args);
}

#endif /* RTPROTOCOLDEFINITIONS_H_ */
