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
#include <iostream>
#include <stdexcept>

#include "pluginparams.h"
#include "templatefile.h"

using std::cout;
using std::endl;

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================
#include <QFile>
#include <QRegularExpression>
#include <QSharedPointer>

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

    /**
     * @brief updateTestsProjectFile creates modifies the testframes project file to include your new plugin
     * @param params contains information about the desired output location of the new plugin's files.
     *
     * Adds a the plugin testframe to the non-minimal build only. This should cover just about all use cases,
     * but you may override and modify if you need any other kind of behavior.
     */
    virtual void updateTestsProjectFile(const PluginParams& params) const;

    /**
     * @brief copyTestTemplates creates a minimal test suite for your plugin.
     * @param params contains information about the desired output location of the new plugin's files.
     *
     * Creates a minimal test suite containing only the basic Qt libraries. If you want to import your
     * plugin and run tests against, it you must either manually pudate the project file to include the
     * librarries you need, or override this method in sucblassses of `IPluginCreator` and provide the
     * logic to build a fully functional test suite there.
     */
    virtual void copyTestTemplates(const PluginParams& params) const;

    /**
     * @brief readFile loads the text from a file.
     * @param filepath the file to read from.
     * @return the raw text from the file.
     */
    QString readFile(const QString& filepath) const;

    /**
     * @brief overwriteFile changes the content of a file on disk.
     * @param filepath to the file you wish to overwrite.
     * @param text is the new content of the file.
     */
    void overwriteFile(const QString& filepath, const QString& text) const;

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

    /**
     * @brief openFile opens a file for both reading and writing. Throws a descriptive error if there is problem.
     * @param filepath is the file to open
     * @return a pointer to the now opened file
     *
     * The file will be opened for reading and writing. If no file exists an exception will be thrown.
     * The caller is responsible for closing the file when done with it!
     */
    QSharedPointer<QFile> openFile(const QString& filepath) const;
};

#endif // PLUGINCREATOR_H
