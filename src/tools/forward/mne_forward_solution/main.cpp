//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2017-2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     February, 2017
 * @brief    Implements the mne_forward_solution application.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fwd/compute_fwd/compute_fwd_settings.h>
#include <fwd/compute_fwd/compute_fwd.h>
#include <mne/mne_forward_solution.h>

#include <memory>

#include <utils/generics/mne_logger.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QApplication>
#include <QCommandLineParser>
#include <QFile>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FWDLIB;

//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

#define PROGRAM_VERSION MNE_CPP_VERSION

//=============================================================================================================
// MAIN
//=============================================================================================================

//=============================================================================================================
/**
 * The function main marks the entry point of the mne_forward_solution application.
 * By default, main has the storage class extern.
 *
 * @param[in] argc  (argument count) is an integer that indicates how many arguments were entered on the command line when the program was started.
 * @param[in] argv  (argument vector) is an array of pointers to arrays of character objects. The array objects are null-terminated strings, representing the arguments that were entered on the command line when the program was started.
 * @return the value that was set to exit() (which is 0 if exit() is called via quit()).
 */
int main(int argc, char *argv[])
{
    qInstallMessageHandler(UTILSLIB::MNELogger::customLogWriter);
    QApplication app(argc, argv);
    QCoreApplication::setApplicationName("mne_forward_solution");
    QCoreApplication::setApplicationVersion(PROGRAM_VERSION);

    //=========================================================================================================
    // Command line parser
    //=========================================================================================================

    QCommandLineParser parser;
    parser.setApplicationDescription(
        "Compute the MEG/EEG forward solution.\n"
        "Port of the original MNE-C mne_forward_solution by Matti Hamalainen."
    );
    parser.addHelpOption();
    parser.addVersionOption();

    // Source & sensor options
    QCommandLineOption srcOpt("src", "Source space file.", "file");
    QCommandLineOption measOpt("meas", "MEG/EEG measurement file (sensor & electrode locations).", "file");
    QCommandLineOption fwdOpt("fwd", "Output forward solution file.", "file");

    // Transform options
    QCommandLineOption mriOpt("mri", "MRI description file for head/MRI transform.", "file");
    QCommandLineOption transOpt("trans", "Text file for head/MRI transform.", "file");
    QCommandLineOption notransOpt("notrans", "Head and MRI coordinate systems are identical.");

    // BEM & sphere model options
    QCommandLineOption bemOpt("bem", "BEM model file.", "file");
    QCommandLineOption originOpt("origin", "Sphere model origin in head coordinates (x:y:z in mm).", "x:y:z");
    QCommandLineOption eegscalpOpt("eegscalp", "Scale electrode locations to the scalp surface (sphere model).");
    QCommandLineOption eegmodelsOpt("eegmodels", "File of EEG sphere model specifications.", "file");
    QCommandLineOption eegmodelOpt("eegmodel", "Name of the EEG sphere model to use (default: Default).", "name");
    QCommandLineOption eegradOpt("eegrad", "Scalp radius for EEG sphere model in mm (default: 90.0).", "radius");

    // Modality flags
    QCommandLineOption megOpt("meg", "Compute the MEG forward solution.");
    QCommandLineOption eegOpt("eeg", "Compute the EEG forward solution.");
    QCommandLineOption gradOpt("grad", "Compute the gradient of the field with respect to dipole coordinates.");
    QCommandLineOption fixedOpt("fixed", "Calculate only for the source orientation given by surface normals.");
    QCommandLineOption accurateOpt("accurate", "Use accurate coil definitions in MEG forward computation.");
    QCommandLineOption mricoordOpt("mricoord", "Do calculations in MRI coordinates instead of head coordinates.");
    QCommandLineOption allOpt("all", "Calculate forward solution in all nodes instead of selected ones only.");

    // Source space filtering
    QCommandLineOption labelOpt("label", "FsLabel file to select sources (can be specified multiple times).", "file");
    QCommandLineOption mindistOpt("mindist", "Minimum distance of sources from the inner skull surface (mm).", "dist");
    QCommandLineOption mindistoutOpt("mindistout", "Output file for omitted source space points.", "file");
    QCommandLineOption includeallOpt("includeall", "Omit all source space checks.");

    parser.addOptions({srcOpt, measOpt, fwdOpt,
                       mriOpt, transOpt, notransOpt,
                       bemOpt, originOpt, eegscalpOpt, eegmodelsOpt, eegmodelOpt, eegradOpt,
                       megOpt, eegOpt, gradOpt, fixedOpt, accurateOpt, mricoordOpt, allOpt,
                       labelOpt, mindistOpt, mindistoutOpt, includeallOpt});

    parser.process(app);

    //=========================================================================================================
    // Populate settings from parsed arguments
    //=========================================================================================================

    std::shared_ptr<ComputeFwdSettings> settings = std::make_shared<ComputeFwdSettings>();

    // Build the command string for FIFF stamping
    settings->command = QCoreApplication::arguments().join(" ");

    // Source & sensor files
    if (parser.isSet(srcOpt))
        settings->srcname = parser.value(srcOpt);
    if (parser.isSet(measOpt))
        settings->measname = parser.value(measOpt);
    if (parser.isSet(fwdOpt))
        settings->solname = parser.value(fwdOpt);

    // Transform options (--mri and --trans are mutually exclusive with --notrans)
    if (parser.isSet(notransOpt)) {
        settings->mri_head_ident = true;
        settings->mriname.clear();
        settings->transname.clear();
    } else if (parser.isSet(mriOpt)) {
        settings->mri_head_ident = false;
        settings->mriname = parser.value(mriOpt);
        settings->transname.clear();
    } else if (parser.isSet(transOpt)) {
        settings->mri_head_ident = false;
        settings->transname = parser.value(transOpt);
        settings->mriname.clear();
    }

    // BEM model
    if (parser.isSet(bemOpt))
        settings->bemname = parser.value(bemOpt);

    // Sphere model origin (x:y:z in mm, converted to meters)
    if (parser.isSet(originOpt)) {
        QStringList parts = parser.value(originOpt).split(':');
        if (parts.size() != 3) {
            qCritical("Could not interpret the origin.");
            return 1;
        }
        bool ok1, ok2, ok3;
        settings->r0[0] = parts[0].toFloat(&ok1) / 1000.0f;
        settings->r0[1] = parts[1].toFloat(&ok2) / 1000.0f;
        settings->r0[2] = parts[2].toFloat(&ok3) / 1000.0f;
        if (!ok1 || !ok2 || !ok3) {
            qCritical("Could not interpret the origin.");
            return 1;
        }
    }

    // EEG sphere model options
    if (parser.isSet(eegscalpOpt))
        settings->scale_eeg_pos = true;
    if (parser.isSet(eegmodelsOpt))
        settings->eeg_model_file = parser.value(eegmodelsOpt);
    if (parser.isSet(eegmodelOpt))
        settings->eeg_model_name = parser.value(eegmodelOpt);
    if (parser.isSet(eegradOpt)) {
        bool ok;
        float rad = parser.value(eegradOpt).toFloat(&ok);
        if (!ok || rad <= 0) {
            qCritical("Radius must be a positive number.");
            return 1;
        }
        settings->eeg_sphere_rad = rad / 1000.0f;
    }

    // Modality flags
    if (parser.isSet(megOpt))
        settings->include_meg = true;
    if (parser.isSet(eegOpt))
        settings->include_eeg = true;
    if (parser.isSet(gradOpt))
        settings->compute_grad = true;
    if (parser.isSet(fixedOpt))
        settings->fixed_ori = true;
    if (parser.isSet(accurateOpt))
        settings->accurate = true;
    if (parser.isSet(mricoordOpt))
        settings->coord_frame = FIFFV_COORD_MRI;
    if (parser.isSet(allOpt))
        settings->do_all = true;

    // Source space filtering
    if (parser.isSet(labelOpt)) {
        settings->labels = parser.values(labelOpt);
        settings->nlabel = settings->labels.size();
    }
    if (parser.isSet(mindistOpt)) {
        bool ok;
        float dist = parser.value(mindistOpt).toFloat(&ok);
        if (!ok) {
            qCritical("Could not interpret the distance.");
            return 1;
        }
        settings->mindist = (dist <= 0.0f) ? 0.0f : dist / 1000.0f;
    }
    if (parser.isSet(mindistoutOpt))
        settings->mindistoutname = parser.value(mindistoutOpt);
    if (parser.isSet(includeallOpt))
        settings->filter_spaces = false;

    //=========================================================================================================
    // Run forward computation
    //=========================================================================================================

    settings->checkIntegrity();

    ComputeFwd computer(settings);
    auto fwdSolution = computer.calculateFwd();

    QFile fwdFile(settings->solname);
    fwdSolution->write(fwdFile);

    return app.exec();
}
