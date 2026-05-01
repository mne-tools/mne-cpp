//=============================================================================================================
/**
 * @file     report.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *       following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 *       the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * @brief    HTML report generation for MNE-CPP analysis results.
 *
 * Equivalent to MNE-Python's mne.Report for generating structured
 * HTML reports from MEG/EEG analysis pipelines.
 */

#ifndef REPORT_UTILS_H
#define REPORT_UTILS_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "utils_global.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QString>
#include <QStringList>
#include <QList>
#include <QPair>

//=============================================================================================================
// DEFINE NAMESPACE UTILSLIB
//=============================================================================================================

namespace UTILSLIB
{

//=============================================================================================================
/**
 * @brief A section in the report (title + HTML content).
 */
struct UTILSSHARED_EXPORT ReportSection
{
    QString title;
    QString htmlContent;
};

//=============================================================================================================
/**
 * @brief Simple HTML report builder for MEG/EEG analysis results.
 *
 * Usage:
 * @code
 *   Report report("My Analysis");
 *   report.addText("Data Info", "Subject: sub-01, 306 MEG channels");
 *   report.addTable("Channel Stats", headers, rows);
 *   report.addKeyValue("Parameters", {{"Lambda", "0.1"}, {"SNR", "3.0"}});
 *   report.save("/path/to/report.html");
 * @endcode
 */
class UTILSSHARED_EXPORT Report
{
public:
    //=========================================================================================================
    /**
     * @brief Construct a report with a title.
     */
    explicit Report(const QString& sTitle = "MNE-CPP Report");

    //=========================================================================================================
    /**
     * @brief Add a text section.
     *
     * @param[in] sTitle    Section title.
     * @param[in] sContent  Text content (plain text or HTML).
     */
    void addText(const QString& sTitle, const QString& sContent);

    //=========================================================================================================
    /**
     * @brief Add a table section.
     *
     * @param[in] sTitle    Section title.
     * @param[in] headers   Column headers.
     * @param[in] rows      Row data (list of string lists).
     */
    void addTable(const QString& sTitle,
                   const QStringList& headers,
                   const QList<QStringList>& rows);

    //=========================================================================================================
    /**
     * @brief Add a key-value section.
     *
     * @param[in] sTitle    Section title.
     * @param[in] pairs     List of (key, value) pairs.
     */
    void addKeyValue(const QString& sTitle,
                      const QList<QPair<QString, QString>>& pairs);

    //=========================================================================================================
    /**
     * @brief Add a preformatted code block.
     */
    void addCode(const QString& sTitle, const QString& sCode);

    //=========================================================================================================
    /**
     * @brief Get the number of sections.
     */
    int sectionCount() const { return m_sections.size(); }

    //=========================================================================================================
    /**
     * @brief Generate the full HTML string.
     */
    QString toHtml() const;

    //=========================================================================================================
    /**
     * @brief Save the report to an HTML file.
     *
     * @param[in] sPath  Output file path.
     *
     * @return true if successful.
     */
    bool save(const QString& sPath) const;

    //=========================================================================================================
    /**
     * @brief Get the report title.
     */
    const QString& title() const { return m_sTitle; }

private:
    QString m_sTitle;
    QList<ReportSection> m_sections;
};

} // namespace UTILSLIB

#endif // REPORT_UTILS_H
