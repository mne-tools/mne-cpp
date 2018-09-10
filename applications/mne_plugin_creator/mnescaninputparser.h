//=============================================================================================================
/**
* @file     mnescaninputparser.h
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
* @brief    Contains the declaration of the MNEScanInputParser class.
*
*/
#ifndef MNESCANINPUTPARSER_H
#define MNESCANINPUTPARSER_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================
#include "iinputparser.h"
#include "pluginparams.h"

//=============================================================================================================
/**
 * @brief The MNEScanInputParser class parses input for creating MNE Scan Plugins, including both
 * IAlgorithm and ISensor plugins.
 */
class MNEScanInputParser : public IInputParser {

public:
    /**
     * @brief MNEScanInputParser creates a new instace of `MNEScanInputParser`.
     */
    MNEScanInputParser();

    /**
     * @brief parseUserInput asks the user for all the information necessary to create an MNE Scan plugin.
     * @return params contains information about the desired output location of the new plugin's files.
     */
    PluginParams parseUserInput() override;

private:
    /**
     * @brief getSuperClassName asks the user if they want to create an algorithm or a sensor plugin.
     * @return "IAlgorithm" or "IPlugin"
     */
    QString getSuperClassName();
};

#endif // MNESCANINPUTPARSER_H
