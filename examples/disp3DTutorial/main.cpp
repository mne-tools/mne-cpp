//=============================================================================================================
/**
* @file     main.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     July, 2012
*
* @section  LICENSE
*
* Copyright (C) 2012, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Example of an FreeSurfer Surface application
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <disp3D/view3D.h>
#include <disp3D/control/control3dwidget.h>

#include "disp/helpers/roundededgeswidget.h"

#include <fs/label.h>
#include <fs/surfaceset.h>
#include <fs/annotationset.h>

#include <fiff/fiff_evoked.h>
#include <fiff/fiff.h>
#include <mne/mne.h>

#include <mne/mne_epoch_data_list.h>

#include <mne/mne_sourceestimate.h>
#include <inverse/minimumNorm/minimumnorm.h>

#include <utils/mnemath.h>

#include <iostream>

#include <stdlib.h>     //for using the function sleep


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QApplication>
#include <QMainWindow>


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

    //########################################################################################
    //
    // Source Estimate START
    //
    //########################################################################################

    QFile t_fileFwd("./MNE-sample-data/MEG/sample/sample_audvis-meg-eeg-oct-6-fwd.fif");
    QFile t_fileCov("./MNE-sample-data/MEG/sample/sample_audvis-cov.fif");
    QFile t_fileEvoked("./MNE-sample-data/MEG/sample/sample_audvis-ave.fif");

    QString t_sFileClusteredInverse("");//QFile t_fileClusteredInverse("./clusteredInverse-inv.fif");
    AnnotationSet t_annotationSet ("sample", 2, "aparc.a2009s", "./MNE-sample-data/subjects");

    double snr = 1.0;
    double lambda2 = 1.0 / pow(snr, 2);
    QString method("dSPM"); //"MNE" | "dSPM" | "sLORETA"

    // Load data
    fiff_int_t setno = 1;
    QPair<QVariant, QVariant> baseline(QVariant(), 0);
    FiffEvoked evoked(t_fileEvoked, setno, baseline);
    if(evoked.isEmpty())
        return 1;

    std::cout << "Evoked description: " << evoked.comment.toLatin1().constData() << std::endl;

    MNEForwardSolution t_Fwd(t_fileFwd);
    if(t_Fwd.isEmpty())
        return 1;

    FiffCov noise_cov(t_fileCov);

    // regularize noise covariance
    noise_cov = noise_cov.regularize(evoked.info, 0.05, 0.05, 0.1, true);

    //
    // Cluster forward solution;
    //
    MNEForwardSolution t_clusteredFwd = t_Fwd;//.cluster_forward_solution(t_annotationSet, 40);

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
    MNESourceEstimate sourceEstimate = minimumNorm.calculateInverse(evoked);

    if(sourceEstimate.isEmpty())
        return 1;

//    // View activation time-series
//    std::cout << "\nsourceEstimate:\n" << sourceEstimate.data.block(0,0,10,10) << std::endl;
//    std::cout << "time\n" << sourceEstimate.times.block(0,0,1,10) << std::endl;
//    std::cout << "timeMin\n" << sourceEstimate.times[0] << std::endl;
//    std::cout << "timeMax\n" << sourceEstimate.times[sourceEstimate.times.size()-1] << std::endl;
//    std::cout << "time step\n" << sourceEstimate.tstep << std::endl;

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

    SurfaceSet tSurfSet ("sample", 2, "inflated", "./MNE-sample-data/subjects");
    AnnotationSet tAnnotSet ("sample", 2, "aparc.a2009s", "./MNE-sample-data/subjects");
    //Surface tSurfRight ("sample", 1, "inflated", "./MNE-sample-data/subjects");
    //Annotation tAnnotRight ("sample", 1, "aparc.a2009s", "./MNE-sample-data/subjects");
    //Surface tSurfLeft ("sample", 0, "inflated", "./MNE-sample-data/subjects");
    //Annotation tAnnotLeft ("sample", 0, "aparc.a2009s", "./MNE-sample-data/subjects");

    View3D::SPtr testWindow = View3D::SPtr(new View3D());
    //testWindow->addBrainData("HemiLR", tSurfLeft, tAnnotLeft);
    //testWindow->addBrainData("HemiLR", tSurfRight, tAnnotRight);
    testWindow->addBrainData("HemiLRSet", tSurfSet, tAnnotSet);

//    QFile t_File("./MNE-sample-data/MEG/sample/sample_audvis-meg-eeg-oct-6-fwd.fif");
//    MNEForwardSolution t_forwardSolution(t_File);
    //testWindow->addBrainData("HemiLRSet", t_forwardSolution);

    QList<BrainRTDataTreeItem*> rtItemList = testWindow->addRtBrainData("HemiLRSet", sourceEstimate, t_clusteredFwd);

    //testWindow->addRtBrainData("HemiLRSet", sourceEstimate);
    //rtItemList.at(0)->addData(sourceEstimate);

    testWindow->show();    

    Control3DWidget::SPtr control3DWidget = Control3DWidget::SPtr(new Control3DWidget());
    control3DWidget->setWindowFlags(Qt::WindowStaysOnTopHint);
    control3DWidget->setView3D(testWindow);
    control3DWidget->show();

    //########################################################################################
    //
    // Create the test view END
    //
    //########################################################################################

    return a.exec();
}
