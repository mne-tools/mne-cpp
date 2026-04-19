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
 * @brief    CLI tool for dipole fitting using InvDipoleFit.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <inv/dipole_fit/inv_dipole_fit_settings.h>
#include <inv/dipole_fit/inv_dipole_fit.h>
#include <inv/dipole_fit/inv_ecd_set.h>

#include <utils/generics/mne_logger.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QFile>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace INVLIB;
using namespace UTILSLIB;

//=============================================================================================================
// DEFINES
//=============================================================================================================

#define PROGRAM_VERSION MNE_CPP_VERSION

//=============================================================================================================
// MAIN
//=============================================================================================================

int main(int argc, char *argv[])
{
    qInstallMessageHandler(MNELogger::customLogWriter);
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("mne_dipole_fit");
    QCoreApplication::setApplicationVersion(PROGRAM_VERSION);

    //
    // CLI argument parsing
    //
    QCommandLineParser parser;
    parser.setApplicationDescription("Dipole fitting CLI tool");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption measOpt("meas", "Measurement data file (evoked FIFF).", "file");
    QCommandLineOption noiseOpt("noise", "Noise covariance file.", "file");
    QCommandLineOption bemOpt("bem", "BEM model file.", "file");
    QCommandLineOption mriOpt("mri", "MRI-head coordinate transform file.", "file");
    QCommandLineOption tminOpt("tmin", "Start time for fitting (seconds).", "secs");
    QCommandLineOption tmaxOpt("tmax", "End time for fitting (seconds).", "secs");
    QCommandLineOption bminOpt("bmin", "Baseline start (seconds).", "secs");
    QCommandLineOption bmaxOpt("bmax", "Baseline end (seconds).", "secs");
    QCommandLineOption dipOpt("dip", "Output dipole file path.", "file");

    parser.addOption(measOpt);
    parser.addOption(noiseOpt);
    parser.addOption(bemOpt);
    parser.addOption(mriOpt);
    parser.addOption(tminOpt);
    parser.addOption(tmaxOpt);
    parser.addOption(bminOpt);
    parser.addOption(bmaxOpt);
    parser.addOption(dipOpt);

    parser.process(app);

    //
    // Validate required arguments
    //
    if (!parser.isSet(measOpt)) {
        qCritical("Error: --meas is required.");
        return 1;
    }

    //
    // Populate dipole fit settings
    //
    InvDipoleFitSettings settings;

    settings.measname = parser.value(measOpt);

    if (parser.isSet(noiseOpt))
        settings.noisename = parser.value(noiseOpt);

    if (parser.isSet(bemOpt))
        settings.bemname = parser.value(bemOpt);

    if (parser.isSet(mriOpt))
        settings.mriname = parser.value(mriOpt);

    if (parser.isSet(tminOpt))
        settings.tmin = parser.value(tminOpt).toFloat();

    if (parser.isSet(tmaxOpt))
        settings.tmax = parser.value(tmaxOpt).toFloat();

    if (parser.isSet(bminOpt)) {
        settings.bmin = parser.value(bminOpt).toFloat();
        settings.do_baseline = parser.isSet(bmaxOpt);
    }

    if (parser.isSet(bmaxOpt)) {
        settings.bmax = parser.value(bmaxOpt).toFloat();
        settings.do_baseline = parser.isSet(bminOpt);
    }

    if (parser.isSet(dipOpt))
        settings.dipname = parser.value(dipOpt);

    settings.gui = false;

    //
    // Check settings integrity
    //
    settings.checkIntegrity();

    //
    // Run dipole fit
    //
    fprintf(stderr, "Starting dipole fit...\n");

    InvDipoleFit dipFit(&settings);
    InvEcdSet set = dipFit.calculateFit();

    if (set.size() == 0) {
        qCritical("Dipole fitting failed — no dipoles were fitted.");
        return 1;
    }

    fprintf(stderr, "Fitted %d dipole(s).\n", set.size());

    //
    // Save results
    //
    if (!settings.dipname.isEmpty()) {
        if (!set.save_dipoles_dip(settings.dipname)) {
            qCritical("Could not save dipoles to %s.", settings.dipname.toUtf8().constData());
            return 1;
        }
        fprintf(stderr, "Dipoles saved to %s\n", settings.dipname.toUtf8().constData());
    }

    if (!settings.bdipname.isEmpty()) {
        if (!set.save_dipoles_bdip(settings.bdipname)) {
            qCritical("Could not save dipoles to %s.", settings.bdipname.toUtf8().constData());
            return 1;
        }
        fprintf(stderr, "Dipoles saved to %s\n", settings.bdipname.toUtf8().constData());
    }

    fprintf(stderr, "Finished.\n");
    return 0;
}
