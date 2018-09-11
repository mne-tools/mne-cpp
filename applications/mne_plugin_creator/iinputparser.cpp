//=============================================================================================================
/**
* @file     iinputparser.cpp
* @author   Erik Hornberver <erik.hornberger@shi-g.com>;
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
* @brief    Definition of the IInputParser class.
*
*/
#include "iinputparser.h"

//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================
IInputParser::IInputParser()
    : in(stdin)
    , out(stdout)
{
}

IInputParser::~IInputParser() {}

//=============================================================================================================

QString IInputParser::getPluginName()
{
    out << "Enter a class name to be used for your new plugin (Use CamelCase):" << endl;
    return validateArbitraryInput();
}

//=============================================================================================================

QString IInputParser::getNamespace()
{
    out << "Enter a namespace for your new plugin to reside in:" << endl;
    return validateArbitraryInput();
}

//=============================================================================================================

QString IInputParser::getAuthorName()
{
    out << "Enter your (the author's) name. It will be inserted into the license." << endl;
    return acceptArbitraryInput();
}

//=============================================================================================================

QString IInputParser::getAuthorEmail()
{
    out << "Enter your (the author's) email address. It will be inserted into the license" << endl;
    return validateArbitraryInput();
}

//=============================================================================================================

QString IInputParser::validateFiniteOptionsInput(const QStringList& validInputs)
{
    QString value = in.readLine();
    if (!validInputs.contains(value)) {
        out << "Your input, " << value << ", is invalid!" << endl;
        out << "Valid options are: ";
        showOptions(validInputs);
        out << endl;
        return validateFiniteOptionsInput(validInputs);
    }
    out << endl;
    return value;
}

//=============================================================================================================

QString IInputParser::validateArbitraryInput()
{
    QString value = in.readLine();
    if (value.contains(" ")) {
        out << "Value may not contain any spaces!";
        return validateArbitraryInput();
    }
    out << endl;
    return value;
}

//=============================================================================================================

QString IInputParser::acceptArbitraryInput()
{
    QString value = in.readLine();
    out << endl;
    return value;
}

//=============================================================================================================

void IInputParser::showOptions(const QStringList& validInputs)
{
    out << "[";
    for (int i = 0; i < validInputs.length(); i++) {
        out << validInputs[i];
        if (i < validInputs.length() - 1) {
            out << ", ";
        }
    }
    out << "]";
}
