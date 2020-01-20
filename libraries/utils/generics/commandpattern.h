//=============================================================================================================
/**
 * @file     commandpattern.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @version  1.0
 * @date     July, 2012
 *
 * @section  LICENSE
 *
 * Copyright (C) 2012, Lorenz Esch, Christoph Dinh. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *       following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 *       the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * @brief    Contains declarations of the command design pattern: ICommand interface.
 *
 */

#ifndef COMMANDPATTERN_H
#define COMMANDPATTERN_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../utils_global.h"


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QString>


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


#endif // COMMANDPATTERN_H
