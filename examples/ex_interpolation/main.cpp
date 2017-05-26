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
#include <interpolation/interpolation.h>
#include <fiff/fiff_constants.h>

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QApplication>
#include <QMainWindow>
#include <QCommandLineParser>
#include <QDateTime>


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
using namespace INTERPOLATION;


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
    QCommandLineOption sampleEvokedFileOption("ave", "Path to the evoked/average <file>.", "file", "./MNE-sample-data/MEG/sample/sample_audvis-ave.fif");

    parser.addOption(surfOption);
    parser.addOption(annotOption);
    parser.addOption(hemiOption);
    parser.addOption(subjectOption);
    parser.addOption(subjectPathOption);
    parser.addOption(sampleEvokedFileOption);

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



    //acquire sensor positions
    QFile t_fileEvoked(parser.value(sampleEvokedFileOption));
    /// Load data
    fiff_int_t setno = 0;
    QPair<QVariant, QVariant> baseline(QVariant(), 0);
    FiffEvoked evoked(t_fileEvoked, setno, baseline);
    if(evoked.isEmpty())
    {
        return 1;
    }

    // positions of EEG and MEG sensors
    QVector<Vector3f> eegSensors;
    QVector<Vector3f> megSensors; //currently not used
    //fill both QVectors with the right sensor positions
    for( const FiffChInfo &info : evoked.info.chs)
    {
        //EEG
        if(info.kind == FIFFV_EEG_CH)
        {
            eegSensors.push_back(info.chpos.r0);
        }
        //MEG
        if(info.kind == FIFFV_MEG_CH)
        {
            megSensors.push_back(info.chpos.r0);
        }
    }
    std::cout << "Number EEG sensors: " << eegSensors.size() << std::endl;
    std::cout << "Number MEG sensors: " << megSensors.size() << std::endl;

    //acquire surface data
    QFile t_filesensorSurfaceVV("./MNE-sample-data/subjects/sample/bem/sample-head.fif");
    MNEBem t_sensorSurfaceVV(t_filesensorSurfaceVV);

    MNEBemSurface &testSurface = t_sensorSurfaceVV[0];
    std::cout << "Number of vertices: ";
    std::cout << testSurface.rr.rows() << std::endl;

    //projecting with EEG
    qint64 startTimeProjecting = QDateTime::currentMSecsSinceEpoch();
    QSharedPointer<QVector<qint32>> mappedSubSet = GeometryInfo::projectSensor(testSurface, megSensors);
    std::cout <<  "Projecting duration: " << QDateTime::currentMSecsSinceEpoch() - startTimeProjecting <<" ms " << std::endl;

    //SCDC with cancel distance 0.03
    qint64 startTimeScdc = QDateTime::currentMSecsSinceEpoch();
    QSharedPointer<MatrixXd> distanceMatrix = GeometryInfo::scdc(testSurface, *mappedSubSet, 0.03);
    std::cout << "SCDC duration: " << QDateTime::currentMSecsSinceEpoch() - startTimeScdc<< " ms " << std::endl;

    // linear weight matrix
    qint64 startTimeWMat = QDateTime::currentMSecsSinceEpoch();
    Interpolation::createInterpolationMat(*mappedSubSet, distanceMatrix);
    std::cout << "Weight matrix duration: " << QDateTime::currentMSecsSinceEpoch() - startTimeWMat<< " ms " << std::endl;

    // realtime interpolation (1 iteration)
    VectorXd signal = VectorXd::Random(megSensors.size());
    qint64 startTimeRTI = QDateTime::currentMSecsSinceEpoch();
    Interpolation::interpolateSignal(signal);
    std::cout << "Real time interpol. : " << QDateTime::currentMSecsSinceEpoch() - startTimeRTI << " ms " << std::endl;



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
