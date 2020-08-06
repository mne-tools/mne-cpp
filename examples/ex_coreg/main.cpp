//=============================================================================================================
/**
 * @file     main.cpp
 * @author   Ruben Dörfel <doerfelruben@aol.com>
 * @since    0.1.0
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
#include <disp3D/engine/view/view3D.h>
#include <disp3D/helpers/geometryinfo/geometryinfo.h>
#include <disp3D/helpers/geometryinfo/geometryinfo.h>
#include <disp3D/helpers/interpolation/interpolation.h>

#include "fiff/fiff_dig_point_set.h"
#include "fiff/fiff_dig_point.h"
#include "fiff/fiff_coord_trans.h"

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
using namespace RTPROCESSINGLIB;
using namespace FIFFLIB;
using namespace DISP3DLIB;

//=============================================================================================================
// MAIN
//=============================================================================================================

//=============================================================================================================
/**
 * The function main marks the entry point of the program.
 * By default, main has the storage class extern.
 *
 * @param [in] argc (argument count) is an integer that indicates how many arguments were entered on the command line when the program was started.
 * @param [in] argv (argument vector) is an array of pointers to arrays of character objects. The array objects are null-terminated strings, representing the arguments that were entered on the command line when the program was started.
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

    QCommandLineOption srcOption("src", "The original point set", "file", QCoreApplication::applicationDirPath() + "/MNE-sample-data/coreg/sample-fiducials.fif");
    QCommandLineOption dstOption("dst", "The destination point set", "file", QCoreApplication::applicationDirPath() + "/MNE-sample-data/MEG/sample/sample_audvis-ave.fif");
    QCommandLineOption scaleOption("scale", "Weather to scale during the registration or not", "bool", "false");

    parser.addOption(srcOption);
    parser.addOption(dstOption);
    parser.addOption(scaleOption);
    parser.process(a);

    // get cli parameters
    QFile t_fileSrc(parser.value(srcOption));
    QFile t_fileDst(parser.value(dstOption));

    bool bScale = false;
    if(parser.value(scaleOption) == "false" || parser.value(scaleOption) == "0") {
        bScale = false;
    } else if(parser.value(scaleOption) == "true" || parser.value(scaleOption) == "1") {
        bScale = true;
    }


    // read digitizer data
    QList<int> lPickFiducials({FIFFV_POINT_CARDINAL});
    FiffDigPointSet digSetSrc = FiffDigPointSet(t_fileSrc).pickTypes(lPickFiducials);
    FiffDigPointSet digSetDst = FiffDigPointSet(t_fileDst).pickTypes(lPickFiducials);

    // Declare variables
    Matrix3f matSrc(digSetSrc.size(),3);
    Matrix3f matDst(digSetDst.size(),3);
    Matrix4f matTrans;
    Vector3f vecWeights(digSetSrc.size()); // LPA, Nasion, RPA
    float fScale;

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

    if(!fitMatched(matSrc,matDst,matTrans,fScale,bScale,vecWeights)) {
        qWarning() << "point cloud registration not succesfull";
    }

    FiffCoordTrans transMriHead = FiffCoordTrans::make(digSetSrc[0].coord_frame, digSetDst[0].coord_frame,matTrans);
    Matrix3f matSrcTransposed = transMriHead.apply_trans(matSrc);
    Matrix3f matDiff = matDst - matSrcTransposed;
    qInfo() << "Transformation Matrix: ";
    std::cout << transMriHead.trans << std::endl;

    qInfo() << "Alignment Error: ";
    std::cout << matDiff.colwise().mean() << std::endl;


    // Abstract View
    AbstractView::SPtr p3DAbstractView = AbstractView::SPtr(new AbstractView());
    Data3DTreeModel::SPtr p3DDataModel = p3DAbstractView->getTreeModel();
    DigitizerSetTreeItem* pDigSrcSetTreeItem = p3DDataModel->addDigitizerData("Sample", "Source", digSetSrc);
    DigitizerSetTreeItem* pDigDstSetTreeItem = p3DDataModel->addDigitizerData("Sample", "Destination", digSetDst);
    pDigSrcSetTreeItem->setTransform(transMriHead,false);

    p3DAbstractView->show();

    return a.exec();
}
