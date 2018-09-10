//=============================================================================================================
/**
* @file     pluginparams.cpp
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
* @brief    Definition of the PluginParams struct.
*
*/
#include "pluginparams.h"

//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================
PluginParams::PluginParams(const QString& name, const QString& superclass, const QString& nameSpace, const QString author, const QString& email)
{
    const QString baseFileName = name.toLower();
    const QString baseDefine = name.toUpper();

    // Class names
    m_name = name;
    m_superclass = superclass;
    m_widgetName = name + "Widget";
    m_aboutWidgetName = name + "AboutWidget";
    m_setupWidgetName = name + "SetupWidget";

    // Basic parameters
    m_email = email;
    m_author = author;
    m_namespace = nameSpace;
    m_targetName = baseFileName;

    // File names
    m_proFileName = baseFileName + ".pro";
    m_jsonFileName = baseFileName + ".json";
    m_globalsFileName = baseFileName + "_global.h";

    // Header files
    m_headerFileName = baseFileName + ".h";
    m_widgetHeaderFileName = baseFileName + "widget.h";
    m_aboutWidgetHeaderFileName = baseFileName + "aboutwidget.h";
    m_setupWidgetHeaderFileName = baseFileName + "setupwidget.h";

    // Source files
    m_sourceFileName = baseFileName + ".cpp";
    m_widgetSourceFileName = baseFileName + "widget.cpp";
    m_aboutWidgetSourceFileName = baseFileName + "aboutwidget.cpp";
    m_setupWidgetSourceFileName = baseFileName + "setupwidget.cpp";

    // Form files
    m_widgetFormFileName = baseFileName + "widget.ui";
    m_aboutWidgetFormFileName = baseFileName + "aboutwidget.ui";
    m_setupWidgetFormFileName = baseFileName + "setupwidget.ui";

    // #define
    m_globalHeaderDefine = baseDefine + "_GLOBAL_H";
    m_libraryDefine = baseDefine + "_LIBRARY";
    m_exportDefine = baseDefine + "SHARED_EXPORT";
    m_headerDefine = baseDefine + "_H";
    m_widgetHeaderDefine = baseDefine + "WIDGET_H";
    m_setupWidgetHeaderDefine = baseDefine + "SETUPWIDGET_H";
    m_aboutWidgetHeaderDefine = baseDefine + "ABOUTWIDGET_H";
}
