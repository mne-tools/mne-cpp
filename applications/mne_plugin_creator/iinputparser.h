//=============================================================================================================
/**
* @file     iinputparser.h
* @author   Erik Hornberger <erik.hornberger@shi-g.com>;
* @version  1.0
* @date     September, 2018
*
* @section  LICENSE
*
* Copyright (C) 2018, Sumitomo Heavy Industries, Ltd., Lorenz Esch and Matti Hamalainen. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that
* the following conditions are met:
*     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
*       following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
*       the following disclaimer in the documentation and/or other materials provided with the distribution.
*     * Neither the name of the Massachusetts General Hospital nor the names of its contributors may be used
*       to endorse or promote products derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MASSACHUSETTS GENERAL HOSPITAL BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*
* @brief    Contains the declaration of the IInputParser class.
*
*/
#ifndef IINPUTPARSER_H
#define IINPUTPARSER_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================
#include <iostream>
#include <stdexcept>

#include "pluginparams.h"

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================
#include <QTextStream>

//=============================================================================================================
/**
 * @brief The IInputParser class is an abstract base class for all user input parsers.
 *
 * This class provides base functionality for all user input parsers. You should subclass
 * `IInputParser` and override `parseUnserInput`.
 */
class IInputParser {
public:
    /**
     * @brief IInputParser creates a new instance of `IInputParser`.
     */
    IInputParser();

protected:
    /**
     * @brief parseUserInput asks the user about the plugin they wish to create.
     * @return Information about the plugin the user wishes to create.
     *
     * When you inherit `IInputParser`, you must provide an implementation for this method.
     */
    virtual PluginParams parseUserInput() = 0;

    /**
     * @brief getPluginName asks the user what they would like to name their plugin.
     * @return The class name of the new plugin.
     */
    QString getPluginName();

    /**
     * @brief getNamespace asks the user for the namespace to place the new plugin in.
     * @return The namespace for the new plugin.
     */
    QString getNamespace();

    /**
     * @brief getAuthorName asks the user for their (the author's) name.
     * @return The authors name
     */
    QString getAuthorName();

    /**
     * @brief getAuthorEmail asks the user to provide their email address.
     * @return  The user's email address.
     */
    QString getAuthorEmail();

    /**
     * @brief acceptArbitraryInput retrieves the users reply and performs no validation.
     * @return The user's reply.
     */
    QString acceptArbitraryInput();

    /**
     * @brief validateArbitraryInput retrieves the users reaply and performs basic validation.
     * @return  The user's reply
     *
     * Check to ensure that the input does not contain any spaces. This is useful for checking
     * things like namespaces and class names. If the user's input contains invalid characters,
     * they will be asked to try again until they give a proper response.
     */
    QString validateArbitraryInput();

    /**
     * @brief validateFiniteOptionsInput gets the user input and ensures it comes from a finite set.
     * @param validInputs is a list of inputs that should be accepted.
     * @return The user's validated response.
     *
     * Check to ensure that the users input matches a valid set of options. If the input is malformed,
     * the user will be asked to input their response again until they give a valid response.
     */
    QString validateFiniteOptionsInput(const QStringList& validInputs);
    void showOptions(const QStringList& validInputs);

    /**
     * @brief in is stdin wrapped in a text stream for convenient parsing.
     */
    QTextStream in;

    /**
     * @brief out is stdout wrapped in a text stream for convenient output.
     */
    QTextStream out;
};

#endif // IINPUTPARSER_H
