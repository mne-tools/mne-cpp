//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
 * @brief    Compute a weighted average of STC source estimate files.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <inv/inv_source_estimate.h>
#include <base/log.h>
#include <base/log_qt.h>

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

using namespace mne;
using namespace Eigen;

//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

#define PROGRAM_VERSION MNE_CPP_VERSION

//=============================================================================================================

int main(int argc, char* argv[])
{
    mne::install_qt_message_handler();
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

        auto read = InvSourceEstimate::read(path.toStdU16String());
        if (!read) {
            qCritical() << read.error().what();
            return 1;
        }
        const InvSourceEstimate& stc = *read;

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
    if (!result.write(parser.value(outOpt).toStdU16String())) {
        qCritical() << "Failed to write output STC.";
        return 1;
    }

    qInfo() << "Averaged" << entries.size() << "STC files -> " << parser.value(outOpt);
    return 0;
}
