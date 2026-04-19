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
    printf("Read %lld derivation rule(s) from %s\n",
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

    printf("Derivation matrix: %lld x %lld\n",
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

    printf("Written derivation matrix to: %s\n", qPrintable(outFile));
    return 0;
}
