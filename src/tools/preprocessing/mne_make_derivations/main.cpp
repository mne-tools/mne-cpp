//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
 * @brief    Parse text channel derivation file and write as FIFF named matrix.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fiff/fiff_stream.h>
#include <fiff/fiff_named_matrix.h>
#include <fiff/fiff_types.h>
#include <dsp/channel_derivation.h>
#include <utils/generics/mne_logger.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QFile>
#include <QDebug>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;
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
    QCoreApplication::setApplicationName("mne_make_derivations");
    QCoreApplication::setApplicationVersion(PROGRAM_VERSION);

    QCommandLineParser parser;
    parser.setApplicationDescription("Parse text channel derivation file and write as FIFF.\n\n"
                                     "Text format (one derivation per line):\n"
                                     "  derived_name = coeff1 * ch_name1 + coeff2 * ch_name2 + ...");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption inOpt("in", "Input text derivation file.", "file");
    parser.addOption(inOpt);

    QCommandLineOption outOpt("out", "Output FIFF file.", "file");
    parser.addOption(outOpt);

    parser.process(app);

    QString inFile = parser.value(inOpt);
    QString outFile = parser.value(outOpt);

    if (inFile.isEmpty()) { qCritical("--in is required."); return 1; }
    if (outFile.isEmpty()) { qCritical("--out is required."); return 1; }

    // Read derivation rules using ChannelDerivation utility
    QVector<DerivationRule> rules = ChannelDerivation::readDefinitionFile(inFile);
    if (rules.isEmpty()) {
        qCritical("No derivation rules read from: %s", qPrintable(inFile));
        return 1;
    }
    qInfo("Read %lld derivation rule(s) from %s" ,
           static_cast<long long>(rules.size()), qPrintable(inFile));

    // Collect all unique input channel names (columns)
    QStringList colNames;
    for (const DerivationRule &rule : rules) {
        for (auto it = rule.inputWeights.constBegin(); it != rule.inputWeights.constEnd(); ++it) {
            if (!colNames.contains(it.key()))
                colNames.append(it.key());
        }
    }
    colNames.sort();

    // Build row names (derived channel names)
    QStringList rowNames;
    for (const DerivationRule &rule : rules) {
        rowNames.append(rule.outputName);
    }

    // Build the derivation matrix (nRules x nInputChannels)
    MatrixXd mat = MatrixXd::Zero(rules.size(), colNames.size());
    for (int r = 0; r < rules.size(); ++r) {
        for (auto it = rules[r].inputWeights.constBegin(); it != rules[r].inputWeights.constEnd(); ++it) {
            int col = colNames.indexOf(it.key());
            if (col >= 0)
                mat(r, col) = it.value();
        }
    }

    qInfo("Derivation matrix: %lld x %lld" ,
           static_cast<long long>(mat.rows()), static_cast<long long>(mat.cols()));

    // Create FiffNamedMatrix
    FiffNamedMatrix namedMat(mat.rows(), mat.cols(), rowNames, colNames, mat);

    // Write as FIFF
    QFile fOut(outFile);
    FiffStream::SPtr stream = FiffStream::start_file(fOut);
    if (!stream) {
        qCritical("Cannot open output file: %s", qPrintable(outFile));
        return 1;
    }

    stream->start_block(FIFFB_MNE);
    stream->write_named_matrix(FIFF_MNE_CH_NAME_LIST, namedMat);
    stream->end_block(FIFFB_MNE);
    stream->end_file();

    qInfo("Written derivation matrix to: %s" , qPrintable(outFile));
    return 0;
}
