//=============================================================================================================
/**
 * @file     main.cpp
 * @author   Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Felix Griesau <Felix.Griesau@tu-ilmenau.de>;
 *           Juan Garcia-Prieto <juangpc@gmail.com>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Simon Heinke <Simon.Heinke@tu-ilmenau.de>
 * @since    0.1.0
 * @date     July, 2016
 *
 * @section  LICENSE
 *
 * Copyright (C) 2016, Lars Debor, Christoph Dinh, Felix Griesau, Juan Garcia-Prieto, Lorenz Esch,
 *                     Simon Heinke. All rights reserved.
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

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <disp3D_rhi/view/brainview.h>
#include <disp3D_rhi/model/braintreemodel.h>
#include <disp3D_rhi/model/items/digitizersettreeitem.h>
#include <disp3D_rhi/model/items/bemtreeitem.h>

#include <fs/surfaceset.h>
#include <fs/annotationset.h>

#include <mne/mne_sourceestimate.h>
#include <mne/mne_bem.h>

#include <fiff/fiff_dig_point_set.h>

#include <inverse/minimumNorm/minimumnorm.h>

#include <utils/generics/applicationlogger.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QApplication>
#include <QDir>
#include <QMainWindow>
#include <QCommandLineParser>
#include <QVector3D>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <iostream>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNELIB;
using namespace FSLIB;
using namespace FIFFLIB;
using namespace INVERSELIB;
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
    // Q_INIT_RESOURCE(mne_disp3d);
    #endif
    
    qInstallMessageHandler(ApplicationLogger::customLogWriter);
    QApplication a(argc, argv);

    // Command Line Parser
    QCommandLineParser parser;
    parser.setApplicationDescription("Disp3D Example");
    parser.addHelpOption();

    QCommandLineOption surfOption("surfType", "Surface type <type>.", "type", "pial");
    QCommandLineOption annotOption("annotType", "Annotation type <type>.", "type", "aparc.a2009s");
    QCommandLineOption hemiOption("hemi", "Selected hemisphere <hemi>.", "hemi", "2");
    QCommandLineOption subjectOption("subject", "Selected subject <subject>.", "subject", "sample");
    QCommandLineOption subjectPathOption("subjectPath", "Selected subject path <subjectPath>.", "subjectPath", QCoreApplication::applicationDirPath() + "/../resources/data/MNE-sample-data/subjects");
    QCommandLineOption sourceLocOption("doSourceLoc", "Do real time source localization.", "doSourceLoc", "true");
    QCommandLineOption fwdOption("fwd", "Path to forwad solution <file>.", "file", QCoreApplication::applicationDirPath() + "/../resources/data/MNE-sample-data/MEG/sample/sample_audvis-meg-eeg-oct-6-fwd.fif");
    QCommandLineOption invOpOption("inv", "Path to inverse operator <file>.", "file", "");
    QCommandLineOption clustOption("doClust", "Path to clustered inverse operator <doClust>.", "doClust", "true");
    QCommandLineOption covFileOption("cov", "Path to the covariance <file>.", "file", QCoreApplication::applicationDirPath() + "/../resources/data/MNE-sample-data/MEG/sample/sample_audvis-cov.fif");
    QCommandLineOption evokedFileOption("ave", "Path to the evoked/average <file>.", "file", QCoreApplication::applicationDirPath() + "/../resources/data/MNE-sample-data/MEG/sample/sample_audvis-ave.fif");
    QCommandLineOption methodOption("method", "Inverse estimation <method>, i.e., 'MNE', 'dSPM' or 'sLORETA'.", "method", "dSPM");//"MNE" | "dSPM" | "sLORETA"
    QCommandLineOption snrOption("snr", "The SNR value used for computation <snr>.", "snr", "3.0");//3.0;//0.1;//3.0;
    QCommandLineOption evokedIndexOption("aveIdx", "The average <index> to choose from the average file.", "index", "1");

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

    bool bAddRtSourceLoc = false;
    if(parser.value(sourceLocOption) == "false" || parser.value(sourceLocOption) == "0") {
        bAddRtSourceLoc = false;
    } else if(parser.value(sourceLocOption) == "true" || parser.value(sourceLocOption) == "1") {
        bAddRtSourceLoc = true;
    }

    bool bDoClustering = false;
    if(parser.value(clustOption) == "false" || parser.value(clustOption) == "0") {
        bDoClustering = false;
    } else if(parser.value(clustOption) == "true" || parser.value(clustOption) == "1") {
        bDoClustering = true;
    }

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
    QPair<float, float> baseline(-1.0f, -1.0f);
    MNESourceEstimate sourceEstimate;
    FiffEvoked evoked(t_fileEvoked, parser.value(evokedIndexOption).toInt(), baseline);

    if(bAddRtSourceLoc) {
        double snr = parser.value(snrOption).toDouble();
        double lambda2 = 1.0 / pow(snr, 2);
        QString method(parser.value(methodOption));

        // Load data
        t_fileEvoked.close();
        if(evoked.isEmpty())
            return 1;

        std::cout << std::endl;
        std::cout << "Evoked description: " << evoked.comment.toUtf8().constData() << std::endl;

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
    //Source Estimate END
    //
    //########################################################################################

    //Create 3D data model
    BrainView *pBrainView = new BrainView();
    BrainTreeModel *pModel = new BrainTreeModel();
    pBrainView->setModel(pModel);

    //Add fressurfer surface set including both hemispheres
    for (auto it = tSurfSet.data().constBegin(); it != tSurfSet.data().constEnd(); ++it) {
        int hIdx = it.key();
        QString hemi = (it.value().hemi() == 0) ? "lh" : "rh";
        QString surfType = it.value().surf().isEmpty() ? "inflated" : it.value().surf();
        pModel->addSurface(parser.value(subjectOption), hemi, surfType, it.value());
        if (tAnnotSet.size() > hIdx)
            pModel->addAnnotation(parser.value(subjectOption), hemi, tAnnotSet[hIdx]);
    }

    //Read and show BEM
    QFile t_fileBem(QCoreApplication::applicationDirPath() + "/../resources/data/MNE-sample-data/subjects/sample/bem/sample-5120-5120-5120-bem.fif");
    MNEBem t_Bem(t_fileBem);
    for (int i = 0; i < t_Bem.size(); ++i) {
        pModel->addBemSurface(parser.value(subjectOption), "BEM", t_Bem[i]);
    }

    //Read and show sensor helmets
    pBrainView->loadSensors(QCoreApplication::applicationDirPath() + "/../resources/general/sensorSurfaces/306m_rt.fif");

    // Read and show digitizer points
    QFile t_fileDig(QCoreApplication::applicationDirPath() + "/../resources/data/MNE-sample-data/MEG/sample/sample_audvis-ave.fif");
    FiffDigPointSet t_Dig(t_fileDig);

    QFile coordTransfile(QCoreApplication::applicationDirPath() + "/../resources/data/MNE-sample-data/MEG/sample/all-trans.fif");
    FiffCoordTrans coordTrans(coordTransfile);

    pModel->addDigitizerData(t_Dig.getList());

    // Load sensor field data from evoked file for MEG/EEG field mapping
    pBrainView->loadSensorField(parser.value(evokedFileOption), parser.value(evokedIndexOption).toInt());

    if(bAddRtSourceLoc) {
        // Write source estimate to temp files for visualization
        int nVertLh = t_clusteredFwd.src[0].nuse;
        MNESourceEstimate stcLh, stcRh;
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
        QString lhStcPath = tmpDir + "/mnecpp_disp3d-lh.stc";
        QString rhStcPath = tmpDir + "/mnecpp_disp3d-rh.stc";
        QFile lhStcFile(lhStcPath);
        stcLh.write(lhStcFile);
        QFile rhStcFile(rhStcPath);
        stcRh.write(rhStcFile);

        // Load source estimate and configure visualization
        QObject::connect(pBrainView, &BrainView::sourceEstimateLoaded, [&](int /*nTimePoints*/) {
            pBrainView->setSourceColormap("Jet");
            pBrainView->setSourceThresholds(0.0f, 0.5f, 10.0f);
        });
        pBrainView->loadSourceEstimate(lhStcPath, rhStcPath);
    }

    pBrainView->show();

    return a.exec();
}
