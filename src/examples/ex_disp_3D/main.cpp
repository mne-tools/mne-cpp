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

#include <disp3D/viewers/abstractview.h>
#include <disp3D/engine/model/items/sourcedata/mnedatatreeitem.h>
#include <disp3D/engine/model/items/sensordata/sensordatatreeitem.h>
#include <disp3D/engine/model/items/digitizer/digitizersettreeitem.h>
#include <disp3D/engine/model/items/bem/bemtreeitem.h>
#include <disp3D/engine/model/items/freesurfer/fssurfacetreeitem.h>
#include <disp3D/engine/model/data3Dtreemodel.h>
#include <disp3D/engine/view/view3D.h>
#include <disp3D/helpers/geometryinfo/geometryinfo.h>
#include <disp3D/helpers/interpolation/interpolation.h>

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
#include <QMainWindow>
#include <QCommandLineParser>
#include <QVector3D>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISP3DLIB;
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
    AbstractView::SPtr p3DAbstractView = AbstractView::SPtr(new AbstractView());
    Data3DTreeModel::SPtr p3DDataModel = p3DAbstractView->getTreeModel();

    //Add fressurfer surface set including both hemispheres
    QList<FsSurfaceTreeItem*> lFsSurfaces = p3DDataModel->addSurfaceSet(parser.value(subjectOption),
                                                                        "MRI",
                                                                        tSurfSet,
                                                                        tAnnotSet);

    for(int i = 0; i < lFsSurfaces.size(); ++i) {
        lFsSurfaces.at(i)->setTransform(evoked.info.dev_head_t, true);
    }

    //Read and show BEM
    QFile t_fileBem(QCoreApplication::applicationDirPath() + "/../resources/data/MNE-sample-data/subjects/sample/bem/sample-5120-5120-5120-bem.fif");
    MNEBem t_Bem(t_fileBem);
    p3DDataModel->addBemData(parser.value(subjectOption), "BEM", t_Bem)->setTransform(evoked.info.dev_head_t, true);

    //Read and show sensor helmets
    QFile t_filesensorSurfaceVV(QCoreApplication::applicationDirPath() + "/../resources/general/sensorSurfaces/306m_rt.fif");
    MNEBem t_sensorSurfaceVV(t_filesensorSurfaceVV);
    p3DDataModel->addMegSensorInfo("Sensors", "VectorView", evoked.info.chs, t_sensorSurfaceVV, evoked.info.bads);

    // Read, co-register and show digitizer points
    QFile t_fileDig(QCoreApplication::applicationDirPath() + "/../resources/data/MNE-sample-data/MEG/sample/sample_audvis-ave.fif");
    FiffDigPointSet t_Dig(t_fileDig);

    QFile coordTransfile(QCoreApplication::applicationDirPath() + "/../resources/data/MNE-sample-data/MEG/sample/all-trans.fif");
    FiffCoordTrans coordTrans(coordTransfile);

    DigitizerSetTreeItem* pDigitizerSetTreeItem = p3DDataModel->addDigitizerData(parser.value(subjectOption), evoked.comment, t_Dig);
    pDigitizerSetTreeItem->setTransform(coordTrans, true);
    pDigitizerSetTreeItem->applyTransform(evoked.info.dev_head_t, true);

    //add sensor item for MEG data
    if (SensorDataTreeItem* pMegSensorTreeItem = p3DDataModel->addSensorData(parser.value(subjectOption),
                                                                             evoked.comment,
                                                                             evoked.data,
                                                                             t_sensorSurfaceVV[0],
                                                                             evoked.info,
                                                                             "MEG")) {
        pMegSensorTreeItem->setLoopState(true);
        pMegSensorTreeItem->setTimeInterval(17);
        pMegSensorTreeItem->setNumberAverages(1);
        pMegSensorTreeItem->setStreamingState(false);
        pMegSensorTreeItem->setThresholds(QVector3D(0.0f, 3e-12f*0.5f, 3e-12f));
        pMegSensorTreeItem->setColormapType("Jet");
        pMegSensorTreeItem->setSFreq(evoked.info.sfreq);
    }

    //add sensor item for EEG data
    //Co-Register EEG points in order to correctly map them to the scalp
    for(int i = 0; i < evoked.info.chs.size(); ++i) {
        if(evoked.info.chs.at(i).kind == FIFFV_EEG_CH) {
            Vector4f tempvec;
            tempvec(0) = evoked.info.chs.at(i).chpos.r0(0);
            tempvec(1) = evoked.info.chs.at(i).chpos.r0(1);
            tempvec(2) = evoked.info.chs.at(i).chpos.r0(2);
            tempvec(3) = 1;
            tempvec = coordTrans.invtrans * tempvec;
            evoked.info.chs[i].chpos.r0(0) = tempvec(0);
            evoked.info.chs[i].chpos.r0(1) = tempvec(1);
            evoked.info.chs[i].chpos.r0(2) = tempvec(2);
        }
    }

    if (SensorDataTreeItem* pEegSensorTreeItem = p3DDataModel->addSensorData(parser.value(subjectOption),
                                                                             evoked.comment,
                                                                             evoked.data,
                                                                             t_Bem[0],
                                                                             evoked.info,
                                                                             "EEG")) {
        pEegSensorTreeItem->setLoopState(true);
        pEegSensorTreeItem->setTimeInterval(17);
        pEegSensorTreeItem->setNumberAverages(1);
        pEegSensorTreeItem->setStreamingState(false);
        pEegSensorTreeItem->setThresholds(QVector3D(0.0f, 6.0e-6f*0.5f, 6.0e-6f));
        pEegSensorTreeItem->setColormapType("Jet");
        pEegSensorTreeItem->setSFreq(evoked.info.sfreq);
        pEegSensorTreeItem->setTransform(evoked.info.dev_head_t, true);
    }

    if(bAddRtSourceLoc) {
        //Add rt source loc data and init some visualization values
        if(MneDataTreeItem* pRTDataItem = p3DDataModel->addSourceData(parser.value(subjectOption),
                                                                      evoked.comment,
                                                                      sourceEstimate,
                                                                      t_clusteredFwd,
                                                                      tSurfSet,
                                                                      tAnnotSet)) {
            pRTDataItem->setLoopState(true);
            pRTDataItem->setTimeInterval(17);
            pRTDataItem->setNumberAverages(1);
            pRTDataItem->setAlpha(1.0);
            pRTDataItem->setStreamingState(false);
            pRTDataItem->setThresholds(QVector3D(0.0f,0.5f,10.0f));
            pRTDataItem->setVisualizationType("Annotation based");
            pRTDataItem->setColormapType("Jet");
            pRTDataItem->setTransform(evoked.info.dev_head_t, true);
        }
    }

    p3DAbstractView->show();

    return a.exec();
}
