//=============================================================================================================
/**
* @file     pluginparams.h
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
* @brief    Contains the declaration of the PluginParams struct.
*
*/
#ifndef PLUGINPARAMS_H
#define PLUGINPARAMS_H

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================
#include <QString>

//=============================================================================================================
/**
 * @brief The PluginParams struct hold information about what the user wants their plugin to be named and where
 * it should be saved.
 *
 * When you create a new instance of `PluginParams`, some reasonable default values will be chosen automatically
 * for you. If you ever want to change these values, it's as easy as updating the appropriate field before passing
 * the struct to an `IPluginCreator`.
 */
struct PluginParams {
    /**
     * @brief PluginParams creates a new structure will recommended values based on basic inputs.
     * @param app The name of the app the plugin will be created for.
     * @param name The (class) name of the plugin you wish to create.
     * @param superclass The class you want your plugin to inherit from.
     * @param nameSpace The namespace you wish for your new plugin to reside in.
     * @param author Your name.
     * @param email Your email address.
     *
     * Given some basic inputs, the remainder of the fields will be automatically populated with sensible default
     * values. You are welcome to change any of the values after initialization.
     */
    PluginParams(
        const QString& app, const QString& name, const QString& superclass, const QString& nameSpace, const QString author, const QString& email);

    // Meta data
    QString m_app;
    QString m_author;
    QString m_email;
    QString m_namespace;
    QString m_targetName;

    // Class names
    QString m_name;
    QString m_superclass;
    QString m_widgetName;
    QString m_aboutWidgetName;
    QString m_setupWidgetName;

    // Config file names
    QString m_proFileName;
    QString m_jsonFileName;
    QString m_globalsFileName;

    // Header file names
    QString m_headerFileName;
    QString m_widgetHeaderFileName;
    QString m_setupWidgetHeaderFileName;
    QString m_aboutWidgetHeaderFileName;

    // Source file names
    QString m_sourceFileName;
    QString m_widgetSourceFileName;
    QString m_setupWidgetSourceFileName;
    QString m_aboutWidgetSourceFileName;

    // Form file names
    QString m_widgetFormFileName;
    QString m_aboutWidgetFormFileName;
    QString m_setupWidgetFormFileName;

    // Header #define constants
    QString m_globalHeaderDefine;
    QString m_libraryDefine;
    QString m_exportDefine;
    QString m_headerDefine;
    QString m_widgetHeaderDefine;
    QString m_setupWidgetHeaderDefine;
    QString m_aboutWidgetHeaderDefine;
};

#endif // PLUGINPARAMS_H
