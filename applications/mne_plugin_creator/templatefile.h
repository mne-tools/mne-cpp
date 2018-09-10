//=============================================================================================================
/**
* @file     templatefile.h
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
* @brief    Contains the declaration of the TemplateFile class.
*
*/
#ifndef TEMPLATE_H
#define TEMPLATE_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================
#include <stdexcept>

#include "pluginparams.h"

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================
#include <QDate>
#include <QDir>
#include <QFile>
#include <QFileInfo>

//=============================================================================================================
/**
 * @brief The TemplateFile creates new plugin files from templated text files.
 *
 * Takes a text file containing variables encloesd in {{ }} and substitutes values from `PluginParameters` into
 * them.
 */
class TemplateFile {
public:
    /**
     * @brief TemplateFile creates a new instance of TemplateFile with a source and destination.
     * @param filepath to a textfile containing variables enclosed in {{ }}.
     * @param destinationPath contains the filepath at which the new output file will be created.
     */
    TemplateFile(const QString& filepath, const QString& destinationPath);

    /**
     * @brief fill performs loading, substitution, and output.
     * @param params contains variable information to be inserted into the template.
     *
     * This function will first attempt to load and read the text from the source template. The
     * source template should contain variable names enclosed in double brackets, e.g. {{author}}.
     * It will then perform substitution and attempt to save the output to a new file specified by
     * the params argument. It will automatically create directories as needed, but if there are
     * any problems, such as naming conflicts, it will fail and throw an exception instead of
     * overwriting an existing file.
     */
    void fill(const PluginParams& params);

private:
    /**
     * @brief openFiles prepares files for reading and creates any directories and files that
     * will be written to.
     */
    void openFiles();

    /**
     * @brief closeFiles should be called when all read and write operations are complete to
     * cleanup and close open file descriptors.
     */
    void closeFiles();

    /**
     * @brief m_outfile the file that will be created and written to when `fill` is called.
     */
    QFile m_outfile;

    /**
     * @brief m_template is the template file that will be read from and substituted into.
     */
    QFile m_template;
};

#endif // TEMPLATE_H
