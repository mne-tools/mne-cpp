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
 * @brief    Process STC files: scale, export to ASCII, copy.
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
    QCoreApplication::setApplicationName("mne_process_stc");
    QCoreApplication::setApplicationVersion(PROGRAM_VERSION);

    QCommandLineParser parser;
    parser.setApplicationDescription("Process STC files: scale, export to ASCII, copy.");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption inOpt("in", "Input STC file.", "file");
    parser.addOption(inOpt);

    QCommandLineOption outOpt("out", "Output binary STC file.", "file");
    parser.addOption(outOpt);

    QCommandLineOption outAscOpt("outasc", "Output ASCII file.", "file");
    parser.addOption(outAscOpt);

    QCommandLineOption scaleByOpt("scaleby", "Multiply values by this factor.", "factor");
    parser.addOption(scaleByOpt);

    QCommandLineOption scaleToOpt("scaleto", "Scale data so that the maximum equals this value.", "value");
    parser.addOption(scaleToOpt);

    parser.process(app);

    if (!parser.isSet(inOpt)) {
        qCritical() << "--in is required.";
        parser.showHelp(1);
    }

    if (!parser.isSet(outOpt) && !parser.isSet(outAscOpt)) {
        qCritical() << "At least one of --out or --outasc is required.";
        parser.showHelp(1);
    }

    //--- Read input STC ---
    QFile inFile(parser.value(inOpt));
    if (!inFile.open(QIODevice::ReadOnly)) {
        qCritical() << "Cannot open input file:" << parser.value(inOpt);
        return 1;
    }

    InvSourceEstimate stc;
    if (!InvSourceEstimate::read(inFile, stc)) {
        qCritical() << "Failed to read STC file:" << parser.value(inOpt);
        return 1;
    }
    inFile.close();

    //--- Apply scaling ---
    if (parser.isSet(scaleByOpt)) {
        bool ok;
        double factor = parser.value(scaleByOpt).toDouble(&ok);
        if (!ok) {
            qCritical() << "Invalid --scaleby value:" << parser.value(scaleByOpt);
            return 1;
        }
        stc.data *= factor;
        qInfo() << "Scaled data by factor" << factor;
    }

    if (parser.isSet(scaleToOpt)) {
        bool ok;
        double target = parser.value(scaleToOpt).toDouble(&ok);
        if (!ok) {
            qCritical() << "Invalid --scaleto value:" << parser.value(scaleToOpt);
            return 1;
        }
        double maxVal = stc.data.array().abs().maxCoeff();
        if (qFuzzyIsNull(maxVal)) {
            qWarning() << "Data maximum is zero — cannot scale to target.";
        } else {
            stc.data *= (target / maxVal);
            qInfo() << "Scaled data so max =" << target;
        }
    }

    //--- Write binary STC ---
    if (parser.isSet(outOpt)) {
        QFile outFile(parser.value(outOpt));
        if (!outFile.open(QIODevice::WriteOnly)) {
            qCritical() << "Cannot open output file:" << parser.value(outOpt);
            return 1;
        }
        if (!stc.write(outFile)) {
            qCritical() << "Failed to write output STC.";
            return 1;
        }
        outFile.close();
        qInfo() << "Wrote binary STC:" << parser.value(outOpt);
    }

    //--- Write ASCII ---
    if (parser.isSet(outAscOpt)) {
        QFile ascFile(parser.value(outAscOpt));
        if (!ascFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            qCritical() << "Cannot open ASCII output file:" << parser.value(outAscOpt);
            return 1;
        }
        QTextStream out(&ascFile);
        for (int v = 0; v < stc.data.rows(); ++v) {
            for (int t = 0; t < stc.data.cols(); ++t) {
                out << stc.vertices(v) << " " << t << " " << stc.data(v, t) << "\n";
            }
        }
        ascFile.close();
        qInfo() << "Wrote ASCII file:" << parser.value(outAscOpt);
    }

    return 0;
}
