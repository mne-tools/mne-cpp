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
 * @brief    Create a uniform STC where every vertex has the same value.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <inv/inv_source_estimate.h>
#include <mne/mne_source_spaces.h>
#include <fiff/fiff_stream.h>
#include <utils/generics/mne_logger.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QFile>
#include <QDebug>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace INVLIB;
using namespace MNELIB;
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
    QCoreApplication::setApplicationName("mne_make_uniform_stc");
    QCoreApplication::setApplicationVersion(PROGRAM_VERSION);

    QCommandLineParser parser;
    parser.setApplicationDescription("Create a uniform STC where every vertex has the same value.");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption srcOpt("src", "Source space FIFF file.", "file");
    parser.addOption(srcOpt);

    QCommandLineOption valOpt("val", "Uniform value for all vertices (default: 1.0).", "value", "1.0");
    parser.addOption(valOpt);

    QCommandLineOption outOpt("out", "Output STC file path.", "file");
    parser.addOption(outOpt);

    parser.process(app);

    if (!parser.isSet(srcOpt) || !parser.isSet(outOpt)) {
        qCritical() << "Both --src and --out are required.";
        parser.showHelp(1);
    }

    bool ok;
    double uniformVal = parser.value(valOpt).toDouble(&ok);
    if (!ok) {
        qCritical() << "Invalid --val value:" << parser.value(valOpt);
        return 1;
    }

    //--- Read source space ---
    QFile srcFile(parser.value(srcOpt));
    if (!srcFile.open(QIODevice::ReadOnly)) {
        qCritical() << "Cannot open source space file:" << parser.value(srcOpt);
        return 1;
    }

    FiffStream::SPtr pStream(new FiffStream(&srcFile));
    if (!pStream->open()) {
        qCritical() << "Failed to open FIFF stream from:" << parser.value(srcOpt);
        return 1;
    }

    MNESourceSpaces srcSpaces;
    if (!MNESourceSpaces::readFromStream(pStream, true, srcSpaces)) {
        qCritical() << "Failed to read source spaces from:" << parser.value(srcOpt);
        return 1;
    }
    pStream->close();

    //--- Collect in-use vertices from all hemispheres ---
    QList<VectorXi> vertnoList = srcSpaces.get_vertno();
    int totalVerts = 0;
    for (const auto& v : vertnoList)
        totalVerts += v.size();

    if (totalVerts == 0) {
        qCritical() << "No in-use vertices found in source space.";
        return 1;
    }

    // Concatenate vertex numbers
    VectorXi allVertices(totalVerts);
    int offset = 0;
    for (const auto& v : vertnoList) {
        allVertices.segment(offset, v.size()) = v;
        offset += v.size();
    }

    //--- Build uniform source estimate (single time point) ---
    MatrixXd data = MatrixXd::Constant(totalVerts, 1, uniformVal);
    float tmin = 0.0f;
    float tstep = 1.0f;

    InvSourceEstimate stc(data, allVertices, tmin, tstep);

    //--- Write output ---
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

    qInfo() << "Created uniform STC with" << totalVerts << "vertices, value ="
            << uniformVal << "->" << parser.value(outOpt);
    return 0;
}
