//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2017-2026 MNE-CPP Authors
 *
 * @file     command_pattern.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     September 2017
 * @brief    Minimal @c ICommand interface of the command pattern behind the real-time command classes.
 *
 * @ref RTSERVER::Command and @ref RTSERVER::RawCommand implement it to model the control requests exchanged with
 * mne_rt_server (start or stop a measurement, set parameters, request status). Invokers and receivers are
 * expressed through callbacks rather than dedicated classes.
 */

#ifndef RTSERVER_COMMAND_PATTERN_H
#define RTSERVER_COMMAND_PATTERN_H

//=============================================================================================================
// DEFINE NAMESPACE RTSERVER
//=============================================================================================================

namespace RTSERVER
{

//=============================================================================================================
/**
 * @brief Base of every command: an action that can be queued and executed later.
 *
 * @snippet ex_com/main.cpp command_pattern_usage
 */
class ICommand
{
public:
    virtual ~ICommand() = default;

    //=========================================================================================================
    /**
     * Performs the action.
     */
    virtual void execute() = 0;
};

} // namespace RTSERVER

#endif // RTSERVER_COMMAND_PATTERN_H
