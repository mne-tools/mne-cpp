//=============================================================================================================
/**
 * @file     report.cpp
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
 * @brief    Report implementation.
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
