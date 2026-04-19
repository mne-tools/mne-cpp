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
 * @brief    List all available coil definitions from a coil definition file.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fwd/fwd_coil_set.h>
#include <fwd/fwd_coil.h>

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

using namespace FWDLIB;
using namespace UTILSLIB;

//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

#define PROGRAM_VERSION MNE_CPP_VERSION

//=============================================================================================================

static QString coilClassName(int coilClass)
{
    switch (coilClass) {
    case FWD_COILC_MAG:         return "MAG";
    case FWD_COILC_AXIAL_GRAD:  return "AXIAL_GRAD";
    case FWD_COILC_PLANAR_GRAD: return "PLANAR_GRAD";
    case FWD_COILC_AXIAL_GRAD2: return "AXIAL_GRAD2";
    case FWD_COILC_EEG:         return "EEG";
    default:                    return "UNKNOWN";
    }
}

//=============================================================================================================

static QString accuracyName(int acc)
{
    switch (acc) {
    case FWD_COIL_ACCURACY_POINT:    return "point";
    case FWD_COIL_ACCURACY_NORMAL:   return "normal";
    case FWD_COIL_ACCURACY_ACCURATE: return "accurate";
    default:                         return "unknown";
    }
}

//=============================================================================================================

int main(int argc, char *argv[])
{
    qInstallMessageHandler(MNELogger::customLogWriter);
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("mne_list_coil_def");
    QCoreApplication::setApplicationVersion(PROGRAM_VERSION);

    QCommandLineParser parser;
    parser.setApplicationDescription("List all available coil definitions.\n\nReads a coil definition file and prints a table of coil types.");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption coildefOpt("coildef", "Custom coil definition file (optional).", "name");
    parser.addOption(coildefOpt);

    parser.process(app);

    QString coildefName = parser.value(coildefOpt);

    // If no custom file, use the default coil_def.dat location
    if (coildefName.isEmpty()) {
        // Try standard locations
        QStringList searchPaths;
        searchPaths << QCoreApplication::applicationDirPath() + "/../resources/coil_def.dat"
                    << QCoreApplication::applicationDirPath() + "/resources/coil_def.dat"
                    << QCoreApplication::applicationDirPath() + "/coil_def.dat";

        for (const QString& path : searchPaths) {
            if (QFile::exists(path)) {
                coildefName = path;
                break;
            }
        }

        if (coildefName.isEmpty()) {
            qCritical("No coil definition file found. Use --coildef to specify one.");
            return 1;
        }
    }

    fprintf(stderr, "Reading coil definitions from: %s\n", qPrintable(coildefName));

    FwdCoilSet::UPtr coilSet = FwdCoilSet::read_coil_defs(coildefName);
    if (!coilSet) {
        qCritical("Cannot read coil definitions from: %s", qPrintable(coildefName));
        return 1;
    }

    int ncoils = coilSet->ncoil();
    fprintf(stderr, "Read %d coil definitions.\n\n", ncoils);

    // Print header
    fprintf(stdout, "%-8s %-14s %-10s %-6s %-10s %s\n",
            "Type", "Class", "Accuracy", "NP", "Size(m)", "Description");
    fprintf(stdout, "%-8s %-14s %-10s %-6s %-10s %s\n",
            "--------", "--------------", "----------", "------", "----------", "--------------------");

    for (int k = 0; k < ncoils; ++k) {
        const FwdCoil& coil = *(coilSet->coils[k]);
        fprintf(stdout, "%-8d %-14s %-10s %-6d %-10.4f %s\n",
                coil.type,
                qPrintable(coilClassName(coil.coil_class)),
                qPrintable(accuracyName(coil.accuracy)),
                coil.np,
                coil.size,
                qPrintable(coil.desc));
    }

    return 0;
}
