//=============================================================================================================
/**
* @file     appinputparser.cpp
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
* @brief    Definition of the AppInputParser class.
*
*/
#include "iplugincreator.h"

//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================
IPluginCreator::IPluginCreator() {}

//=============================================================================================================

void IPluginCreator::createPlugin(PluginParams& params)
{
    cout << "Creating plugin: " << params.m_name.toStdString() << "..." << endl;
    copyTemplates(params);
    cout << "Copied and filled templates!" << endl;
    updateProjectFile(params);
    cout << "Finished creating new " << params.m_name.toStdString() << " plugin!" << endl;

    cout << "Creating test suite for your new plugin..." << endl;
    copyTestTemplates(params);
    cout << "Copied and filled test suite templates!" << endl;
    updateTestsProjectFile(params);
    cout << "Finished creating test suite!" << endl;
}

//=============================================================================================================

void IPluginCreator::copyTemplates(const PluginParams& params) const
{
    for (QPair<QString, QString> pair : templateInputOutputPairs(params)) {
        TemplateFile templateFile(pair.first, pair.second);
        templateFile.fill(params);
    }
}

void IPluginCreator::copyTestTemplates(const PluginParams& params) const
{
    const QString templateDir = "../../../mne-cpp/applications/mne_plugin_creator/templates/testframes/";
    const QString testDirectory = "../../../mne-cpp/testframes/" + params.m_targetName + "/";

    const QString proFileTemplate = templateDir + "template.pro";
    const QString proFilePath = testDirectory + "test_" + params.m_targetName + ".pro";
    TemplateFile(proFileTemplate, proFilePath).fill(params);

    const QString sourceTemplate = templateDir + "template.cpp";
    const QString sourcePath = testDirectory + "test_" + params.m_targetName + ".cpp";
    TemplateFile(sourceTemplate, sourcePath).fill(params);
}

void IPluginCreator::updateTestsProjectFile(const PluginParams& params) const
{
    const QString testsProFilePath = "../../../mne-cpp/testframes/testframes.pro";
    QByteArray text = readFile(testsProFilePath).toUtf8();
    QRegularExpression regex;
    regex.setPattern("\\s*(SUBDIRS\\s*\\+=\\s*\\\\)");
    regex.setPatternOptions(QRegularExpression::MultilineOption);
    QRegularExpressionMatchIterator matches = regex.globalMatch(text);

    // Only insert into the first match. The second match is for the minimal build.
    text.insert(matches.next().capturedEnd(), "\n\t" + params.m_targetName + " \\");
    overwriteFile(testsProFilePath, text);
}

QSharedPointer<QFile> IPluginCreator::openFile(const QString& filepath) const
{
    QSharedPointer<QFile> proFile = QSharedPointer<QFile>(new QFile(filepath));
    if (!proFile->exists()) {
        throw std::invalid_argument(filepath.toStdString() + "could not be found!");
    }

    const bool success = proFile->open(QIODevice::ReadWrite | QIODevice::Text);
    if (!success) {
        QString filename = proFile->fileName();
        QString problem = proFile->errorString();
        throw std::runtime_error("Unable to open profile: " + filename.toStdString() + "\nError: " + problem.toStdString());
    }

    return QSharedPointer<QFile>(proFile);
}

QString IPluginCreator::readFile(const QString& filepath) const
{
    QSharedPointer<QFile> proFile = openFile(filepath);
    QByteArray text = proFile->readAll();
    proFile->close();
    return text;
}

void IPluginCreator::overwriteFile(const QString& filepath, const QString& text) const
{
    QSharedPointer<QFile> file = openFile(filepath);
    file->resize(0);
    file->write(text.toUtf8());
    file->close();
}
