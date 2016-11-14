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
* @brief    Example of using the MNE-CPP Connectivity library
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <disp3D/view3D.h>
#include <disp3D/control/control3dwidget.h>
#include <disp3D/3DObjects/brain/brainrtsourcelocdatatreeitem.h>
#include <disp/imagesc.h>

#include <fs/label.h>
#include <fs/surfaceset.h>
#include <fs/annotationset.h>

#include <fiff/fiff_evoked.h>
#include <fiff/fiff.h>

#include <connectivity/connectivitymeasures.h>

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

using namespace DISPLIB;
using namespace DISP3DLIB;
using namespace MNELIB;
using namespace FSLIB;
using namespace FIFFLIB;
using namespace INVERSELIB;
using namespace UTILSLIB;
using namespace CONNECTIVITYLIB;


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
    parser.setApplicationDescription("Start connectivity tutorial");
    parser.addHelpOption();
    QCommandLineOption surfOption("surfType", "Surface type <type>.", "type", "orig");
    QCommandLineOption annotOption("annotType", "Annotation type <type>.", "type", "aparc.a2009s");
    QCommandLineOption hemiOption("hemi", "Selected hemisphere <hemi>.", "hemi", "2");
    QCommandLineOption subjectOption("subj", "Selected subject <subject>.", "subject", "sample");
    QCommandLineOption subjectPathOption("subjDir", "Selected subject path <subjectPath>.", "subjectPath", "./MNE-sample-data/subjects");
    QCommandLineOption sourceLocOption("doSourceLoc", "Do real time source localization <doSourceLoc>.", "doSourceLoc", "true");
    QCommandLineOption fwdOption("fwd", "Path to forwad solution <file>.", "file", "./MNE-sample-data/MEG/sample/sample_audvis-meg-eeg-oct-6-fwd.fif");
    QCommandLineOption invOpOption("invOp", "Path to inverse operator <file>, which is to be written.", "file", "");
    QCommandLineOption clustOption("doClust", "Path to clustered inverse operator <doClust>.", "doClust", "true");
    QCommandLineOption covFileOption("cov", "Path to the covariance <file>.", "file", "./MNE-sample-data/MEG/sample/sample_audvis-cov.fif");
    QCommandLineOption evokedFileOption("ave", "Path to the evoked/average <file>.", "file", "./MNE-sample-data/MEG/sample/sample_audvis-ave.fif");
    QCommandLineOption methodOption("method", "Inverse estimation <method>, i.e., 'MNE', 'dSPM' or 'sLORETA'.", "method", "dSPM");//"MNE" | "dSPM" | "sLORETA"
    QCommandLineOption snrOption("snr", "The SNR value used for computation <snr>.", "snr", "3.0");//3.0f;//0.1f;//3.0f;
    QCommandLineOption evokedIndexOption("aveIdx", "The average <index> to choose from the average file.", "index", "0");

    parser.addOption(surfOption);
    parser.addOption(annotOption);
    parser.addOption(hemiOption);
    parser.addOption(subjectOption);
    parser.addOption(subjectPathOption);
    parser.addOption(sourceLocOption);
    parser.addOption(fwdOption);
    parser.addOption(invOpOption);
    parser.addOption(clustOption);
    parser.addOption(covFileOption);
    parser.addOption(evokedFileOption);
    parser.addOption(methodOption);
    parser.addOption(snrOption);
    parser.addOption(evokedIndexOption);
    parser.process(a);

    bool bAddRtSourceLoc = parser.value(sourceLocOption) == "false" ? false : true;
    bool bDoClustering = parser.value(clustOption) == "false" ? false : true;

    //Inits
    SurfaceSet tSurfSet (parser.value(subjectOption), parser.value(hemiOption).toInt(), parser.value(surfOption), parser.value(subjectPathOption));
    AnnotationSet tAnnotSet (parser.value(subjectOption), parser.value(hemiOption).toInt(), parser.value(annotOption), parser.value(subjectPathOption));

    QFile t_fileFwd(parser.value(fwdOption));
    MNEForwardSolution t_Fwd(t_fileFwd);
    MNEForwardSolution t_clusteredFwd;

    QString t_sFileClusteredInverse(parser.value(invOpOption));

    QFile t_fileCov(parser.value(covFileOption));
    QFile t_fileEvoked(parser.value(evokedFileOption));

    //########################################################################################
    //
    // Source Estimate START
    //
    //########################################################################################

    // Load data
    QPair<QVariant, QVariant> baseline(QVariant(), 0);
    MNESourceEstimate sourceEstimate;
    FiffEvoked evoked(t_fileEvoked, parser.value(evokedIndexOption).toInt(), baseline);

    if(bAddRtSourceLoc) {
        double snr = parser.value(snrOption).toDouble();
        double lambda2 = 1.0 / pow(snr, 2);
        QString method(parser.value(methodOption));

        t_fileEvoked.close();

        if(evoked.isEmpty())
            return 1;

        std::cout << std::endl;
        std::cout << "Evoked description: " << evoked.comment.toLatin1().constData() << std::endl;

        if(t_Fwd.isEmpty())
            return 1;

        FiffCov noise_cov(t_fileCov);

        // regularize noise covariance
        noise_cov = noise_cov.regularize(evoked.info, 0.05, 0.05, 0.1, true);

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
        FiffInfo info = evoked.info;

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
        sourceEstimate = minimumNorm.calculateInverse(evoked);

        if(sourceEstimate.isEmpty())
            return 1;

        // View activation time-series
        std::cout << "\nsourceEstimate:\n" << sourceEstimate.data.block(0,0,10,10) << std::endl;
        std::cout << "time\n" << sourceEstimate.times.block(0,0,1,10) << std::endl;
        std::cout << "timeMin\n" << sourceEstimate.times[0] << std::endl;
        std::cout << "timeMax\n" << sourceEstimate.times[sourceEstimate.times.size()-1] << std::endl;
        std::cout << "time step\n" << sourceEstimate.tstep << std::endl;
    }

    //########################################################################################
    //
    // Source Estimate END
    //
    //########################################################################################

    //########################################################################################
    //
    // Do connectivity analysis START
    //
    //########################################################################################

    //Generate node vertices
    MatrixX3f matNodeVertLeft, matNodeVertRight, matNodeVertComb;

    if(bDoClustering) {
        matNodeVertLeft.resize(t_clusteredFwd.src[0].cluster_info.centroidVertno.size(),3);

        for(int j = 0; j < matNodeVertLeft.rows(); ++j) {
            matNodeVertLeft.row(j) = tSurfSet[0].rr().row(t_clusteredFwd.src[0].cluster_info.centroidVertno.at(j)) - tSurfSet[0].offset().transpose();
        }

        matNodeVertRight.resize(t_clusteredFwd.src[1].cluster_info.centroidVertno.size(),3);
        for(int j = 0; j < matNodeVertRight.rows(); ++j) {
            matNodeVertRight.row(j) = tSurfSet[1].rr().row(t_clusteredFwd.src[1].cluster_info.centroidVertno.at(j)) - tSurfSet[1].offset().transpose();
        }
    } else {
        matNodeVertLeft.resize(t_Fwd.src[0].vertno.rows(),3);
        for(int j = 0; j < matNodeVertLeft.rows(); ++j) {
            matNodeVertLeft.row(j) = tSurfSet[0].rr().row(t_Fwd.src[0].vertno(j)) - tSurfSet[0].offset().transpose();
        }

        matNodeVertRight.resize(t_Fwd.src[1].vertno.rows(),3);
        for(int j = 0; j < matNodeVertRight.rows(); ++j) {
            matNodeVertRight.row(j) = tSurfSet[1].rr().row(t_Fwd.src[1].vertno(j)) - tSurfSet[1].offset().transpose();
        }
    }

    matNodeVertComb.resize(matNodeVertLeft.rows()+matNodeVertRight.rows(),3);
    matNodeVertComb << matNodeVertLeft, matNodeVertRight;

    Network::SPtr pConnect_LA = ConnectivityMeasures::pearsonsCorrelationCoeff(sourceEstimate.data, matNodeVertComb);
//    Network::SPtr pConnect_LA = ConnectivityMeasures::crossCorrelation(sourceEstimate.data, matNodeVertComb);

    //########################################################################################
    //
    // Do connectivity analysis END
    //
    //########################################################################################

    //########################################################################################
    //
    // Create the test view START
    //
    //########################################################################################

    View3D::SPtr testWindow = View3D::SPtr(new View3D());
    testWindow->addSurfaceSet("Subject01", evoked.comment, tSurfSet, tAnnotSet);

    QList<BrainRTConnectivityDataTreeItem*> rtItemListConnect= testWindow->addConnectivityData("Subject01", evoked.comment, pConnect_LA);

    QList<BrainRTSourceLocDataTreeItem*> rtItemListSourceLoc = testWindow->addSourceData("Subject01", evoked.comment, sourceEstimate, t_clusteredFwd);
    //Init some rt related values for right visual data
    for(int i = 0; i < rtItemListSourceLoc.size(); ++i) {
        rtItemListSourceLoc.at(i)->setLoopState(true);
        rtItemListSourceLoc.at(i)->setTimeInterval(17);
        rtItemListSourceLoc.at(i)->setNumberAverages(1);
        rtItemListSourceLoc.at(i)->setStreamingActive(true);
        rtItemListSourceLoc.at(i)->setNormalization(QVector3D(0.0,5.5,10));
        rtItemListSourceLoc.at(i)->setVisualizationType("Annotation based");
        rtItemListSourceLoc.at(i)->setColortable("Hot");
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
