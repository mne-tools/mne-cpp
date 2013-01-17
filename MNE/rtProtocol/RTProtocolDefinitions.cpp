/**
 * @author  Christof Pieloth
 */

#include <QStringList>

#include "RTProtocolDefinitions.h"

namespace RTSTREAMING
{
CommandRequestT createCommandRequest(const CommandT& cmd,
        const CommandArgListT& args)
{
    CommandRequestT request;
    request.append(cmd);
    if (!args.empty())
    {
        CommandArgListT::const_iterator it = args.begin();
        for (; it != args.end(); ++it)
        {
            request.append(ARG_SEPARATOR);
            request.append(*it);
        }
    }
    request.append(ETX);
    return request;
}

bool parseCommandRequest(const CommandRequestT& req, CommandT* cmd,
        CommandArgListT* args)
{
    if (!req.endsWith(ETX))
    {
        return false;
    }

    CommandRequestT request = req;
    request.remove(ETX);
    // TODO(pieloth): code smell - CommandRequestT -> QStringList
    QStringList splits = request.split(ARG_SEPARATOR);

    if (splits.empty())
    {
        return false;
    }
    QStringList::iterator it = splits.begin();
    *cmd = *it;

    args->clear();
    ++it;
    for (; it != splits.end(); ++it)
    {
        args->push_back(*it);
    }

    return true;
}
}
