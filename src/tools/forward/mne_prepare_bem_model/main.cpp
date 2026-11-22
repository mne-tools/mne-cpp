//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March, 2026
 * @brief    Prepare a BEM model for forward computation (compute BEM solution matrix).
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fwd/fwd_bem_model.h>
#include <mne/mne_surface.h>
#include <fiff/fiff_stream.h>
#include <fiff/fiff_constants.h>
#include <base/log.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <memory>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

#define PROGRAM_VERSION MNE_CPP_VERSION

//=============================================================================================================

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("mne_prepare_bem_model");
    QCoreApplication::setApplicationVersion(PROGRAM_VERSION);

    QCommandLineParser parser;
    parser.setApplicationDescription("Compute the BEM solution matrix for forward modeling.");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption bemOpt("bem", "Input BEM model FIFF file.", "file");
    parser.addOption(bemOpt);

    QCommandLineOption solOpt("sol", "Output solution FIFF file (default: derived from --bem).", "file");
    parser.addOption(solOpt);

    QCommandLineOption methodOpt("method", "BEM method: linear or constant.", "name", "linear");
    parser.addOption(methodOpt);

    parser.process(app);

    QString bemFile = parser.value(bemOpt);
    QString solFile = parser.value(solOpt);
    QString methodStr = parser.value(methodOpt).toLower();

    if (bemFile.isEmpty()) {
        Log::critical("--bem is required.");
        parser.showHelp(1);
    }

    int bemMethod;
    if (methodStr == "linear")
        bemMethod = FWD_BEM_LINEAR_COLL;
    else if (methodStr == "constant")
        bemMethod = FWD_BEM_CONSTANT_COLL;
    else {
        Log::critical("Unknown method: {} (use 'linear' or 'constant')", methodStr.toStdString());
        return 1;
    }

    // Derive solution filename  if not specified
    if (solFile.isEmpty())
        solFile = QString::fromStdU16String(FwdBemModel::fwd_bem_make_bem_sol_name(bemFile.toStdU16String()).u16string());

    Log::info("BEM model file:    {}", bemFile.toStdString());
    Log::info("BEM solution file: {}", solFile.toStdString());
    Log::info("BEM method:        {}", FwdBemModel::fwd_bem_explain_method(bemMethod));

    // Determine how many layers we have
    // Try three-layer first, fall back to single-layer
    std::unique_ptr<FwdBemModel> bemModel;
    bemModel = FwdBemModel::fwd_bem_load_three_layer_surfaces(bemFile.toStdU16String());
    if (!bemModel) {
        Log::info("Not a three-layer model, trying single-layer (homogeneous)...");
        bemModel = FwdBemModel::fwd_bem_load_homog_surface(bemFile.toStdU16String());
    }

    if (!bemModel) {
        Log::critical("Cannot load BEM surfaces from: {}", bemFile.toStdString());
        return 1;
    }

    Log::info("Loaded {} BEM surface(s)", bemModel->nsurf);
    for (int k = 0; k < bemModel->nsurf; ++k) {
        Log::info("  Surface {}: {}  ({} vertices, {} triangles)", k + 1, FwdBemModel::fwd_bem_explain_surface(bemModel->surfs[k]->id), bemModel->surfs[k]->np, bemModel->surfs[k]->ntri);
    }

    // Compute the BEM solution
    Log::info("\nComputing BEM solution...");
    if (bemModel->fwd_bem_compute_solution(bemMethod) != 0) {
        Log::critical("BEM solution computation failed.");
        return 1;
    }
    Log::info("BEM solution computed successfully.");

    // Save the solution
    Log::info("Writing solution to: {}", solFile.toStdString());

    if (bemModel->fwd_bem_save_model(solFile.toStdU16String()) != 0) {
        Log::critical("Cannot write output file: {}", solFile.toStdString());
        return 1;
    }

    Log::info("BEM solution saved.");
    return 0;
}
