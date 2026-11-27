//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     July, 2012
 * @brief    Cluster the forward solution by cortical parcellation and compute an MNE, dSPM or sLORETA estimate of an evoked response.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/fs/fs_label.h>
#include <mne/fs/fs_surface.h>
#include <mne/fs/fs_surfaceset.h>
#include <mne/fs/fs_annotationset.h>
#include <mne/fiff/fiff_evoked.h>
#include <mne/inv/inv_source_estimate.h>
#include <mne/inv/minimum_norm/inv_minimum_norm.h>
#include <mne/viz/backends/qt/3d/view/brainview.h>
#include <mne/viz/backends/qt/3d/model/braintreemodel.h>
#include <mne/math/linalg.h>
#include <mne/base/log.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QSet>
#include <QVector3D>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <iostream>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;
using namespace mne::viz;
using namespace mne::viz::qt;

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
int main(int argc, char* argv[])
{
#ifdef STATICBUILD
// Q_INIT_RESOURCE(mne_disp3d);
#endif
    QApplication app(argc, argv);

    // Command Line Parser
    QCommandLineParser parser;
    parser.setApplicationDescription("Clustered Inverse MNE Example");
    parser.addHelpOption();

    QCommandLineOption sampleFwdFileOption("fwd", "Path to the forward solution <file>.", "file", QCoreApplication::applicationDirPath() + "/../resources/data/MNE-sample-data/MEG/sample/sample_audvis-meg-eeg-oct-6-fwd.fif");
    QCommandLineOption sampleCovFileOption("cov", "Path to the covariance <file>.", "file", QCoreApplication::applicationDirPath() + "/../resources/data/MNE-sample-data/MEG/sample/sample_audvis-cov.fif");
    QCommandLineOption sampleEvokedFileOption("ave", "Path to the evoked/average <file>.", "file", QCoreApplication::applicationDirPath() + "/../resources/data/MNE-sample-data/MEG/sample/sample_audvis-ave.fif");
    QCommandLineOption snrOption("snr", "The <snr> value used for computation.", "snr", "1.0");                                    //3.0;//0.1;//3.0;
    QCommandLineOption methodOption("method", "Inverse estimation <method>, i.e., 'MNE', 'dSPM' or 'sLORETA'.", "method", "dSPM"); //"MNE" | "dSPM" | "sLORETA"
    QCommandLineOption invFileOption("invOut", "Path to inverse <file>, which is to be written.", "file", "");
    QCommandLineOption stcFileOption("stcOut", "Path to stc <file>, which is to be written.", "file", "");
    QCommandLineOption surfOption("surfType", "FsSurface type <type>.", "type", "orig");
    QCommandLineOption annotOption("annotType", "FsAnnotation type <type>.", "type", "aparc.a2009s");
    QCommandLineOption hemiOption("hemi", "Selected hemisphere <hemi>.", "hemi", "2");
    QCommandLineOption subjectOption("subject", "Selected subject <subject>.", "subject", "sample");
    QCommandLineOption subjectPathOption("subjectPath", "Selected subject path <subjectPath>.", "subjectPath", QCoreApplication::applicationDirPath() + "/../resources/data/MNE-sample-data/subjects");

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

    double snr = parser.value(snrOption).toDouble();
    QString method = parser.value(methodOption);

    QString t_sFileNameClusteredInv = parser.value(invFileOption);
    QString t_sFileNameStc = parser.value(stcFileOption);

    double lambda2 = 1.0 / pow(snr, 2);

    FsSurfaceSet t_surfSet(parser.value(subjectOption).toStdString(), parser.value(hemiOption).toInt(), parser.value(surfOption).toStdString(), parser.value(subjectPathOption).toStdU16String());
    FsAnnotationSet t_annotationSet(parser.value(subjectOption).toStdString(), parser.value(hemiOption).toInt(), parser.value(annotOption).toStdString(), parser.value(subjectPathOption).toStdU16String());

    Log::debug("Start calculation with: SNR {} ; Lambda {} ; Method {} ; stc: {}", snr, lambda2, method.toStdString(), t_sFileNameStc.toStdString());

    // Load data
    FiffEvoked evoked(parser.value(sampleEvokedFileOption).toStdU16String(), 0);

    std::cout << "evoked first " << evoked.first << "; last " << evoked.last << std::endl;

    MNEForwardSolution t_Fwd(parser.value(sampleFwdFileOption).toStdU16String());
    if (t_Fwd.empty())
        return 1;

    FiffCov noise_cov(parser.value(sampleCovFileOption).toStdString());

    // regularize noise covariance
    noise_cov = noise_cov.regularize(evoked.info, 0.05, 0.05, 0.1, true);

    //
    // Cluster forward solution;
    //
    MNEForwardSolution t_clusteredFwd = t_Fwd.cluster_forward_solution(t_annotationSet, 20); //40);

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
    if (!t_sFileNameClusteredInv.isEmpty()) {
        if (auto written = inverse_operator.write(t_sFileNameClusteredInv.toStdU16String()); !written) {
            Log::warning("{}", written.error().what());
        }
    }

    //
    // Compute inverse solution
    //
    InvMinimumNorm minimumNorm(inverse_operator, lambda2, method.toStdString());
    InvSourceEstimate sourceEstimate = minimumNorm.calculateInverse(evoked);

    if (sourceEstimate.empty())
        return 1;

    // View activation time-series
    std::cout << "\nsourceEstimate:\n"
              << sourceEstimate.data.block(0, 0, 10, 10) << std::endl;
    std::cout << "time\n"
              << sourceEstimate.times.block(0, 0, 1, 10) << std::endl;
    std::cout << "timeMin\n"
              << sourceEstimate.times[0] << std::endl;
    std::cout << "timeMax\n"
              << sourceEstimate.times[sourceEstimate.times.size() - 1] << std::endl;
    std::cout << "time step\n"
              << sourceEstimate.tstep << std::endl;

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

    double t_dConditionNumber = Linalg::getConditionNumber(t_Fwd.sol.data, s);
    double t_dConditionNumberClustered = Linalg::getConditionNumber(t_clusteredFwd.sol.data, s);

    std::cout << "Condition Number:\n"
              << t_dConditionNumber << std::endl;
    std::cout << "Clustered Condition Number:\n"
              << t_dConditionNumberClustered << std::endl;

    std::cout << "ForwardSolution" << t_Fwd.sol.data.block(0, 0, 10, 10) << std::endl;

    std::cout << "Clustered ForwardSolution" << t_clusteredFwd.sol.data.block(0, 0, 10, 10) << std::endl;

    //    double t_dConditionNumberMags = Linalg::getConditionNumber(mags, s);
    //    double t_dConditionNumberMagsClustered = Linalg::getConditionNumber(magsClustered, s);

    //    std::cout << "Condition Number Magnetometers:\n" << t_dConditionNumberMags << std::endl;
    //    std::cout << "Clustered Condition Number Magnetometers:\n" << t_dConditionNumberMagsClustered << std::endl;

    //    double t_dConditionNumberGrads = Linalg::getConditionNumber(grads, s);
    //    double t_dConditionNumberGradsClustered = Linalg::getConditionNumber(gradsClustered, s);

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

    // Write source estimate to temp files for visualization
    int nVertLh = t_clusteredFwd.src[0].nuse;
    InvSourceEstimate stcLh, stcRh;
    stcLh.data = sourceEstimate.data.topRows(nVertLh);
    stcLh.vertices = sourceEstimate.vertices.head(nVertLh);
    stcLh.tmin = sourceEstimate.tmin;
    stcLh.tstep = sourceEstimate.tstep;
    stcLh.times = sourceEstimate.times;
    stcRh.data = sourceEstimate.data.bottomRows(sourceEstimate.data.rows() - nVertLh);
    stcRh.vertices = sourceEstimate.vertices.tail(sourceEstimate.vertices.size() - nVertLh);
    stcRh.tmin = sourceEstimate.tmin;
    stcRh.tstep = sourceEstimate.tstep;
    stcRh.times = sourceEstimate.times;

    QString tmpDir = QDir::tempPath();
    QString lhStcPath = tmpDir + "/mnecpp_clustered_inv_mne-lh.stc";
    QString rhStcPath = tmpDir + "/mnecpp_clustered_inv_mne-rh.stc";
    if (auto written = stcLh.write(lhStcPath.toStdU16String()); !written) {
        Log::warning("{}", written.error().what());
    }
    if (auto written = stcRh.write(rhStcPath.toStdU16String()); !written) {
        Log::warning("{}", written.error().what());
    }

    BrainView* pBrainView = new BrainView();
    BrainTreeModel* pModel = new BrainTreeModel();
    pBrainView->setModel(pModel);

    // Add hemisphere surfaces
    for (auto it = t_surfSet.data().cbegin(); it != t_surfSet.data().cend(); ++it) {
        int hIdx = it->first;
        QString hemi = (it->second.hemi() == 0) ? "lh" : "rh";
        QString surfType = QString::fromStdString(it->second.surf().empty() ? "inflated" : it->second.surf());
        pModel->addSurface(parser.value(subjectOption), hemi, surfType, it->second);
        if (t_annotationSet.size() > hIdx)
            pModel->addAnnotation(parser.value(subjectOption), hemi, t_annotationSet[hIdx]);
    }

    // Load source estimate and start streaming when loaded
    QObject::connect(pBrainView, &BrainView::sourceEstimateLoaded, [&](int /*nTimePoints*/) {
        pBrainView->setSourceColormap("Hot");
        pBrainView->setSourceThresholds(0.0f, 0.5f, 10.0f);
        pBrainView->setRealtimeLooping(true);
        pBrainView->setRealtimeInterval(17);
        pBrainView->startRealtimeStreaming();
    });
    pBrainView->loadSourceEstimate(lhStcPath.toStdU16String(), rhStcPath.toStdU16String());

    if (!t_sFileNameStc.isEmpty()) {
        if (auto written = sourceEstimate.write(t_sFileNameStc.toStdU16String()); !written) {
            Log::warning("{}", written.error().what());
        }
    }

    pBrainView->show();

    return app.exec(); //1;
}
