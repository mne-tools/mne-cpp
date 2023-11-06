//=============================================================================================================
/**
 * @file     main.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Gabriel B Motta <gabrielbenmotta@gmail.com>;
 *           Daniel Strohmeier <Daniel.Strohmeier@tu-ilmenau.de>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Faris Yahya <mfarisyahya@gmail.com>
 * @since    0.1.0
 * @date     July, 2016
 *
 * @section  LICENSE
 *
 *
 *                      Copyright (C) 2016, Christoph Dinh, Gabriel B Motta, Daniel Strohmeier, Lorenz Esch, Faris Yahya. All rights reserved.
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

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "connectivitysettingsmanager.h"

#include <disp3D/viewers/networkview.h>
#include <disp3D/engine/model/data3Dtreemodel.h>
#include <disp3D/engine/model/items/network/networktreeitem.h>
#include <disp3D/engine/model/items/freesurfer/fssurfacetreeitem.h>
#include <disp3D/engine/model/items/sourcedata/mnedatatreeitem.h>
#include <disp3D/engine/model/items/digitizer/digitizersettreeitem.h>

#include <connectivity/connectivity.h>
#include <connectivity/connectivitysettings.h>
#include <connectivity/network/network.h>

#include <fiff/fiff_raw_data.h>

#include <fs/label.h>
#include <fs/annotationset.h>
#include <fs/surfaceset.h>

#include <mne/mne_sourceestimate.h>
#include <mne/mne_epoch_data_list.h>
#include <mne/mne.h>

#include <inverse/minimumNorm/minimumnorm.h>

#include <utils/ioutils.h>
#include <utils/generics/applicationlogger.h>

#include <disp/viewers/connectivitysettingsview.h>
#include <disp/viewers/minimumnormsettingsview.h>
#include <disp/viewers/tfsettingsview.h>

#include <disp3D/engine/model/items/network/networktreeitem.h>
#include <disp3D/engine/model/items/sensordata/sensordatatreeitem.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QApplication>
#include <QMainWindow>
#include <QCommandLineParser>
#include <QDebug>
#include <QFile>
#include <QObject>
#include <QVariant>
#include <Qt3DCore/QTransform>
#include <QMatrix4x4>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISP3DLIB;
using namespace DISPLIB;
using namespace INVERSELIB;
using namespace Eigen;
using namespace FIFFLIB;
using namespace CONNECTIVITYLIB;
using namespace Eigen;
using namespace UTILSLIB;
using namespace MNELIB;
using namespace FSLIB;

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
    QApplication::addLibraryPath(QApplication::applicationDirPath()+"/../lib");

    AbstractMetric::m_bStorageModeIsActive = false;
    AbstractMetric::m_iNumberBinStart = 0;
    AbstractMetric::m_iNumberBinAmount = 50;

    QCommandLineParser parser;
    parser.setApplicationDescription("Connectivity Example");
    parser.addHelpOption();

    QCommandLineOption rawFileOption("fileIn", "The input file <in>.", "in", QCoreApplication::applicationDirPath() + "/../resources/data/MNE-sample-data/MEG/sample/sample_audvis_raw.fif");
    QCommandLineOption eventsFileOption("eve", "Path to the event <file>.", "file", QCoreApplication::applicationDirPath() + "/../resources/data/MNE-sample-data/MEG/sample/sample_audvis_raw-eve.fif");
    QCommandLineOption fwdOption("fwd", "Path to forwad solution <file>.", "file", QCoreApplication::applicationDirPath() + "/../resources/data/MNE-sample-data/MEG/sample/sample_audvis-meg-eeg-oct-6-fwd.fif");
    QCommandLineOption subjectOption("subject", "Selected subject <subject>.", "subject", "sample");
    QCommandLineOption subjectPathOption("subjectPath", "Selected subject path <subjectPath>.", "subjectPath", QCoreApplication::applicationDirPath() + "/../resources/data/MNE-sample-data/subjects");
    QCommandLineOption covFileOption("cov", "Path to the covariance <file>.", "file", QCoreApplication::applicationDirPath() + "/../resources/data/MNE-sample-data/MEG/sample/sample_audvis-cov.fif");
    QCommandLineOption annotOption("annotType", "Annotation <type> (for source level usage only).", "type", "aparc.a2009s");

    QCommandLineOption sourceLocOption("doSourceLoc", "Do source localization (for source level usage only).", "doSourceLoc", "true");
    QCommandLineOption clustOption("doClust", "Do clustering of source space (for source level usage only).", "doClust", "true");
    QCommandLineOption sourceLocMethodOption("sourceLocMethod", "Inverse estimation <method> (for source level usage only), i.e., 'MNE', 'dSPM' or 'sLORETA'.", "method", "dSPM");
    QCommandLineOption connectMethodOption("connectMethod", "Connectivity <method>, i.e., 'COR', 'XCOR.", "method", "IMAGCOH");
    QCommandLineOption snrOption("snr", "The SNR <value> used for computation (for source level usage only).", "value", "3.0");
    QCommandLineOption evokedIndexOption("aveIdx", "The average <index> to choose from the average file.", "index", "1");
    QCommandLineOption chTypeOption("chType", "The channel <type> (for sensor level usage only), i.e. 'eeg' or 'meg'.", "type", "meg");
    QCommandLineOption coilTypeOption("coilType", "The coil <type> (for sensor level usage only), i.e. 'grad' or 'mag'.", "type", "grad");
    QCommandLineOption tMinOption("tmin", "The time minimum value for averaging in seconds relativ to the trigger onset.", "value", "-0.1");
    QCommandLineOption tMaxOption("tmax", "The time maximum value for averaging in seconds relativ to the trigger onset.", "value", "0.4");

    parser.addOption(annotOption);
    parser.addOption(subjectOption);
    parser.addOption(subjectPathOption);
    parser.addOption(fwdOption);
    parser.addOption(sourceLocOption);
    parser.addOption(clustOption);
    parser.addOption(covFileOption);
    parser.addOption(connectMethodOption);
    parser.addOption(sourceLocMethodOption);
    parser.addOption(snrOption);
    parser.addOption(evokedIndexOption);
    parser.addOption(coilTypeOption);
    parser.addOption(chTypeOption);
    parser.addOption(eventsFileOption);
    parser.addOption(rawFileOption);
    parser.addOption(tMinOption);
    parser.addOption(tMaxOption);

    parser.process(a);

    // Init from arguments
    QString sConnectivityMethod = parser.value(connectMethodOption);
    QString sAnnotType = parser.value(annotOption);
    QString sSubj = parser.value(subjectOption);
    QString sSubjDir = parser.value(subjectPathOption);
    QString sFwd = parser.value(fwdOption);
    QString sCov = parser.value(covFileOption);
    QString sSourceLocMethod = parser.value(sourceLocMethodOption);
    QString sCoilType = parser.value(coilTypeOption);
    QString sChType = parser.value(chTypeOption);
    QString sEve = parser.value(eventsFileOption);
    QString sRaw = parser.value(rawFileOption);
    float fTMin = parser.value(tMinOption).toFloat();
    float fTMax = parser.value(tMaxOption).toFloat();
    double dSnr = parser.value(snrOption).toDouble();
    int iEvent = parser.value(evokedIndexOption).toInt();

    bool bDoSourceLoc = false;
    bool bDoClust = false;
    if(parser.value(sourceLocOption) == "false" || parser.value(sourceLocOption) == "0") {
        bDoSourceLoc = false;
    } else if(parser.value(sourceLocOption) == "true" || parser.value(sourceLocOption) == "1") {
        bDoSourceLoc = true;
    }

    if(parser.value(clustOption) == "false" || parser.value(clustOption) == "0") {
        bDoClust = false;
    } else if(parser.value(clustOption) == "true" || parser.value(clustOption) == "1") {
        bDoClust = true;
    }

    //Set parameters
    QList<MatrixXd> matDataList;
    MatrixXi events;
    RowVectorXi picks;

    MNEForwardSolution t_clusteredFwd;
    MNEForwardSolution t_Fwd;

    SurfaceSet tSurfSetInflated (sSubj, 2, "inflated", sSubjDir);
    AnnotationSet tAnnotSet(sSubj, 2, sAnnotType, sSubjDir);

    QFile coordTransfile(QCoreApplication::applicationDirPath() + "/../resources/data/MNE-sample-data/MEG/sample/all-trans.fif");

    bool keep_comp = false;
    fiff_int_t dest_comp = 0;

    // Create sensor level data
    QFile t_fileRaw(sRaw);
    FiffRawData raw(t_fileRaw);

    int samplesToCutOut = (abs(fTMin) + 0.0) * raw.info.sfreq;
    QSharedPointer<ConnectivitySettingsManager> pConnectivitySettingsManager = QSharedPointer<ConnectivitySettingsManager>::create();

    // Select bad channels
    //raw.info.bads << "MEG2412" << "MEG2413";

    // Setup compensators and projectors so they get applied while reading
    MNE::setup_compensators(raw,
                            dest_comp,
                            keep_comp);

    // Read the events
    MNE::read_events(sEve,
                     sRaw,
                     events);

    // Read the epochs and reject bad epochs. Note, that SSPs are automatically applied to the data if MNE::setup_compensators was called beforehand.
    QStringList exludeChs;
    //exludeChs << "EOG061" << "EOG063";

    QMap<QString,double> mapReject;
    mapReject.insert("eog", 100e-06);
    mapReject.insert("grad", 3000e-13);
    mapReject.insert("mag", 3.5e-12);

    MNEEpochDataList data = MNEEpochDataList::readEpochs(raw,
                                                         events, //events.block(0,0,400,events.cols()),
                                                         fTMin,
                                                         fTMax,
                                                         iEvent,
                                                         mapReject,
                                                         exludeChs);
    data.dropRejected();

    QPair<float, float> pair(fTMin, 0.0f);
    data.applyBaselineCorrection(pair);

    // Average epochs. Do not use SSPs.
    FiffEvoked evoked = data.average(raw.info,
                                     fTMin,
                                     fTMax);

    MNESourceEstimate sourceEstimateEvoked;
    VectorXi vDataIndices;

    QStringList exclude;
    exclude << raw.info.bads << raw.info.ch_names.filter("EOG");

    if(!bDoSourceLoc) {
        // Pick relevant channels
        if(sChType.contains("EEG", Qt::CaseInsensitive)) {
            picks = raw.info.pick_types(false,true,false,QStringList(),exclude);
        } else if(sCoilType.contains("grad", Qt::CaseInsensitive)) {
            // Only pick every second gradiometer which are not marked as bad.
            RowVectorXi picksTmp = raw.info.pick_types(QString("grad"),false,false,QStringList(),exclude);
            picks.resize(0);

            for(int i = 0; i < picksTmp.cols()-1; i+=2) {
                picks.conservativeResize(picks.cols()+1);
                picks(picks.cols()-1) = picksTmp(i);
            }
        } else if (sCoilType.contains("mag", Qt::CaseInsensitive)) {
            picks = raw.info.pick_types(QString("mag"),false,false,QStringList(),exclude);
        }

        // Transform to a more generic data matrix list, pick only channels of interest and remove EOG channel
        MatrixXd matData;
        int iNumberRows = picks.cols(); //picks.cols() 32

        vDataIndices = VectorXi::LinSpaced(iNumberRows,0,iNumberRows);

        for(int i = 0; i < data.size(); ++i) {
            matData.resize(iNumberRows, data.at(i)->epoch.cols());

            for(qint32 j = 0; j < iNumberRows; ++j) {
                matData.row(j) = data.at(i)->epoch.row(picks(j));
            }
            matDataList << matData;
        }

        // Generate network nodes
        pConnectivitySettingsManager->m_settings.setNodePositions(raw.info, picks);
    } else {
        //Create source level data
        QFile t_fileFwd(sFwd);
        t_Fwd = MNEForwardSolution(t_fileFwd, false, true);

        // Load data
        MNESourceEstimate sourceEstimate;

        double lambda2 = 1.0 / pow(dSnr, 2);
        QString method(sSourceLocMethod);

        // regularize noise covariance
        QFile t_fileCov(sCov);
        FiffCov noise_cov(t_fileCov);
        noise_cov = noise_cov.regularize(raw.info,
                                         0.05,
                                         0.05,
                                         0.1,
                                         true);

        // Cluster forward solution;
        if(bDoClust) {
            t_clusteredFwd = t_Fwd.cluster_forward_solution(tAnnotSet, 40);
        } else {
            t_clusteredFwd = t_Fwd;
        }

        MNEInverseOperator inverse_operator(raw.info,
                                            t_clusteredFwd,
                                            noise_cov,
                                            0.2f,
                                            0.8f);

        // Compute inverse solution
        MinimumNorm minimumNorm(inverse_operator, lambda2, method);
        minimumNorm.doInverseSetup(1, true);

        picks = raw.info.pick_types(QString("all"),true,false,QStringList(),exclude);
        data.pick_channels(picks);
        for(int i = 0; i < data.size(); i++) {
            sourceEstimate = minimumNorm.calculateInverse(data.at(i)->epoch,
                                                          evoked.times[0],
                                                          //0.0f,
                                                          1.0/raw.info.sfreq,
                                                          true);

            if(sourceEstimate.isEmpty()) {
                printf("Source estimate is empty");
            } else {
                matDataList << sourceEstimate.data;
            }
        }

        MinimumNorm minimumNormEvoked(inverse_operator, lambda2, method);
        sourceEstimateEvoked = minimumNormEvoked.calculateInverse(evoked, false);
        pConnectivitySettingsManager->m_matEvoked = evoked.data;
        pConnectivitySettingsManager->m_matEvokedSource = sourceEstimateEvoked.data;

        //Generate network nodes and define ROIs
        /*QList<Label> lLabels;
        QList<RowVector4i> qListLabelRGBAs;
        QStringList lWantedLabels;

        lWantedLabels << "G_frontal_inf-Opercular_part-lh"
                        << "G_postcentral-lh"
                        << "G_precentral-lh"
                        << "G_subcentral-lh"
                        << "S_central-lh"
                        << "G_frontal_inf-Opercular_part-rh"
                        << "G_postcentral-rh"
                        << "G_precentral-rh"
                        << "G_subcentral-rh"
                        << "S_central-rh"
                        << "G_occipital_middle-lh"
                        << "G_occipital_middle-rh";

        tAnnotSet.toLabels(tSurfSetInflated, lLabels, qListLabelRGBAs, lWantedLabels);

        //Get active source indices based on picked labels
        vDataIndices = sourceEstimateEvoked.getIndicesByLabel(lLabels, bDoClust);

        //Generate node vertices based on picked labels
        MatrixX3f matNodePositions = t_clusteredFwd.getSourcePositionsByLabel(lLabels, tSurfSetInflated);
        pConnectivitySettingsManager->m_settings.setNodePositions(matNodePositions);*/
        pConnectivitySettingsManager->m_settings.setNodePositions(t_clusteredFwd, tSurfSetInflated);
    }

    pConnectivitySettingsManager->epochs = data;

    //Do connectivity estimation and visualize results
    pConnectivitySettingsManager->m_settings.setConnectivityMethods(QStringList() << sConnectivityMethod);
    pConnectivitySettingsManager->m_settings.setSamplingFrequency(raw.info.sfreq);
    pConnectivitySettingsManager->m_settings.setWindowType("hanning");

    ConnectivitySettings::IntermediateTrialData connectivityData;
    int iNumTrials = matDataList.size();
    for(int i = 0; i < iNumTrials; i++) {
        /*connectivityData.matData.resize(vDataIndices.rows(),matDataList.at(i).cols()-samplesToCutOut);

        for(int j = 0; j < vDataIndices.rows(); j++) {
            // Only calculate connectivity for post stim
            connectivityData.matData.row(j) = matDataList.at(i).row(vDataIndices(j)).segment(samplesToCutOut, matDataList.at(i).cols()-samplesToCutOut);
        }*/
        connectivityData.matData = matDataList.at(i).block(0,
                                                           samplesToCutOut,
                                                           matDataList.at(i).rows(),
                                                           matDataList.at(i).cols()-samplesToCutOut);

        pConnectivitySettingsManager->m_settings.append(connectivityData);
        pConnectivitySettingsManager->m_dataListOriginal.append(connectivityData);
    }

    //Create NetworkView
    NetworkView tNetworkView;
    tNetworkView.show();

    QObject::connect(tNetworkView.getConnectivitySettingsView().data(), &ConnectivitySettingsView::connectivityMetricChanged,
                     pConnectivitySettingsManager.data(), &ConnectivitySettingsManager::onConnectivityMetricChanged);

    QObject::connect(tNetworkView.getConnectivitySettingsView().data(), &ConnectivitySettingsView::numberTrialsChanged,
                     //pConnectivitySettingsManager.data(), &ConnectivitySettingsManager::onNumberTrialsChanged);
                     [&pConnectivitySettingsManager,&tNetworkView,iNumTrials] (int a) {
                         tNetworkView.getConnectivitySettingsView()->setNumberTrials(qMin(iNumTrials,a));
                         pConnectivitySettingsManager.data()->onNumberTrialsChanged(qMin(iNumTrials,a));});


    QObject::connect(tNetworkView.getConnectivitySettingsView().data(), &ConnectivitySettingsView::freqBandChanged,
                     pConnectivitySettingsManager.data(), &ConnectivitySettingsManager::onFreqBandChanged);

    QObject::connect(pConnectivitySettingsManager.data(), &ConnectivitySettingsManager::newConnectivityResultAvailable,
                     [&](const QString& a, const QString& b, const Network& c) {if(NetworkTreeItem* pNetworkTreeItem = tNetworkView.addData(a,b,c)) {
                                                                                    pNetworkTreeItem->setThresholds(QVector3D(0.9,0.95,1.0));
                                                                                }});

    //QPointer<TfSettingsView> pTfSettingsView = new TfSettingsView();
    QList<QWidget*> lWidgets;
    /*lWidgets << pTfSettingsView;

    QObject::connect(pTfSettingsView.data(), &TfSettingsView::numberTrialRowChanged,
                     pConnectivitySettingsManager.data(), &ConnectivitySettingsManager::plotTimeCourses);*/

    //Read and show sensor helmets
    if(!bDoSourceLoc && sChType.contains("meg", Qt::CaseInsensitive)) {
        //Read and show sensor helmets
        QFile t_filesensorSurfaceVV(QCoreApplication::applicationDirPath() + "/../resources/general/sensorSurfaces/306m_rt.fif");
        MNEBem t_sensorSurfaceVV(t_filesensorSurfaceVV);

        if (SensorDataTreeItem* pMegSensorTreeItem = tNetworkView.getTreeModel()->addSensorData(parser.value(subjectOption),
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

        tNetworkView.getTreeModel()->addMegSensorInfo("Sensors",
                                                      "VectorView",
                                                      raw.info.chs,
                                                      t_sensorSurfaceVV,
                                                      raw.info.bads);
    } else {
        QPointer<MinimumNormSettingsView> pMinimumNormSettingsView = new MinimumNormSettingsView("EX_CONNECTIVITY", sSourceLocMethod);
        lWidgets << pMinimumNormSettingsView;

        QObject::connect(pMinimumNormSettingsView.data(), &MinimumNormSettingsView::timePointChanged,
                         [&](int a) {int aSamples = raw.info.sfreq * (float)a * 1.0e-03;
                                     if(aSamples >= sourceEstimateEvoked.samples()-1) {
                                        tNetworkView.getTreeModel()->addSourceData("sample",
                                                                                   evoked.comment,
                                                                                   sourceEstimateEvoked,
                                                                                   t_clusteredFwd,
                                                                                   tSurfSetInflated,
                                                                                   tAnnotSet);
                                     } else {
                                        tNetworkView.getTreeModel()->addSourceData("sample",
                                                                                   evoked.comment,
                                                                                   sourceEstimateEvoked.reduce(aSamples,1),
                                                                                   t_clusteredFwd,
                                                                                   tSurfSetInflated,
                                                                                   tAnnotSet);
                                     }
                                     });

        //Add source loc data and init some visualization values
        if(MneDataTreeItem* pRTDataItem = tNetworkView.getTreeModel()->addSourceData("sample",
                                                                                     evoked.comment,
                                                                                     sourceEstimateEvoked,
                                                                                     t_clusteredFwd,
                                                                                     tSurfSetInflated,
                                                                                     tAnnotSet)) {
            pRTDataItem->setLoopState(true);
            pRTDataItem->setTimeInterval(17);
            pRTDataItem->setNumberAverages(1);
            pRTDataItem->setStreamingState(false);
            pRTDataItem->setThresholds(QVector3D(0.0f,0.5f,10.0f));
            pRTDataItem->setVisualizationType("Interpolation based");
            pRTDataItem->setColormapType("Jet");
            pRTDataItem->setAlpha(0.75f);
        }

        QList<FsSurfaceTreeItem*> lHemis = tNetworkView.getTreeModel()->addSurfaceSet("sample",
                                                                                      "MRI",
                                                                                      tSurfSetInflated,
                                                                                      tAnnotSet);

        lHemis[0]->setAlpha(0.2f);
        lHemis[1]->setAlpha(0.2f);
    }

    tNetworkView.setQuickControlWidgets(lWidgets);
    tNetworkView.getConnectivitySettingsView()->setNumberTrials(iNumTrials);
    pConnectivitySettingsManager->onNumberTrialsChanged(iNumTrials);
    pConnectivitySettingsManager->onFreqBandChanged(tNetworkView.getConnectivitySettingsView()->getLowerFreq(),tNetworkView.getConnectivitySettingsView()->getUpperFreq());

    return a.exec();
}
