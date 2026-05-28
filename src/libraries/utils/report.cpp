//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file report.cpp
 * @since 2026
 * @date  May 2026
 * @brief Implementation of @ref UTILSLIB::Report — builds the HTML scaffolding, escapes user content and writes the file to disk.
 *
 * HTML is assembled by streaming @c QString concatenations
 * directly into the section vector; no DOM model is kept in
 * memory. All caller-supplied strings are HTML-escaped before
 * being injected to avoid markup leakage from arbitrary file
 * names or stack traces that the application chooses to record
 * in the report.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "report.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QDateTime>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Report::Report(const QString& sTitle)
    : m_sTitle(sTitle)
{
}

//=============================================================================================================

void Report::addText(const QString& sTitle, const QString& sContent)
{
    ReportSection section;
    section.title = sTitle;
    section.htmlContent = "<p>" + sContent.toHtmlEscaped() + "</p>";
    m_sections.append(section);
}

//=============================================================================================================

void Report::addTable(const QString& sTitle,
                       const QStringList& headers,
                       const QList<QStringList>& rows)
{
    QString html = "<table>\n<thead><tr>\n";
    for (const auto& h : headers) {
        html += "  <th>" + h.toHtmlEscaped() + "</th>\n";
    }
    html += "</tr></thead>\n<tbody>\n";

    for (const auto& row : rows) {
        html += "<tr>\n";
        for (const auto& cell : row) {
            html += "  <td>" + cell.toHtmlEscaped() + "</td>\n";
        }
        html += "</tr>\n";
    }
    html += "</tbody>\n</table>";

    ReportSection section;
    section.title = sTitle;
    section.htmlContent = html;
    m_sections.append(section);
}

//=============================================================================================================

void Report::addKeyValue(const QString& sTitle,
                          const QList<QPair<QString, QString>>& pairs)
{
    QString html = "<dl>\n";
    for (const auto& pair : pairs) {
        html += "  <dt>" + pair.first.toHtmlEscaped() + "</dt>\n";
        html += "  <dd>" + pair.second.toHtmlEscaped() + "</dd>\n";
    }
    html += "</dl>";

    ReportSection section;
    section.title = sTitle;
    section.htmlContent = html;
    m_sections.append(section);
}

//=============================================================================================================

void Report::addCode(const QString& sTitle, const QString& sCode)
{
    ReportSection section;
    section.title = sTitle;
    section.htmlContent = "<pre><code>" + sCode.toHtmlEscaped() + "</code></pre>";
    m_sections.append(section);
}

//=============================================================================================================

QString Report::toHtml() const
{
    QString html;
    html += "<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n";
    html += "<meta charset=\"UTF-8\">\n";
    html += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n";
    html += "<title>" + m_sTitle.toHtmlEscaped() + "</title>\n";
    html += "<style>\n";
    html += "body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; ";
    html += "max-width: 960px; margin: 0 auto; padding: 20px; color: #333; }\n";
    html += "h1 { color: #2c3e50; border-bottom: 2px solid #3498db; padding-bottom: 10px; }\n";
    html += "h2 { color: #2980b9; margin-top: 30px; }\n";
    html += "table { border-collapse: collapse; width: 100%; margin: 10px 0; }\n";
    html += "th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }\n";
    html += "th { background-color: #3498db; color: white; }\n";
    html += "tr:nth-child(even) { background-color: #f2f2f2; }\n";
    html += "dl { margin: 10px 0; }\n";
    html += "dt { font-weight: bold; margin-top: 8px; }\n";
    html += "dd { margin-left: 20px; color: #555; }\n";
    html += "pre { background-color: #f8f8f8; border: 1px solid #ddd; padding: 12px; ";
    html += "border-radius: 4px; overflow-x: auto; }\n";
    html += "code { font-family: 'SF Mono', Menlo, Monaco, monospace; font-size: 0.9em; }\n";
    html += ".footer { margin-top: 40px; padding-top: 10px; border-top: 1px solid #ddd; ";
    html += "color: #999; font-size: 0.85em; }\n";
    html += "</style>\n</head>\n<body>\n";

    html += "<h1>" + m_sTitle.toHtmlEscaped() + "</h1>\n";

    for (const auto& section : m_sections) {
        html += "<h2>" + section.title.toHtmlEscaped() + "</h2>\n";
        html += section.htmlContent + "\n";
    }

    html += "<div class=\"footer\">\n";
    html += "<p>Generated by MNE-CPP on " + QDateTime::currentDateTime().toString(Qt::ISODate) + "</p>\n";
    html += "</div>\n";
    html += "</body>\n</html>\n";

    return html;
}

//=============================================================================================================

bool Report::save(const QString& sPath) const
{
    QFile file(sPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "[Report::save] Cannot open:" << sPath;
        return false;
    }

    QTextStream out(&file);
    out << toHtml();
    file.close();
    return true;
}
