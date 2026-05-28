//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     report.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.1
 * @date     May 2026
 * @brief    HTML report builder — mne-cpp's equivalent of @c mne.Report for summarising analysis pipelines.
 *
 * @ref UTILSLIB::Report collects titled sections (free text,
 * tables, key-value pairs, preformatted code) and emits a
 * self-contained HTML document that can be opened directly in
 * a browser or attached to an email. The output is dependency-
 * free — inline CSS, no JavaScript — so the file remains
 * readable in restricted enterprise environments where the
 * MEG/EEG processing reports are typically reviewed.
 *
 * Used by the demo applications in @c src/applications and by
 * the integration tests in @c tools/ to produce a one-page
 * summary of each pipeline run (input files, channel counts,
 * forward / inverse parameters, output checksums).
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
