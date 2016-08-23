//=============================================================================================================
/**
* @file     main.cpp
* @author   Jana Kiesel <jana.kiesel@tu-ilmenau.de>
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     August, 2016
* @section  LICENSE
*
* Copyright (C) 2016, Jana Kiesel and Matti Hamalainen. All rights reserved.
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
* @brief    Example to project points on a surface
*
*/
//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/mne_project_to_surface.h>
#include <mne/mne.h>
#include <disp3D/view3D.h>
#include <disp3D/control/control3dwidget.h>
#include <fiff/fiff_dig_point_set.h>
#include <utils/warp.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QApplication>
#include <QMainWindow>
#include <QCommandLineParser>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISP3DLIB;
using namespace MNELIB;
using namespace FIFFLIB;


//*************************************************************************************************************
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
    QApplication app(argc, argv);

    // Command Line Parser
    QCommandLineParser parser;
    parser.setApplicationDescription("Projection Example");
    parser.addHelpOption();
    QCommandLineOption sampleBEMFileOption("f", "Path to BEM <file>.", "file",
                                           "./MNE-sample-data/warping/AVG4-0Years_segmented_BEM3/bem/AVG4-0Years_segmented_BEM3-2118-2188-3186-bem-sol.fif");

    parser.addOption(sampleBEMFileOption);
    parser.process(app);

    //########################################################################################
    // Read the BEM
    QFile t_fileBem(parser.value(sampleBEMFileOption));
    MNEBem t_Bem(t_fileBem);

    // Read the Digitizer
    QFile t_fileDig = "./MNE-sample-data/warping/AVG4-0Years_GSN128.fif";
    FiffDigPointSet t_Dig(t_fileDig);

    MatrixXf ElecPos(t_Dig.size(), 3);
    for (int i = 0; i < t_Dig.size(); ++i)
    {
        ElecPos(i,0) = t_Dig[i].r[0];
        ElecPos(i,1) = t_Dig[i].r[1];
        ElecPos(i,2) = t_Dig[i].r[2];
    }

    // Read and apply Transformation
    QFile t_fileTrans("./MNE-sample-data/warping/AVG4-0Years_GSN128-trans.fif");
    FiffCoordTrans t_Trans (t_fileTrans);
    ElecPos=t_Trans.apply_trans(ElecPos);
    FiffDigPointSet t_DigTrans(t_Dig);

    for (int i = 0; i < t_DigTrans.size(); ++i)
    {
        t_DigTrans[i].r[0] = ElecPos(i,0);
        t_DigTrans[i].r[1] = ElecPos(i,1);
        t_DigTrans[i].r[2] = ElecPos(i,2);
    }

    //Projection
    MatrixXf DigProject(t_DigTrans.size(), 3);
    VectorXi nearest;
    VectorXf dist;
    MNEProjectToSurface t_Avg4 (t_Bem[0]);

    t_Avg4.mne_find_closest_on_surface(ElecPos, t_DigTrans.size(), DigProject, nearest, dist);

    FiffDigPointSet t_DigProject(t_Dig);

    for (int i = 0; i < t_DigProject.size(); ++i)
    {
        t_DigProject[i].r[0] = DigProject(i,0);
        t_DigProject[i].r[1] = DigProject(i,1);
        t_DigProject[i].r[2] = DigProject(i,2);
    }

    //
    // calculate Warp
    //
    Warp  t_Avg4Warp;
    MatrixXd wVert(sVert.rows(),3);
    wVert = test.calculate(DigProject, ElecPos, t_Bem);

    //
    // WRITE NEW VERTICES BACK TO BEM
    //
    skin.rr=wVert.cast<float>();
    skin.addVertexNormals();

    std::cout << "Here are the first row of the matrix skin.rr after warp:" << std::endl << skin.rr.topRows(9) << std::endl;
//    std::cout << "Here is the first row of the final matrix skin.tris:" << std::endl << skin.tris.topRows(9) << std::endl;
//    std::cout << "Here is the last row of the final matrix skin.tris:" << std::endl << skin.tris.bottomRows(1) << std::endl;

    MNELIB::MNEBem t_BemWarpedA;
    t_BemWarpedA<<skin;
    QFile t_fileBemWarped("./MNE-sample-data/subjects/sample/bem/sample-5120-5120-5120-bem-warped.fif");
    t_BemWarpedA.write(t_fileBemWarped);
    t_fileBemWarped.close();

    MNELIB::MNEBem t_BemWarpedB (t_fileBemWarped) ;
    MNELIB::MNEBemSurface skinWarped=t_BemWarpedB[0];

    //Show
    View3D::SPtr testWindow = View3D::SPtr(new View3D());
    testWindow->addBemData("AVG4-0Years", "BEM", t_Bem);
    testWindow->addDigitizerData("AVG4-0Years", "Orignal Dig", t_Dig);
    testWindow->addDigitizerData("AVG4-0Years", "Trans Dig", t_DigTrans);
    testWindow->addDigitizerData("AVG4-0Years", "Project Dig", t_DigProject);

    testWindow->show();

    Control3DWidget::SPtr control3DWidget = Control3DWidget::SPtr(new Control3DWidget());
    control3DWidget->setView3D(testWindow);
    control3DWidget->show();


    return app.exec();
}
