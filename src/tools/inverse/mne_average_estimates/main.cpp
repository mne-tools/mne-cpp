//=============================================================================================================
/**
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
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
 * @brief    Compute a weighted average of STC source estimate files.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <inv/inv_source_estimate.h>
#include <utils/generics/mne_logger.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QFile>
#include <QRegularExpression>
#include <QTextStream>
#include <QDebug>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace INVLIB;
using namespace UTILSLIB;
using namespace Eigen;

//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

#define PROGRAM_VERSION MNE_CPP_VERSION

//=============================================================================================================

int main(int argc, char *argv[])
{
    qInstallMessageHandler(MNELogger::customLogWriter);
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("mne_average_estimates");
    QCoreApplication::setApplicationVersion(PROGRAM_VERSION);

    QCommandLineParser parser;
    parser.setApplicationDescription("Compute a weighted average of STC source estimate files.");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption descOpt("desc",
        "Description file listing <weight> <stc-path> pairs (one per line).", "file");
    parser.addOption(descOpt);

    QCommandLineOption outOpt("out", "Output STC file path.", "file");
    parser.addOption(outOpt);

    parser.process(app);

    if (!parser.isSet(descOpt) || !parser.isSet(outOpt)) {
        qCritical() << "Both --desc and --out are required.";
        parser.showHelp(1);
    }

    //--- Parse description file ---
    QFile descFile(parser.value(descOpt));
    if (!descFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCritical() << "Cannot open description file:" << parser.value(descOpt);
        return 1;
    }

    QList<QPair<double, QString>> entries;
    QTextStream in(&descFile);
    int lineNum = 0;
    while (!in.atEnd()) {
        ++lineNum;
        QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#'))
            continue;

        QStringList parts = line.split(QRegularExpression("\\s+"));
        if (parts.size() < 2) {
            qCritical() << "Invalid line" << lineNum << "in description file (expected: <weight> <path>).";
            return 1;
        }

        bool ok;
        double weight = parts[0].toDouble(&ok);
        if (!ok) {
            qCritical() << "Invalid weight on line" << lineNum << ":" << parts[0];
            return 1;
        }
        entries.append(qMakePair(weight, parts[1]));
    }
    descFile.close();

    if (entries.isEmpty()) {
        qCritical() << "Description file contains no entries.";
        return 1;
    }

    //--- Read STCs and compute weighted average ---
    InvSourceEstimate result;
    double weightSum = 0.0;

    for (int i = 0; i < entries.size(); ++i) {
        double w = entries[i].first;
        const QString& path = entries[i].second;

        QFile stcFile(path);
        if (!stcFile.open(QIODevice::ReadOnly)) {
            qCritical() << "Cannot open STC file:" << path;
            return 1;
        }

        InvSourceEstimate stc;
        if (!InvSourceEstimate::read(stcFile, stc)) {
            qCritical() << "Failed to read STC file:" << path;
            return 1;
        }
        stcFile.close();

        if (i == 0) {
            result = stc;
            result.data = w * stc.data;
        } else {
            if (stc.data.rows() != result.data.rows() || stc.data.cols() != result.data.cols()) {
                qCritical() << "STC dimension mismatch in file:" << path;
                return 1;
            }
            result.data += w * stc.data;
        }
        weightSum += w;
    }

    if (qFuzzyIsNull(weightSum)) {
        qCritical() << "Total weight is zero — cannot normalize.";
        return 1;
    }

    result.data /= weightSum;

    //--- Write output ---
    QFile outFile(parser.value(outOpt));
    if (!outFile.open(QIODevice::WriteOnly)) {
        qCritical() << "Cannot open output file:" << parser.value(outOpt);
        return 1;
    }

    if (!result.write(outFile)) {
        qCritical() << "Failed to write output STC.";
        return 1;
    }
    outFile.close();

    qInfo() << "Averaged" << entries.size() << "STC files -> " << parser.value(outOpt);
    return 0;
}
