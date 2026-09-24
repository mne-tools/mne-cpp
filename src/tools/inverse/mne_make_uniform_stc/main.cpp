//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
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
    if (!stc.write(outFile)) {
        qCritical() << "Failed to write output STC.";
        return 1;
    }

    qInfo() << "Created uniform STC with" << totalVerts << "vertices, value ="
            << uniformVal << "->" << parser.value(outOpt);
    return 0;
}
