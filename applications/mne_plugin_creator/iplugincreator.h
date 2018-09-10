//=============================================================================================================
/**
* @file     iplugincreator.h
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
* @brief    Contains the declaration of the IPluginCreator class.
*
*/
#ifndef PLUGINCREATOR_H
#define PLUGINCREATOR_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================
#include <stdexcept>

#include "pluginparams.h"
#include "templatefile.h"

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================
#include <QDebug>
#include <QRegularExpression>

//=============================================================================================================
/**
 * @brief The IPluginCreator class is the parent class of all plugin creators.
 *
 * The `IPluginCreator` class provides basic functionality for all varieties of plugin creators. There should
 * be one subclas of `IPluginCreator` for each MNE app that support extensibility via plugins.
 */
class IPluginCreator {
public:
    /**
     * @brief IPluginCreator creates a new IPluginCreator`.
     */
    IPluginCreator();

    /**
     * @brief createPlugin creates a new plugin.
     * @param params contains information about the desired output location of the new plugin's files.
     */
    void createPlugin(PluginParams& params);

protected:
    /**
     * @brief templateInputOutputPairs returns a list of template files and where they should be copied to.
     * @param params contains information about the desired output location of the new plugin's files.
     * @return a list of source-destination pairs for each template
     *
     * Each subclass of `IPluginCreator` must provide an implementation for this method. You should create
     * and return a list in which each entry in the list contains a pair of strings, where the first string
     * is the path to a template file and the second is the path to where it should be copied, including the
     * new filename.
     *
     * Note that you `IPluginCreator` will take care of creating directories for you, so you can simply
     * provide the paths you want the files to end up at, and if they don't exist yet, they will be created
     * automatically.
     */
    virtual QList<QPair<QString, QString>> templateInputOutputPairs(const PluginParams& params) const = 0;

    /**
     * @brief updateProjectFile updates the app's project file to include the new plugin.
     * @param params contains information about the desired output location of the new plugin's files.
     *
     * When a new plugin is created, the applications .pro file must be updated to include the plugin.
     * Perform that manipulation here. If anything goes wrong, you should throw a std::exception.
     */
    virtual void updateProjectFile(const PluginParams& params) const = 0;

private:
    /**
     * @brief copyTemplates performs the work of copying templates and filling them in.
     * @param params contains information about the desired output location of the new plugin's files.
     *
     * This method calls `templateInputOutputPairs` and proceed load, execute, and save each template file.
     * It will automatically create new folders and files as needed. If anything goes wrong, for example if
     * there is a naming conflict, it will throw exceptions.
     */
    void copyTemplates(const PluginParams& params) const;
};

#endif // PLUGINCREATOR_H
