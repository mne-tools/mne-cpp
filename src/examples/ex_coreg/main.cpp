//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2020-2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Ruben Dörfel <doerfelruben@aol.com>
 * @since    0.1.5
 * @date     July, 2020
 * @brief    Coregistration workflow: fit digitizer points to the head surface with ICP
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff/fiff_dig_point_set.h"
#include "fiff/fiff_dig_point.h"
#include "fiff/fiff_coord_trans.h"
#include "mne/mne_bem.h"
#include "mne/mne_bem_surface.h"
#include "mne/mne_project_to_surface.h"
#include "mne/mne_icp.h"
#include <disp3D/view/brainview.h>
#include <disp3D/model/braintreemodel.h>
#include <disp3D/model/items/digitizersettreeitem.h>
#include <disp3D/model/items/bemtreeitem.h>
#include <base/log.h>
#include <base/string_utils.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QApplication>
#include <QMainWindow>
#include <QCommandLineParser>
#include <QFile>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <iostream>
#include <memory>

//=============================================================================================================
// Eigen
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace mne;
using namespace mne::disp;

//=============================================================================================================
// MAIN
//=============================================================================================================

//=============================================================================================================
/**
 * The function main marks the entry point of the program.
 * By default, main has the storage class extern.
 *
 * @param[in] argc (argument count) is an integer that indicates how many arguments were entered on the command line when the program was started.
 * @param[in] argv (argument vector) is an array of pointers to arrays of character objects. The array objects are null-terminated strings, representing the arguments that were entered on the command line when the program was started.
 * @return the value that was set to exit() (which is 0 if exit() is called via quit()).
 */
int main(int argc, char* argv[])
{
#ifdef STATICBUILD
// Q_INIT_RESOURCE(mne_disp3d);
#endif

    QApplication a(argc, argv);

    // Command Line Parser
    QCommandLineParser parser;
    parser.setApplicationDescription("Example Coregistration");
    parser.addHelpOption();

    QCommandLineOption digOption("dig", "The destination point set", "file", QCoreApplication::applicationDirPath() + "/../resources/data/MNE-sample-data/MEG/sample/sample_audvis-ave.fif");
    QCommandLineOption bemOption("bem", "The bem file", "file", QCoreApplication::applicationDirPath() + "/../resources/data/MNE-sample-data/subjects/sample/bem/sample-head.fif");
    QCommandLineOption transOption("trans", "The MRI-Head transformation file", "file", QCoreApplication::applicationDirPath() + "/../resources/data/MNE-sample-data/MEG/sample/sample_audvis_raw-trans.fif");
    QCommandLineOption scaleOption("scale", "Weather to scale during the registration or not", "bool", "false");
    QCommandLineOption tolOption("tol", "The convergence limit for the icp algorithm.", "float", "0.001");
    QCommandLineOption distOption("dist", "The maximum distance between digitizer and head shape in mm.", "float", "0.02");
    QCommandLineOption iterOption("iter", "The maximum number of icp iterations.", "int", "20");

    parser.addOption(digOption);
    parser.addOption(bemOption);
    parser.addOption(scaleOption);
    parser.addOption(transOption);
    parser.addOption(tolOption);
    parser.addOption(distOption);
    parser.addOption(iterOption);

    parser.process(a);

    // get cli parameters
    QFile t_fileTrans(parser.value(transOption));

    bool bScale = false;
    if (parser.value(scaleOption) == "false" || parser.value(scaleOption) == "0") {
        bScale = false;
    } else if (parser.value(scaleOption) == "true" || parser.value(scaleOption) == "1") {
        bScale = true;
    }

    float fTol = parser.value(tolOption).toFloat();
    float fMaxDist = parser.value(distOption).toFloat();
    int iMaxIter = parser.value(iterOption).toInt();

    // read Trans
    const FiffCoordTrans transHeadMriRef(t_fileTrans.fileName().toStdU16String(), FIFFV_COORD_HEAD, FIFFV_COORD_MRI);

    // read Bem
    MNEBem bemHead(parser.value(bemOption).toStdU16String());
    std::shared_ptr<MNEBemSurface> bemSurface = std::make_shared<MNEBemSurface>(bemHead[0]);
    std::shared_ptr<MNEProjectToSurface> mneSurfacePoints = std::make_shared<MNEProjectToSurface>(*bemSurface);

    // read digitizer data
    const FiffDigPointSet dig(parser.value(digOption).toStdString());
    FiffDigPointSet digSetSrc = dig.pick_types({FIFFV_POINT_CARDINAL}); // Fiducials Head-Space
    FiffDigPointSet digSetDst = digSetSrc;
    digSetDst.apply_transform(transHeadMriRef);
    FiffDigPointSet digSetHsp = dig.pick_types({FIFFV_POINT_CARDINAL, FIFFV_POINT_HPI, FIFFV_POINT_EXTRA, FIFFV_POINT_EEG}); // Head shape points Head-Space

    // Initial Fiducial Alignment
    // Declare variables
    Matrix3f matSrc(digSetSrc.size(), 3);
    Matrix3f matDst(digSetDst.size(), 3);
    Matrix4f matTrans;
    Vector3f vecWeights(digSetSrc.size()); // LPA, Nasion, RPA
    float fScale = 1.0f;

    // get coordinates
    for (int i = 0; i < std::ssize(digSetSrc); ++i) {
        matSrc(i, 0) = digSetSrc[i].r[0];
        matSrc(i, 1) = digSetSrc[i].r[1];
        matSrc(i, 2) = digSetSrc[i].r[2];
        matDst(i, 0) = digSetDst[i].r[0];
        matDst(i, 1) = digSetDst[i].r[1];
        matDst(i, 2) = digSetDst[i].r[2];

        // set standart weights
        if (digSetSrc[i].ident == FIFFV_POINT_NASION) {
            vecWeights(i) = 10.0;
        } else {
            vecWeights(i) = 1.0;
        }
    }

    // align fiducials
    if (!mne::fitMatchedPoints(matSrc, matDst, matTrans, fScale, bScale, vecWeights)) {
        Log::warning("Point cloud registration not succesfull.");
    }

    fiff_int_t iFrom = digSetSrc[0].coord_frame;
    fiff_int_t iTo = bemSurface->coord_frame;
    FiffCoordTrans transHeadMri(iFrom, iTo, matTrans);

    // Prepare Icp:
    VectorXf vecWeightsICP(digSetHsp.size()); // Weigths vector
    MatrixXf matHsp(digSetHsp.size(), 3);

    for (int i = 0; i < std::ssize(digSetHsp); ++i) {
        matHsp(i, 0) = digSetHsp[i].r[0];
        matHsp(i, 1) = digSetHsp[i].r[1];
        matHsp(i, 2) = digSetHsp[i].r[2];
        // set standart weights
        if ((digSetHsp[i].kind == FIFFV_POINT_CARDINAL) && (digSetHsp[i].ident == FIFFV_POINT_NASION)) {
            vecWeightsICP(i) = 10.0;
        } else {
            vecWeightsICP(i) = 1.0;
        }
    }

    MatrixXf matHspClean;
    VectorXi vecTake;
    float fRMSE = 0.0;

    // discard outliers
    if (!mne::discard3DPointOutliers(mneSurfacePoints, matHsp, transHeadMri, vecTake, matHspClean, fMaxDist)) {
        Log::warning("Discard outliers was not succesfull.");
    }
    VectorXf vecWeightsICPClean(vecTake.size());

    for (int i = 0; i < vecTake.size(); ++i) {
        vecWeightsICPClean(i) = vecWeightsICP(vecTake(i));
    }

    // icp
    if (!mne::performIcp(mneSurfacePoints, matHspClean, transHeadMri, fRMSE, bScale, iMaxIter, fTol, vecWeightsICPClean)) {
        Log::warning("ICP was not succesfull.");
    }
    Log::info("transHeadMri:");
    transHeadMri.print();
    Log::info("transHeadMriRef:");
    transHeadMriRef.print();

    BrainView* pBrainView = new BrainView();
    BrainTreeModel* pModel = new BrainTreeModel();
    pBrainView->setModel(pModel);
    pModel->addDigitizerData(digSetSrc.points());
    pModel->addDigitizerData(digSetHsp.points());

    // Add BEM surfaces
    for (std::size_t i = 0; i < bemHead.size(); ++i) {
        pModel->addBemSurface("Sample", "Head", bemHead[i]);
    }

    pBrainView->show();

    return a.exec();
}
