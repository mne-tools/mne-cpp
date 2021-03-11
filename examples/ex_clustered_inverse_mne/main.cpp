//=============================================================================================================
/**
 * @file     main.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Juan Garcia-Prieto <juangpc@gmail.com>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2012
 *
 * @section  LICENSE
 *
 * Copyright (C) 2012, Christoph Dinh, Juan Garcia-Prieto, Lorenz Esch. All rights reserved.
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
 * @brief    Example of a Clustered Inverse Minimum Norm Estimation application
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fs/label.h>
#include <fs/surface.h>
#include <fs/surfaceset.h>
#include <fs/annotationset.h>

#include <fiff/fiff_evoked.h>
#include <mne/mne_sourceestimate.h>
#include <inverse/minimumNorm/minimumnorm.h>

#include <disp3D/viewers/abstractview.h>
#include <disp3D/engine/model/data3Dtreemodel.h>
#include <disp3D/engine/view/view3D.h>
#include <disp3D/engine/model/items/sourcedata/mnedatatreeitem.h>

#include <utils/mnemath.h>
#include <utils/generics/applicationlogger.h>

#include <iostream>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QApplication>
#include <QCommandLineParser>
#include <QSet>
#include <QVector3D>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNELIB;
using namespace FSLIB;
using namespace FIFFLIB;
using namespace INVERSELIB;
using namespace DISP3DLIB;
using namespace UTILSLIB;
using namespace Eigen;

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
    QApplication app(argc, argv);

    // Command Line Parser
    QCommandLineParser parser;
    parser.setApplicationDescription("Clustered Inverse MNE Example");
    parser.addHelpOption();

    QCommandLineOption sampleFwdFileOption("fwd", "Path to the forward solution <file>.", "file", QCoreApplication::applicationDirPath() + "/MNE-sample-data/MEG/sample/sample_audvis-meg-eeg-oct-6-fwd.fif");
    QCommandLineOption sampleCovFileOption("cov", "Path to the covariance <file>.", "file", QCoreApplication::applicationDirPath() + "/MNE-sample-data/MEG/sample/sample_audvis-cov.fif");
    QCommandLineOption sampleEvokedFileOption("ave", "Path to the evoked/average <file>.", "file", QCoreApplication::applicationDirPath() + "/MNE-sample-data/MEG/sample/sample_audvis-ave.fif");
    QCommandLineOption snrOption("snr", "The <snr> value used for computation.", "snr", "1.0");//3.0;//0.1;//3.0;
    QCommandLineOption methodOption("method", "Inverse estimation <method>, i.e., 'MNE', 'dSPM' or 'sLORETA'.", "method", "dSPM");//"MNE" | "dSPM" | "sLORETA"
    QCommandLineOption invFileOption("invOut", "Path to inverse <file>, which is to be written.", "file", "");
    QCommandLineOption stcFileOption("stcOut", "Path to stc <file>, which is to be written.", "file", "");
    QCommandLineOption surfOption("surfType", "Surface type <type>.", "type", "orig");
    QCommandLineOption annotOption("annotType", "Annotation type <type>.", "type", "aparc.a2009s");
    QCommandLineOption hemiOption("hemi", "Selected hemisphere <hemi>.", "hemi", "2");
    QCommandLineOption subjectOption("subject", "Selected subject <subject>.", "subject", "sample");
    QCommandLineOption subjectPathOption("subjectPath", "Selected subject path <subjectPath>.", "subjectPath", QCoreApplication::applicationDirPath() + "/MNE-sample-data/subjects");

    parser.addOption(sampleFwdFileOption);
    parser.addOption(sampleCovFileOption);
    parser.addOption(sampleEvokedFileOption);
    parser.addOption(snrOption);
    parser.addOption(methodOption);
    parser.addOption(invFileOption);
    parser.addOption(stcFileOption);
    parser.addOption(surfOption);
    parser.addOption(annotOption);
    parser.addOption(hemiOption);
    parser.addOption(subjectOption);
    parser.addOption(subjectPathOption);

    parser.process(app);

    //Parse inputs
    QFile t_fileFwd(parser.value(sampleFwdFileOption));
    QFile t_fileCov(parser.value(sampleCovFileOption));
    QFile t_fileEvoked(parser.value(sampleEvokedFileOption));

    double snr = parser.value(snrOption).toDouble();
    QString method = parser.value(methodOption);

    QString t_sFileNameClusteredInv = parser.value(invFileOption);
    QString t_sFileNameStc = parser.value(stcFileOption);

    double lambda2 = 1.0 / pow(snr, 2);

    SurfaceSet t_surfSet (parser.value(subjectOption), parser.value(hemiOption).toInt(), parser.value(surfOption), parser.value(subjectPathOption));
    AnnotationSet t_annotationSet (parser.value(subjectOption), parser.value(hemiOption).toInt(), parser.value(annotOption), parser.value(subjectPathOption));

    qDebug() << "Start calculation with: SNR" << snr << "; Lambda" << lambda2 << "; Method" << method << "; stc:" << t_sFileNameStc;

    // Load data
    fiff_int_t setno = 0;
    QPair<float, float> baseline(-1.0f, -1.0f);
    FiffEvoked evoked(t_fileEvoked, setno, baseline);
    if(evoked.isEmpty())
        return 1;

    std::cout << "evoked first " << evoked.first << "; last " << evoked.last << std::endl;

    MNEForwardSolution t_Fwd(t_fileFwd);
    if(t_Fwd.isEmpty())
        return 1;

    FiffCov noise_cov(t_fileCov);

    // regularize noise covariance
    noise_cov = noise_cov.regularize(evoked.info, 0.05, 0.05, 0.1, true);

    //
    // Cluster forward solution;
    //
    MNEForwardSolution t_clusteredFwd = t_Fwd.cluster_forward_solution(t_annotationSet, 20);//40);

//    std::cout << "Size " << t_clusteredFwd.sol->data.rows() << " x " << t_clusteredFwd.sol->data.cols() << std::endl;
//    std::cout << "Clustered Fwd:\n" << t_clusteredFwd.sol->data.row(0) << std::endl;

    //
    // make an inverse operators
    //
    FiffInfo info = evoked.info;

    MNEInverseOperator inverse_operator(info, t_clusteredFwd, noise_cov, 0.2f, 0.8f);

    //
    // save clustered inverse
    //
    if(!t_sFileNameClusteredInv.isEmpty())
    {
        QFile t_fileClusteredInverse(t_sFileNameClusteredInv);
        inverse_operator.write(t_fileClusteredInverse);
    }

    //
    // Compute inverse solution
    //
    MinimumNorm minimumNorm(inverse_operator, lambda2, method);
    MNESourceEstimate sourceEstimate = minimumNorm.calculateInverse(evoked);

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

//    //only one time point - P100
//    qint32 sample = 0;
//    for(qint32 i = 0; i < sourceEstimate.times.size(); ++i)
//    {
//        if(sourceEstimate.times(i) >= 0)
//        {
//            sample = i;
//            break;
//        }
//    }
//    sample += (qint32)ceil(0.106/sourceEstimate.tstep); //100ms
//    sourceEstimate = sourceEstimate.reduce(sample, 1);

    AbstractView::SPtr p3DAbstractView = AbstractView::SPtr(new AbstractView());
    Data3DTreeModel::SPtr p3DDataModel = p3DAbstractView->getTreeModel();

    p3DDataModel->addSurfaceSet(parser.value(subjectOption), "MRI", t_surfSet, t_annotationSet);

    if(MneDataTreeItem* pRTDataItem = p3DDataModel->addSourceData(parser.value(subjectOption),
                                                                  evoked.comment,
                                                                  sourceEstimate,
                                                                  t_clusteredFwd,
                                                                  t_surfSet,
                                                                  t_annotationSet)) {
        pRTDataItem->setLoopState(true);
        pRTDataItem->setTimeInterval(17);
        pRTDataItem->setNumberAverages(1);
        pRTDataItem->setStreamingState(true);
        pRTDataItem->setThresholds(QVector3D(0.0,0.5,10.0));
        pRTDataItem->setVisualizationType("Interpolation based");
        pRTDataItem->setColormapType("Hot");
    }

    if(!t_sFileNameStc.isEmpty())
    {
        QFile t_fileClusteredStc(t_sFileNameStc);
        sourceEstimate.write(t_fileClusteredStc);
    }

    p3DAbstractView->show();

    return app.exec();//1;
}
