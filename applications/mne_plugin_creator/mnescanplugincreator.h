//=============================================================================================================
/**
* @file     mnescanplugincreator.h
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
* @brief    Contains the declaration of the MNEScanPluginCreator class.
*
*/
#ifndef MNESCANPLUGINCREATOR_H
#define MNESCANPLUGINCREATOR_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================
#include "iplugincreator.h"

//=============================================================================================================
/**
 * @brief The MNEScanPluginCreator creates `IAlgorithm` and `ISensor` plugins for MNE Scan
 */
class MNEScanPluginCreator : public IPluginCreator {
public:
    /**
     * @brief MNEScanPluginCreator creates a new instance of `MNEScanPluginCreator`
     */
    MNEScanPluginCreator();

protected:
    /**
     * @brief templateInputOutputPairs returns a list detailing the template files and where to put them.
     * @param params contains information about the desired output location of the new plugin's files.
     * @return a list detailing the template files and where to put them.
     *
     * Returns a list in which each entry in the list contains a pair of strings, where the first string
     * is the path to a template file and the second is the path to where it should be copied, including the
     * new filename.
     */
    virtual QList<QPair<QString, QString>> templateInputOutputPairs(const PluginParams& params) const override;

    /**
     * @brief updateProjectFile inserts a new line into the .pro file for the new plugin.
     * @param params contains information about the desired output location of the new plugin's files.
     *
     * For `IAlgorithm` plugins, a line will be inserted into the algorithms section of the non-minimal build.
     * For `ISensor` plugins, lines will be inserted into the sensors section for all builds.
     */
    virtual void updateProjectFile(const PluginParams& params) const override;

private:
    /**
     * @brief folderName returns the name of the folder the plugin will be created in.
     */
    QString folderName(const QString& pluginName) const;

    /**
     * @brief srcPath returns the path to the folder the plugin will be created in.
     */
    QString srcPath(const QString& pluginName) const;

    /**
     * @brief iconsPath returns the path to the folder that icons will be stored in.
     */
    QString iconsPath(const QString& pluginName) const;

    /**
     * @brief formsPath returns the path to the folder that form files will be stored in.
     */
    QString formsPath(const QString& PluginName) const;

    /**
     * @brief updateProjectFileForSensor updates the .pro file specifically for `ISensor` plugins.
     * @param params contains information about the desired output location of the new plugin's files.
     */
    void updateProjectFileForSensor(const PluginParams& params) const;

    /**
     * @brief updateProjectFileForAlgorithm updates the .pro file specifically for `IAlgorithm` plugins.
     * @param params contains information about the desired output location of the new plugin's files.
     */
    void updateProjectFileForAlgorithm(const PluginParams& params) const;

    /**
     * @brief m_templatesPath holds the path to the folder that templates are stored in.
     */
    const QString m_templatesPath;

    /**
     * @brief m_pluginsPath holds the path to the folder that MNE Scan plugins reside in.
     */
    const QString m_pluginsPath;

    /**
     * @brief m_profilePath holds the .pro file's file path.
     */
    const QString m_profilePath;
};

#endif // MNESCANPLUGINCREATOR_H
