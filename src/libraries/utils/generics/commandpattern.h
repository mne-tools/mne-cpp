//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2017-2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *   Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 *   Gabriel Motta <gabrielbenmotta@gmail.com>
 *
 * @file commandpattern.h
 * @since September 2017
 * @brief Minimal @c ICommand interface for the GoF command pattern, used by the MNE Scan plugin runtime.
 *
 * The interface intentionally carries only a virtual @c execute()
 * method plus the standard @c QSharedPointer typedefs that every
 * UTILSLIB-managed object exposes. Invokers and receivers are
 * expressed via Qt's signal/slot mechanism rather than dedicated
 * classes, which keeps the surface small and avoids dragging in
 * a parallel object hierarchy.
 *
 * The header is included by MNE Scan's command parser and by
 * COMMUNICATIONLIB to model remote control packets (start /
 * stop acquisition, set parameters, request status).
 */

#ifndef COMMANDPATTERN_H
#define COMMANDPATTERN_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../utils_global.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QString>

//=============================================================================================================
// DEFINE NAMESPACE UTILSLIB
//=============================================================================================================

namespace UTILSLIB
{

//=============================================================================================================
/**
 * Declare interface command
 *
 * @brief The ICommand interface provides the base class of every command of the command design pattern.
 */
class ICommand
{
public:
    typedef QSharedPointer<ICommand> SPtr;             /**< Shared pointer type for ICommand. */
    typedef QSharedPointer<const ICommand> ConstSPtr;  /**< Const shared pointer type for ICommand. */

    //=========================================================================================================
    /**
     * Destroys the ICommand.
     */
    virtual ~ICommand() {};

    //=========================================================================================================
    /**
     * Executes the ICommand.
     */
    virtual void execute() = 0;
};

//Invoker
//--> Use signal/slot

//Receiver
//--> Use signal/slot

} // Namespace

#endif // COMMANDPATTERN_H
