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
*     * Neither the name of the Massachusetts General Hospital nor the names of its contributors may be used
*       to endorse or promote products derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MASSACHUSETTS GENERAL HOSPITAL BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*
* @brief    Implements the main() application function.
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fs/label.h>
#include <fs/surface.h>
#include <fs/annotationset.h>

#include <fiff/fiff_evoked.h>
#include <inverse/sourceestimate.h>
#include <inverse/minimumNorm/minimumnorm.h>

#include <disp3D/inverseview.h>

#include <utils/mnemath.h>

#include <iostream>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QGuiApplication>
#include <QSet>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNELIB;
using namespace FSLIB;
using namespace FIFFLIB;
using namespace INVERSELIB;
using namespace DISP3DLIB;
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
    QGuiApplication a(argc, argv);

    //########################################################################################
    // Source Estimate

//    QFile t_fileFwd("./MNE-sample-data/MEG/sample/sample_audvis-meg-eeg-oct-6-fwd.fif");
//    QFile t_fileCov("./MNE-sample-data/MEG/sample/sample_audvis-cov.fif");
//    QFile t_fileEvoked("./MNE-sample-data/MEG/sample/sample_audvis-ave.fif");

//    QFile t_fileClusteredInverse("./clusteredInverse-inv.fif");

    QFile t_fileFwd("/home/chdinh/sl_data/MEG/mind006/mind006_051209_auditory01_raw-oct-6p-fwd.fif");
    QFile t_fileCov("/home/chdinh/sl_data/MEG/mind006/mind006_051209_auditory01_raw-cov.fif");
    QFile t_fileEvoked("/home/chdinh/sl_data/MEG/mind006/mind006_051209_auditory01_raw-ave.fif");

    QFile t_fileClusteredInverse("./clusteredInverse-inv.fif");

    double snr = 3.0;
    double lambda2 = 1.0 / pow(snr, 2);
    QString method("dSPM"); //"MNE" | "dSPM" | "sLORETA"

    // Load data
    fiff_int_t setno = 0;
    QPair<QVariant, QVariant> baseline(QVariant(), 0);
    FiffEvoked evoked(t_fileEvoked, setno, baseline);
    if(evoked.isEmpty())
        return 1;

    MNEForwardSolution t_Fwd(t_fileFwd);
    if(t_Fwd.isEmpty())
        return 1;

//    AnnotationSet t_annotationSet("./MNE-sample-data/subjects/sample/label/lh.aparc.a2009s.annot", "./MNE-sample-data/subjects/sample/label/rh.aparc.a2009s.annot");
    AnnotationSet t_annotationSet("/home/chdinh/sl_data/subjects/mind006/label/lh.aparc.a2009s.annot", "/home/chdinh/sl_data/subjects/mind006/label/rh.aparc.a2009s.annot");


    FiffCov noise_cov(t_fileCov);

    // regularize noise covariance
    noise_cov = noise_cov.regularize(evoked.info, 0.05, 0.05, 0.1, true);

    //
    // Cluster forward solution;
    //
    MNEForwardSolution t_clusteredFwd = t_Fwd.cluster_forward_solution(t_annotationSet, 40);
/*
    //
    // make an inverse operators
    //
    FiffInfo info = evoked.info;

    MNEInverseOperator inverse_operator(info, t_clusteredFwd, noise_cov, 0.2f, 0.8f);

    inverse_operator.write(t_fileClusteredInverse);

    //
    // Compute inverse solution
    //
    MinimumNorm minimumNorm(inverse_operator, lambda2, method);
    SourceEstimate sourceEstimate = minimumNorm.calculateInverse(evoked);

    if(sourceEstimate.isEmpty())
        return 1;

    // View activation time-series
    std::cout << "\nsourceEstimate:\n" << sourceEstimate.data.block(0,0,10,10) << std::endl;
    std::cout << "time\n" << sourceEstimate.times.block(0,0,1,10) << std::endl;
    std::cout << "timeMin\n" << sourceEstimate.times[0] << std::endl;
    std::cout << "timeMax\n" << sourceEstimate.times[sourceEstimate.times.size()-1] << std::endl;
    std::cout << "time step\n" << sourceEstimate.tstep << std::endl;

    //Condition Numbers
//    MatrixXd mags(102, t_Fwd.sol->data.cols());
//    qint32 count = 0;
//    for(qint32 i = 2; i < 306; i += 3)
//    {
//        mags.row(count) = t_Fwd.sol->data.row(i);
//        ++count;
//    }
//    MatrixXd magsClustered(102, t_clusteredFwd.sol->data.cols());
//    count = 0;
//    for(qint32 i = 2; i < 306; i += 3)
//    {
//        magsClustered.row(count) = t_clusteredFwd.sol->data.row(i);
//        ++count;
//    }

//    MatrixXd grads(204, t_Fwd.sol->data.cols());
//    count = 0;
//    for(qint32 i = 0; i < 306; i += 3)
//    {
//        grads.row(count) = t_Fwd.sol->data.row(i);
//        ++count;
//        grads.row(count) = t_Fwd.sol->data.row(i+1);
//        ++count;
//    }
//    MatrixXd gradsClustered(204, t_clusteredFwd.sol->data.cols());
//    count = 0;
//    for(qint32 i = 0; i < 306; i += 3)
//    {
//        gradsClustered.row(count) = t_clusteredFwd.sol->data.row(i);
//        ++count;
//        gradsClustered.row(count) = t_clusteredFwd.sol->data.row(i+1);
//        ++count;
//    }

    VectorXd s;

    double t_dConditionNumber = MNEMath::getConditionNumber(t_Fwd.sol->data, s);
    double t_dConditionNumberClustered = MNEMath::getConditionNumber(t_clusteredFwd.sol->data, s);


    std::cout << "Condition Number:\n" << t_dConditionNumber << std::endl;
    std::cout << "Clustered Condition Number:\n" << t_dConditionNumberClustered << std::endl;

    std::cout << "ForwardSolution" << t_Fwd.sol->data.block(0,0,10,10) << std::endl;

    std::cout << "Clustered ForwardSolution" << t_clusteredFwd.sol->data.block(0,0,10,10) << std::endl;


//    double t_dConditionNumberMags = MNEMath::getConditionNumber(mags, s);
//    double t_dConditionNumberMagsClustered = MNEMath::getConditionNumber(magsClustered, s);

//    std::cout << "Condition Number Magnetometers:\n" << t_dConditionNumberMags << std::endl;
//    std::cout << "Clustered Condition Number Magnetometers:\n" << t_dConditionNumberMagsClustered << std::endl;

//    double t_dConditionNumberGrads = MNEMath::getConditionNumber(grads, s);
//    double t_dConditionNumberGradsClustered = MNEMath::getConditionNumber(gradsClustered, s);

//    std::cout << "Condition Number Gradiometers:\n" << t_dConditionNumberGrads << std::endl;
//    std::cout << "Clustered Condition Number Gradiometers:\n" << t_dConditionNumberGradsClustered << std::endl;


    //Source Estimate end
    //########################################################################################

//    AnnotationSet t_annotSet("./MNE-sample-data/subjects/sample/label/lh.aparc.a2009s.annot","./MNE-sample-data/subjects/sample/label/rh.aparc.a2009s.annot");
    AnnotationSet t_annotSet("/home/chdinh/sl_data/subjects/mind006/label/lh.aparc.a2009s.annot", "/home/chdinh/sl_data/subjects/mind006/label/rh.aparc.a2009s.annot");

//    SurfaceSet t_surfSet("./MNE-sample-data/subjects/sample/surf/lh.white", "./MNE-sample-data/subjects/sample/surf/rh.white");
    SurfaceSet t_surfSet("/home/chdinh/sl_data/subjects/mind006/surf/lh.white", "/home/chdinh/sl_data/subjects/mind006/surf/rh.white");


    QList<Label> t_qListLabels;
    QList<RowVector4i> t_qListRGBAs;

    //ToDo overload toLabels using instead of t_surfSet rr of MNESourceSpace
    t_annotSet.toLabels(t_surfSet, t_qListLabels, t_qListRGBAs);

    InverseView view(minimumNorm.getSourceSpace(), t_qListLabels, t_qListRGBAs);

    if (view.stereoType() != QGLView::RedCyanAnaglyph)
        view.camera()->setEyeSeparation(0.3f);
    QStringList args = QCoreApplication::arguments();
    int w_pos = args.indexOf("-width");
    int h_pos = args.indexOf("-height");
    if (w_pos >= 0 && h_pos >= 0)
    {
        bool ok = true;
        int w = args.at(w_pos + 1).toInt(&ok);
        if (!ok)
        {
            qWarning() << "Could not parse width argument:" << args;
            return 1;
        }
        int h = args.at(h_pos + 1).toInt(&ok);
        if (!ok)
        {
            qWarning() << "Could not parse height argument:" << args;
            return 1;
        }
        view.resize(w, h);
    }
    else
    {
        view.resize(800, 600);
    }
    view.show();

    //Push Estimate
    view.pushSourceEstimate(sourceEstimate);
*/
    return a.exec();
}
