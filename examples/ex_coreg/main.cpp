//=============================================================================================================
/**
 * @file     main.cpp
 * @author   Ruben Dörfel <doerfelruben@aol.com>
 * @since    0.1.5
 * @date     July, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020, Ruben Dörfel. All rights reserved.
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
 *
 * @brief     Examplethat shows the coregistration workflow.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <iostream>

#include <disp3D/viewers/abstractview.h>
#include <disp3D/engine/model/items/digitizer/digitizersettreeitem.h>
#include <disp3D/engine/model/data3Dtreemodel.h>
#include <disp3D/engine/model/items/bem/bemsurfacetreeitem.h>
#include <disp3D/engine/model/items/bem/bemtreeitem.h>

#include "fiff/fiff_dig_point_set.h"
#include "fiff/fiff_dig_point.h"
#include "fiff/fiff_coord_trans.h"

#include "mne/mne_bem_surface.h"
#include "mne/mne_project_to_surface.h"

#include <utils/generics/applicationlogger.h>
#include "rtprocessing/icp.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QApplication>
#include <QMainWindow>
#include <QCommandLineParser>
#include <QDebug>
#include <QFile>

//=============================================================================================================
// Eigen
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace UTILSLIB;
using namespace FIFFLIB;
using namespace DISP3DLIB;
using namespace MNELIB;

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
int main(int argc, char *argv[])
{
    #ifdef STATICBUILD
    Q_INIT_RESOURCE(disp3d);
    #endif

    qInstallMessageHandler(ApplicationLogger::customLogWriter);
    QApplication a(argc, argv);

    // Command Line Parser
    QCommandLineParser parser;
    parser.setApplicationDescription("Example Coregistration");
    parser.addHelpOption();

    QCommandLineOption digOption("dig", "The destination point set", "file", QCoreApplication::applicationDirPath() + "/MNE-sample-data/MEG/sample/sample_audvis-ave.fif");
    QCommandLineOption bemOption("bem", "The bem file", "file", QCoreApplication::applicationDirPath() + "/MNE-sample-data/subjects/sample/bem/sample-head.fif");
    QCommandLineOption transOption("trans", "The MRI-Head transformation file", "file", QCoreApplication::applicationDirPath() + "/MNE-sample-data/MEG/sample/sample_audvis_raw-trans.fif");
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
    QFile t_fileDig(parser.value(digOption));
    QFile t_fileBem(parser.value(bemOption));
    QFile t_fileTrans(parser.value(transOption));

    bool bScale = false;
    if(parser.value(scaleOption) == "false" || parser.value(scaleOption) == "0") {
        bScale = false;
    } else if(parser.value(scaleOption) == "true" || parser.value(scaleOption) == "1") {
        bScale = true;
    }

    float fTol = parser.value(tolOption).toFloat();
    float fMaxDist = parser.value(distOption).toFloat();
    int iMaxIter = parser.value(iterOption).toInt();

    // read Trans
    FiffCoordTrans transHeadMriRef(t_fileTrans);

    // read Bem
    MNEBem bemHead(t_fileBem);
    MNEBemSurface::SPtr bemSurface = MNEBemSurface::SPtr::create(bemHead[0]);
    MNEProjectToSurface::SPtr mneSurfacePoints = MNEProjectToSurface::SPtr::create(*bemSurface);

    // read digitizer data
    QList<int> lPickFiducials({FIFFV_POINT_CARDINAL});
    QList<int> lPickHSP({FIFFV_POINT_CARDINAL,FIFFV_POINT_HPI,FIFFV_POINT_EXTRA,FIFFV_POINT_EEG});
    FiffDigPointSet digSetSrc = FiffDigPointSet(t_fileDig).pickTypes(lPickFiducials);   // Fiducials Head-Space
    FiffDigPointSet digSetDst = FiffDigPointSet(t_fileDig).pickTypes(lPickFiducials);
    digSetDst.applyTransform(transHeadMriRef, false);
    FiffDigPointSet digSetHsp = FiffDigPointSet(t_fileDig).pickTypes(lPickHSP);         // Head shape points Head-Space

    // Initial Fiducial Alignment
    // Declare variables
    Matrix3f matSrc(digSetSrc.size(),3);
    Matrix3f matDst(digSetDst.size(),3);
    Matrix4f matTrans;
    Vector3f vecWeights(digSetSrc.size()); // LPA, Nasion, RPA
    float fScale = 1.0f;

    // get coordinates
    for(int i = 0; i< digSetSrc.size(); ++i) {
        matSrc(i,0) = digSetSrc[i].r[0]; matSrc(i,1) = digSetSrc[i].r[1]; matSrc(i,2) = digSetSrc[i].r[2];
        matDst(i,0) = digSetDst[i].r[0]; matDst(i,1) = digSetDst[i].r[1]; matDst(i,2) = digSetDst[i].r[2];

        // set standart weights
        if(digSetSrc[i].ident == FIFFV_POINT_NASION) {
            vecWeights(i) = 10.0;
        } else {
            vecWeights(i) = 1.0;
        }
    }

    // align fiducials
    if(!RTPROCESSINGLIB::fitMatchedPoints(matSrc,matDst,matTrans,fScale,bScale,vecWeights)) {
        qWarning() << "Point cloud registration not succesfull.";
    }

    fiff_int_t iFrom = digSetSrc[0].coord_frame;
    fiff_int_t iTo = bemSurface->coord_frame;
    FiffCoordTrans transHeadMri = FiffCoordTrans::make(iFrom, iTo, matTrans);

    // Prepare Icp:
    VectorXf vecWeightsICP(digSetHsp.size()); // Weigths vector
    MatrixXf matHsp(digSetHsp.size(),3);

    for(int i = 0; i < digSetHsp.size(); ++i) {
        matHsp(i,0) = digSetHsp[i].r[0]; matHsp(i,1) = digSetHsp[i].r[1]; matHsp(i,2) = digSetHsp[i].r[2];
        // set standart weights
        if((digSetHsp[i].kind == FIFFV_POINT_CARDINAL) && (digSetHsp[i].ident == FIFFV_POINT_NASION)) {
            vecWeightsICP(i) = 10.0;
        } else {
            vecWeightsICP(i) = 1.0;
        }
    }

    MatrixXf matHspClean;
    VectorXi vecTake;
    float fRMSE = 0.0;

    // discard outliers
    if(!RTPROCESSINGLIB::discard3DPointOutliers(mneSurfacePoints, matHsp, transHeadMri, vecTake, matHspClean, fMaxDist)) {
        qWarning() << "Discard outliers was not succesfull.";
    }
    VectorXf vecWeightsICPClean(vecTake.size());

    for(int i = 0; i < vecTake.size(); ++i) {
        vecWeightsICPClean(i) = vecWeightsICP(vecTake(i));
    }

    // icp
    if(!RTPROCESSINGLIB::performIcp(mneSurfacePoints, matHspClean, transHeadMri, fRMSE, bScale, iMaxIter, fTol, vecWeightsICPClean)) {
        qWarning() << "ICP was not succesfull.";
    }
    qInfo() << "transHeadMri:";
    transHeadMri.print();
    qInfo() << "transHeadMriRef:";
    transHeadMriRef.print();

    AbstractView::SPtr p3DAbstractView = AbstractView::SPtr(new AbstractView());
    Data3DTreeModel::SPtr p3DDataModel = p3DAbstractView->getTreeModel();
    DigitizerSetTreeItem* pDigSrcSetTreeItem = p3DDataModel->addDigitizerData("Sample", "Fiducials Transformed", digSetSrc);
//    DigitizerSetTreeItem* pDigHspSetTreeItem = p3DDataModel->addDigitizerData("Sample", "Digitizer", digSetHsp);
    pDigSrcSetTreeItem->setTransform(transHeadMri,true);

    BemTreeItem* pBemItem = p3DDataModel->addBemData("Sample", "Head", bemHead);
    QList<QStandardItem*> itemList = pBemItem->findChildren(Data3DTreeModelItemTypes::BemSurfaceItem);
    for(int j = 0; j < itemList.size(); ++j) {
        if(BemSurfaceTreeItem* pBemItem = dynamic_cast<BemSurfaceTreeItem*>(itemList.at(j))) {
            pBemItem->setTransform(transHeadMri,true);
        }
    }

    p3DAbstractView->show();

    return a.exec();
}
