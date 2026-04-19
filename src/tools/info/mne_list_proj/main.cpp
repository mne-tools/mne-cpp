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
 * @brief    List SSP projectors from a FIFF measurement file.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fiff/fiff_stream.h>
#include <fiff/fiff_info.h>
#include <fiff/fiff_raw_data.h>
#include <fiff/fiff_proj.h>
#include <fiff/fiff_named_matrix.h>
#include <fiff/fiff_constants.h>

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
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;
using namespace UTILSLIB;

//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

#define PROGRAM_VERSION MNE_CPP_VERSION

//=============================================================================================================

static QString projKindName(int kind)
{
    switch (kind) {
    case 1:                                return "Field";
    case 2:                                return "Fixed dipole";
    case 3:                                return "Rotating dipole";
    case 4:                                return "Homog. grad.";
    case 5:                                return "Homog. field";
    case FIFFV_MNE_PROJ_ITEM_EEG_AVREF:   return "EEG avg ref";
    default:                               return QString("Unknown (%1)").arg(kind);
    }
}

//=============================================================================================================

int main(int argc, char *argv[])
{
    qInstallMessageHandler(MNELogger::customLogWriter);
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("mne_list_proj");
    QCoreApplication::setApplicationVersion(PROGRAM_VERSION);

    QCommandLineParser parser;
    parser.setApplicationDescription("List SSP projectors from a FIFF file.\n\nReads measurement info and prints projector details.");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption measOpt("meas", "Measurement FIFF file.", "name");
    parser.addOption(measOpt);

    parser.process(app);

    QString measName = parser.value(measOpt);

    if (measName.isEmpty()) {
        qCritical("The --meas option is required.");
        parser.showHelp(1);
    }

    // Read measurement info
    QFile measFile(measName);
    FiffRawData raw(measFile);

    if (raw.info.chs.isEmpty()) {
        qCritical("No channel information found in %s", qPrintable(measName));
        return 1;
    }

    fprintf(stderr, "Opened %s : %d channels\n", qPrintable(measName), raw.info.nchan);

    const QList<FiffProj>& projs = raw.info.projs;

    if (projs.isEmpty()) {
        fprintf(stdout, "No SSP projectors found.\n");
        return 0;
    }

    fprintf(stdout, "Found %d SSP projector(s):\n\n", projs.size());

    // Print header
    fprintf(stdout, "%-6s %-20s %-8s %-8s %s\n",
            "Index", "Kind", "NVec", "Active", "Description");
    fprintf(stdout, "%-6s %-20s %-8s %-8s %s\n",
            "------", "--------------------", "--------", "--------", "--------------------");

    for (int k = 0; k < projs.size(); ++k) {
        const FiffProj& proj = projs[k];

        int nVec = 0;
        if (proj.data) {
            nVec = proj.data->nrow;
        }

        fprintf(stdout, "%-6d %-20s %-8d %-8s %s\n",
                k + 1,
                qPrintable(projKindName(proj.kind)),
                nVec,
                proj.active ? "Yes" : "No",
                qPrintable(proj.desc));
    }

    return 0;
}
