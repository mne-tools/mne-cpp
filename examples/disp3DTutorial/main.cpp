//=============================================================================================================
/**
* @file     main.cpp
* @author   Lorenz Esch Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     July, 2016
*
* @section  LICENSE
*
* Copyright (C) 2016, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
* @brief    Example of using the MNE-CPP Disp3D library
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <disp3D/view3D.h>
#include <disp3D/control/control3dwidget.h>
#include <disp3D/3DObjects/brain/brainrtsourcelocdatatreeitem.h>

#include <fs/label.h>
#include <fs/surfaceset.h>
#include <fs/annotationset.h>

#include <fiff/fiff_evoked.h>
#include <fiff/fiff.h>
#include <fiff/fiff_dig_point_set.h>

#include <mne/mne.h>
#include <mne/mne_epoch_data_list.h>
#include <mne/mne_sourceestimate.h>

#include <inverse/minimumNorm/minimumnorm.h>

#include <utils/mnemath.h>

#include <iostream>


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
using namespace FSLIB;
using namespace FIFFLIB;
using namespace INVERSELIB;
using namespace UTILSLIB;


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
    parser.setApplicationDescription("Start disp3D tutorial");
    parser.addHelpOption();
    QCommandLineOption sampleSurfOption("surfType", "Surface type <type>.", "type", "pial");
    QCommandLineOption sampleAnnotOption("annotType", "Annotation type <type>.", "type", "aparc.a2009s");
    QCommandLineOption sampleHemiOption("hemi", "Selected hemisphere <hemi>.", "hemi", "2");
    QCommandLineOption sampleSubjectOption("subject", "Selected subject <subject>.", "subject", "sample");
    QCommandLineOption sampleSubjectPathOption("subjectPath", "Selected subject path <subjectPath>.", "subjectPath", "./MNE-sample-data/subjects");
    QCommandLineOption sampleSourceLocOption("doSourceLoc", "Do real time source localization <doSourceLoc>.", "doSourceLoc", "false");
    QCommandLineOption sampleFwdOption("fwd", "Path to forwad solution <file>.", "file", "./MNE-sample-data/MEG/sample/sample_audvis-meg-eeg-oct-6-fwd.fif");
    QCommandLineOption sampleInvOpOption("invOp", "Path to inverse operator <file>.", "file", "");
    QCommandLineOption sampleClustOption("doClust", "Path to clustered inverse operator <doClust>.", "doClust", "true");

    parser.addOption(sampleSurfOption);
    parser.addOption(sampleAnnotOption);
    parser.addOption(sampleHemiOption);
    parser.addOption(sampleSubjectOption);
    parser.addOption(sampleSubjectPathOption);
    parser.addOption(sampleSourceLocOption);
    parser.addOption(sampleFwdOption);
    parser.addOption(sampleInvOpOption);
    parser.addOption(sampleClustOption);
    parser.process(a);

    bool bAddRtSourceLoc = parser.value(sampleSourceLocOption) == "false" ? false : true;
    bool bDoClustering = parser.value(sampleClustOption) == "false" ? false : true;

    //Inits
    SurfaceSet tSurfSet (parser.value(sampleSubjectOption), parser.value(sampleHemiOption).toInt(), parser.value(sampleSurfOption), parser.value(sampleSubjectPathOption));
    AnnotationSet tAnnotSet (parser.value(sampleSubjectOption), parser.value(sampleHemiOption).toInt(), parser.value(sampleAnnotOption), parser.value(sampleSubjectPathOption));

//    Surface tSurfRight ("sample", 1, "pial", "./MNE-sample-data/subjects");
//    Annotation tAnnotRight ("sample", 1, "aparc.a2009s", "./MNE-sample-data/subjects");
//    Surface tSurfLeft ("sample", 0, "orig", "./MNE-sample-data/subjects");
//    Annotation tAnnotLeft ("sample", 0, "aparc.a2009s", "./MNE-sample-data/subjects");

    QFile t_fileFwd(parser.value(sampleFwdOption));
    MNEForwardSolution t_Fwd(t_fileFwd);
    MNEForwardSolution t_clusteredFwd;

    QString t_sFileClusteredInverse(parser.value(sampleInvOpOption));//QFile t_fileClusteredInverse("./clusteredInverse-inv.fif");

    QFile t_fileCov("./MNE-sample-data/MEG/sample/sample_audvis-cov.fif");
    QFile t_fileEvoked("./MNE-sample-data/MEG/sample/sample_audvis-ave.fif");

    //########################################################################################
    //
    // Source Estimate START
    //
    //########################################################################################

    MNESourceEstimate sourceEstimate_LA;
    MNESourceEstimate sourceEstimate_RA;
    MNESourceEstimate sourceEstimate_LV;
    MNESourceEstimate sourceEstimate_RV;

    if(bAddRtSourceLoc) {
        double snr = 3.0;
        double lambda2 = 1.0 / pow(snr, 2);
        QString method("dSPM"); //"MNE" | "dSPM" | "sLORETA"

        // Load data
        QPair<QVariant, QVariant> baseline(QVariant(), 0);
        FiffEvoked evoked_LA(t_fileEvoked, 0, baseline);
        t_fileEvoked.close();
        FiffEvoked evoked_RA(t_fileEvoked, 1, baseline);
        t_fileEvoked.close();
        FiffEvoked evoked_LV(t_fileEvoked, 2, baseline);
        t_fileEvoked.close();
        FiffEvoked evoked_RV(t_fileEvoked, 3, baseline);
        t_fileEvoked.close();
        if(evoked_LA.isEmpty() || evoked_RA.isEmpty() ||evoked_LV.isEmpty() || evoked_RV.isEmpty())
            return 1;

        std::cout << std::endl;
        std::cout << "Evoked description: " << evoked_LA.comment.toLatin1().constData() << std::endl;
        std::cout << "Evoked description: " << evoked_RA.comment.toLatin1().constData() << std::endl;
        std::cout << "Evoked description: " << evoked_LV.comment.toLatin1().constData() << std::endl;
        std::cout << "Evoked description: " << evoked_RV.comment.toLatin1().constData() << std::endl;

        if(t_Fwd.isEmpty())
            return 1;

        FiffCov noise_cov(t_fileCov);

        // regularize noise covariance
        noise_cov = noise_cov.regularize(evoked_LA.info, 0.05, 0.05, 0.1, true);

        //
        // Cluster forward solution;
        //
        if(bDoClustering) {
            t_clusteredFwd = t_Fwd.cluster_forward_solution(tAnnotSet, 40);
        } else {
            t_clusteredFwd = t_Fwd;
        }

        //
        // make an inverse operators
        //
        FiffInfo info = evoked_LA.info;

        MNEInverseOperator inverse_operator(info, t_clusteredFwd, noise_cov, 0.2f, 0.8f);

        if(!t_sFileClusteredInverse.isEmpty())
        {
            QFile t_fileClusteredInverse(t_sFileClusteredInverse);
            inverse_operator.write(t_fileClusteredInverse);
        }

        //
        // Compute inverse solution
        //
        MinimumNorm minimumNorm(inverse_operator, lambda2, method);
        sourceEstimate_LA = minimumNorm.calculateInverse(evoked_LA);
        sourceEstimate_RA = minimumNorm.calculateInverse(evoked_RA);
        sourceEstimate_LV = minimumNorm.calculateInverse(evoked_LV);
        sourceEstimate_RV = minimumNorm.calculateInverse(evoked_RV);

        if(sourceEstimate_LA.isEmpty() || sourceEstimate_RA.isEmpty() ||sourceEstimate_LV.isEmpty() || sourceEstimate_RV.isEmpty())
            return 1;

        // View activation time-series
        std::cout << "\nsourceEstimate:\n" << sourceEstimate_LA.data.block(0,0,10,10) << std::endl;
        std::cout << "time\n" << sourceEstimate_LA.times.block(0,0,1,10) << std::endl;
        std::cout << "timeMin\n" << sourceEstimate_LA.times[0] << std::endl;
        std::cout << "timeMax\n" << sourceEstimate_LA.times[sourceEstimate_LA.times.size()-1] << std::endl;
        std::cout << "time step\n" << sourceEstimate_LA.tstep << std::endl;
    }

    //########################################################################################
    //
    //Source Estimate END
    //
    //########################################################################################

    //########################################################################################
    //
    // Create the test view START
    //
    //########################################################################################

    std::cout<<"Creating BrainView"<<std::endl;

    //Create the 3D view
    View3D::SPtr testWindow = View3D::SPtr(new View3D());

    //Add fressurfer surfaces per hemisphere
//    testWindow->addSurface("Subject01", "HemiLRSet", tSurfLeft, tAnnotLeft);
//    testWindow->addSurface("Subject01", "HemiLRSet", tSurfRight, tAnnotRight);

    //Add fressurfer surface set including both hemispheres
    //testWindow->addSurfaceSet("Subject01", "Right Auditory", tSurfSet, tAnnotSet);
    testWindow->addSurfaceSet("Subject01", "Right Visual", tSurfSet, tAnnotSet);
    //testWindow->addSurfaceSet("Subject01", "Left Auditory", tSurfSet, tAnnotSet);
    //testWindow->addSurfaceSet("Subject01", "Left Visual", tSurfSet, tAnnotSet);

//    //Read and show BEM
//    QFile t_fileBem("./MNE-sample-data/subjects/sample/bem/sample-5120-5120-5120-bem.fif");
//    MNEBem t_Bem(t_fileBem);
//    testWindow->addBemData("Subject01", "BEM", t_Bem);

//    QFile t_fileBem2("./MNE-sample-data/subjects/sample/bem/sample-head.fif");
//    MNEBem t_Bem2(t_fileBem2);
//    testWindow->addBemData("Subject01", "BEM", t_Bem2);

//    //Read and show sensor helmets
//    QFile t_filesensorSurfaceVV("./resources/sensorSurfaces/306m_rt.fif");
//    MNEBem t_sensorSurfaceVV(t_filesensorSurfaceVV);
//    testWindow->addBemData("Sensors", "VectorView", t_sensorSurfaceVV);

//    QFile t_filesensorSurfaceBM("./resources/sensorSurfaces/BabyMEG.fif");
//    MNEBem t_sensorSurfaceBM(t_filesensorSurfaceBM);
//    testWindow->addBemData("Sensors", "BabyMEG", t_sensorSurfaceBM);

//    // Read & show digitizer points
//    QFile t_fileDig("./MNE-sample-data/MEG/sample/sample_audvis-ave.fif");
//    FiffDigPointSet t_Dig(t_fileDig);
//    testWindow->addDigitizerData("Subject01", "Left Auditory", t_Dig);

    if(bAddRtSourceLoc) {
        //Add rt source loc data
        //QList<BrainRTSourceLocDataTreeItem*> rtItemList_LA = testWindow->addSourceData("Subject01", "Left Auditory", sourceEstimate_LA, t_clusteredFwd);
        //QList<BrainRTSourceLocDataTreeItem*> rtItemList_RA = testWindow->addSourceData("Subject01", "Right Auditory", sourceEstimate_RA, t_clusteredFwd);
        //QList<BrainRTSourceLocDataTreeItem*> rtItemList_LV = testWindow->addSourceData("Subject01", "Left Visual", sourceEstimate_LV, t_clusteredFwd);
        QList<BrainRTSourceLocDataTreeItem*> rtItemList_RV = testWindow->addSourceData("Subject01", "Right Visual", sourceEstimate_RV, t_clusteredFwd);

        //Init some rt related values for right visual data
        for(int i = 0; i < rtItemList_RV.size(); ++i) {
            rtItemList_RV.at(i)->setLoopState(true);
            rtItemList_RV.at(i)->setTimeInterval(17);
            rtItemList_RV.at(i)->setNumberAverages(1);
            rtItemList_RV.at(i)->setStreamingActive(true);
            rtItemList_RV.at(i)->setNormalization(QVector3D(0.0,5.5,10));
            rtItemList_RV.at(i)->setVisualizationType("Annotation based");
            rtItemList_RV.at(i)->setColortable("Hot");
        }
    }

    testWindow->show();

    Control3DWidget::SPtr control3DWidget = Control3DWidget::SPtr(new Control3DWidget());
    control3DWidget->setView3D(testWindow);
    control3DWidget->show();

    //########################################################################################
    //
    // Create the test view END
    //
    //########################################################################################

    return a.exec();
}
