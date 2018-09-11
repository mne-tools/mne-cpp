//=============================================================================================================
/**
* @file     mnescanplugincreator.cpp
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
* @brief    Definition of the MNEScanPluginCreator class.
*
*/
#include "mnescanplugincreator.h"

//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================
MNEScanPluginCreator::MNEScanPluginCreator()
    : IPluginCreator()
    , m_templatesPath("../../../mne-cpp/applications/mne_plugin_creator/templates/mne_scan/")
    , m_pluginsPath("../../../mne-cpp/applications/mne_scan/plugins/")
    , m_profilePath(m_pluginsPath + "plugins.pro")
{
}

//=============================================================================================================

QList<QPair<QString, QString>> MNEScanPluginCreator::templateInputOutputPairs(const PluginParams& params) const
{
    const QString name = params.m_name;
    const QString root = srcPath(name);
    const QString icons = iconsPath(name);
    const QString forms = formsPath(name);
    QList<QPair<QString, QString>> pairs;

    // Config files, etc.
    pairs.append(QPair<QString, QString>(m_templatesPath + "template.pro", root + params.m_proFileName));
    pairs.append(QPair<QString, QString>(m_templatesPath + "template.json", root + params.m_jsonFileName));
    pairs.append(QPair<QString, QString>(m_templatesPath + "options.png", icons + "options.png"));

    // Header files
    pairs.append(QPair<QString, QString>(m_templatesPath + "template_global.h", root + params.m_globalsFileName));
    pairs.append(QPair<QString, QString>(m_templatesPath + "headertemplate.h", root + params.m_headerFileName));
    pairs.append(QPair<QString, QString>(m_templatesPath + "widgettemplate.h", forms + params.m_widgetHeaderFileName));
    pairs.append(QPair<QString, QString>(m_templatesPath + "setupwidgettemplate.h", forms + params.m_setupWidgetHeaderFileName));
    pairs.append(QPair<QString, QString>(m_templatesPath + "aboutwidgettemplate.h", forms + params.m_aboutWidgetHeaderFileName));

    // Source files
    pairs.append(QPair<QString, QString>(m_templatesPath + "sourcetemplate.cpp", root + params.m_sourceFileName));
    pairs.append(QPair<QString, QString>(m_templatesPath + "widgettemplate.cpp", forms + params.m_widgetSourceFileName));
    pairs.append(QPair<QString, QString>(m_templatesPath + "setupwidgettemplate.cpp", forms + params.m_setupWidgetSourceFileName));
    pairs.append(QPair<QString, QString>(m_templatesPath + "aboutwidgettemplate.cpp", forms + params.m_aboutWidgetSourceFileName));

    // Form files
    pairs.append(QPair<QString, QString>(m_templatesPath + "widgettemplate.ui", forms + params.m_widgetFormFileName));
    pairs.append(QPair<QString, QString>(m_templatesPath + "setupwidgettemplate.ui", forms + params.m_setupWidgetFormFileName));
    pairs.append(QPair<QString, QString>(m_templatesPath + "aboutwidgettemplate.ui", forms + params.m_aboutWidgetFormFileName));

    return pairs;
}

//=============================================================================================================

void MNEScanPluginCreator::updateProjectFile(const PluginParams& params) const
{
    if (params.m_superclass == "ISensor") {
        updateProjectFileForSensor(params);
    } else if (params.m_superclass == "IAlgorithm") {
        updateProjectFileForAlgorithm(params);
    } else {
        throw std::invalid_argument("Invalid superclass! Expected 'algorithm' or 'sensor', but got " + params.m_superclass.toStdString());
    }
}

//=============================================================================================================

void MNEScanPluginCreator::updateProjectFileForAlgorithm(const PluginParams& params) const
{
    QByteArray text = readFile(m_profilePath).toUtf8();
    QRegularExpression regex;
    regex.setPattern("\\s*Algorithms\\s*(SUBDIRS\\s*\\+=\\s*\\\\)");
    regex.setPatternOptions(QRegularExpression::MultilineOption);
    QRegularExpressionMatchIterator matches = regex.globalMatch(text);

    // We only want to operate on the second match. The first match is the minimal build. See plugins.pro
    matches.next();
    text.insert(matches.next().capturedEnd(), "\n\t" + folderName(params.m_name) + " \\");
    overwriteFile(m_profilePath, text);
}

//=============================================================================================================

void MNEScanPluginCreator::updateProjectFileForSensor(const PluginParams& params) const
{
    QByteArray text = readFile(m_profilePath).toUtf8();
    QRegularExpression regex;
    regex.setPattern("\\s*Sensors\\s*(SUBDIRS\\s*\\+=\\s*\\\\)");
    regex.setPatternOptions(QRegularExpression::MultilineOption);
    QRegularExpressionMatchIterator matches = regex.globalMatch(text);
    QString insertText = "\n\t" + folderName(params.m_name) + " \\";
    quint32 charsInserted = 0;

    while (matches.hasNext()) {
        QRegularExpressionMatch match = matches.next();
        text.insert(match.capturedEnd() + charsInserted, insertText);
        charsInserted += insertText.length();
    }
    overwriteFile(m_profilePath, text);
}

//=============================================================================================================

QString MNEScanPluginCreator::folderName(const QString& pluginName) const { return pluginName.toLower(); }

//=============================================================================================================

QString MNEScanPluginCreator::srcPath(const QString& pluginName) const { return m_pluginsPath + folderName(pluginName) + "/"; }

//=============================================================================================================

QString MNEScanPluginCreator::iconsPath(const QString& pluginName) const { return srcPath(pluginName) + "images/icons/"; }

//=============================================================================================================

QString MNEScanPluginCreator::formsPath(const QString& pluginName) const { return srcPath(pluginName) + "FormFiles/"; }
