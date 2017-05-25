//=============================================================================================================
/**
* @file     main.cpp
* @author   Lars Debor <lars.debor@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     May, 2017
*
* @section  LICENSE
*
* Copyright (C) 2017, Lars Debor and Matti Hamalainen. All rights reserved.
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
* @brief    Example of using the interpolation library and geometryInfo library
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <disp3D/engine/view/view3D.h>
#include <disp3D/engine/control/control3dwidget.h>
#include <disp3D/engine/model/items/sourceactivity/mneestimatetreeitem.h>
#include <disp3D/engine/model/data3Dtreemodel.h>

#include <fs/surfaceset.h>
#include <fs/annotationset.h>

#include <mne/mne_sourceestimate.h>
#include <mne/mne_bem.h>

#include <fiff/fiff_dig_point_set.h>

#include <inverse/minimumNorm/minimumnorm.h>

#include <iostream>

#include <geometryInfo/geometryinfo.h>

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QApplication>
#include <QMainWindow>
#include <QCommandLineParser>
#include<QDateTime>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISP3DLIB;
using namespace MNELIB;
using namespace FSLIB;
using namespace FIFFLIB;
using namespace INVERSELIB;
using namespace GEOMETRYINFO;
//using namespace INTERPOLATION;


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
    QApplication a(argc, argv);

    // Command Line Parser
    QCommandLineParser parser;
    parser.setApplicationDescription("ex_interpolation");
    parser.addHelpOption();

    QCommandLineOption subjectPathOption("subjectPath", "Selected subject path <subjectPath>.", "subjectPath", "./MNE-sample-data/subjects");
    QCommandLineOption surfOption("surfType", "Surface type <type>.", "type", "pial");
    QCommandLineOption annotOption("annotType", "Annotation type <type>.", "type", "aparc.a2009s");
    QCommandLineOption hemiOption("hemi", "Selected hemisphere <hemi>.", "hemi", "2");
    QCommandLineOption subjectOption("subject", "Selected subject <subject>.", "subject", "sample");

    parser.addOption(surfOption);
    parser.addOption(annotOption);
    parser.addOption(hemiOption);
    parser.addOption(subjectOption);
    parser.addOption(subjectPathOption);

    parser.process(a);


    //Inits
    //surfaceSet = 2x Surface
    // a Surface loads the vertexdata from a file and holds it in :
     ///   MatrixX3f m_matRR;      Vertex coordinates in meters
     /// MatrixX3i m_matTris;    The triangle descriptions
    //SurfaceSet tSurfSet (parser.value(subjectOption), parser.value(hemiOption).toInt(), parser.value(surfOption), parser.value(subjectPathOption));

    //contains Annotations
    ///https://surfer.nmr.mgh.harvard.edu/fswiki/LabelsClutsAnnotationFiles
    /// In FreeSurfer jargon, "annotation" refers to a collection of labels (ie: sets of vertices marked by label values)
    /// vertices are grouped by giving them the same label
    /// Annotations van contain a colortable
    //AnnotationSet tAnnotSet (parser.value(subjectOption), parser.value(hemiOption).toInt(), parser.value(annotOption), parser.value(subjectPathOption));


    //########################################################################################
    //
    //Source Estimate END
    //
    //########################################################################################

    //Create 3D data model
//    Data3DTreeModel::SPtr p3DDataModel = Data3DTreeModel::SPtr(new Data3DTreeModel());

//    //Add fressurfer surface set including both hemispheres
//    p3DDataModel->addSurfaceSet(parser.value(subjectOption), "MRI", tSurfSet, tAnnotSet);

    ///von lorenz für MNEBemSurface
//    QFile t_fileBem("./MNE-sample-data/subjects/sample/bem/sample-head.fif");
//    MNEBem t_Bem(t_fileBem);
//    p3DDataModel->addBemData("testData", "BEM", t_Bem);

    QFile t_filesensorSurfaceVV("./MNE-sample-data/subjects/sample/bem/sample-head.fif");
    MNEBem t_sensorSurfaceVV(t_filesensorSurfaceVV);


    MNEBemSurface &testSurface = t_sensorSurfaceVV[0];
    std::cout << "Number of vertices: ";
    std::cout << testSurface.rr.rows() << std::endl;

    QVector<qint32> subSet;
    int n_sub = 10;
    subSet.reserve(n_sub);
    int stepsize = 1;
    for(int i = 0; i < n_sub; ++i)
    {
        subSet.push_back(i * stepsize);
    }

    qint64 startTimeMsecs = QDateTime::currentMSecsSinceEpoch();

    QSharedPointer<MatrixXd> ptr = GeometryInfo::scdc(testSurface, subSet, 0.04);

    std::cout << "SCDC took ";
    std::cout << QDateTime::currentMSecsSinceEpoch()- startTimeMsecs <<" ms " << std::endl;

    // GeometryInfo::matrixDump(ptr, "output.txt");

    //Read and show sensor helmets
//    QFile t_filesensorSurfaceVV("./resources/sensorSurfaces/306m_rt.fif");
//    MNEBem t_sensorSurfaceVV(t_filesensorSurfaceVV);
//    p3DDataModel->addMegSensorData("Sensors", "VectorView", t_sensorSurfaceVV, evoked.info.chs);

    // Read & show digitizer points
//    QFile t_fileDig("./MNE-sample-data/MEG/sample/sample_audvis-ave.fif");
//    FiffDigPointSet t_Dig(t_fileDig);
//    p3DDataModel->addDigitizerData(parser.value(subjectOption), evoked.comment, t_Dig);



    //Create the 3D view
//    View3D::SPtr testWindow = View3D::SPtr(new View3D());
//    testWindow->setModel(p3DDataModel);
//    testWindow->show();

//    Control3DWidget::SPtr control3DWidget = Control3DWidget::SPtr(new Control3DWidget());
//    control3DWidget->init(p3DDataModel, testWindow);
//    control3DWidget->show();

    return a.exec();
}
