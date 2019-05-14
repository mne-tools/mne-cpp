//=============================================================================================================
/**
* @file     main.cpp
* @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
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

#include "connectivitysettingsmanager.h"

#include <disp3D/viewers/networkview.h>
#include <disp3D/engine/model/data3Dtreemodel.h>
#include <disp3D/engine/model/items/network/networktreeitem.h>
#include <disp3D/engine/model/items/freesurfer/fssurfacetreeitem.h>
#include <disp3D/engine/model/items/sourcedata/mneestimatetreeitem.h>
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

#include <disp/viewers/connectivitysettingsview.h>
#include <disp3D/engine/model/items/network/networktreeitem.h>

//----- TEMP includes
#include <fiff/fiff_dig_point_set.h>
#include <disp3D/engine/model/items/sensordata/sensordatatreeitem.h>
#include <disp3D/engine/model/items/sensorspace/sensorsettreeitem.h>
#include <connectivity/metrics/abstractmetric.h>
#include <mne/mne_bem_surface.h>


//*************************************************************************************************************
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


//*************************************************************************************************************
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

    AbstractMetric::m_bStorageModeIsActive = true;
//    AbstractMetric::m_iNumberBinStart = 8;
//    AbstractMetric::m_iNumberBinAmount = 4;

    QCommandLineParser parser;
    parser.setApplicationDescription("Connectivity Example");
    parser.addHelpOption();

    QCommandLineOption annotOption("annotType", "Annotation <type> (for source level usage only).", "type", "aparc.a2009s");
    QCommandLineOption sourceLocOption("doSourceLoc", "Do source localization (for source level usage only).", "doSourceLoc", "true");
    QCommandLineOption clustOption("doClust", "Do clustering of source space (for source level usage only).", "doClust", "true");
    QCommandLineOption sourceLocMethodOption("sourceLocMethod", "Inverse estimation <method> (for source level usage only), i.e., 'MNE', 'dSPM' or 'sLORETA'.", "method", "dSPM");
    QCommandLineOption connectMethodOption("connectMethod", "Connectivity <method>, i.e., 'COR', 'XCOR.", "method", "COR");
    QCommandLineOption snrOption("snr", "The SNR <value> used for computation (for source level usage only).", "value", "1.0");
    QCommandLineOption evokedIndexOption("aveIdx", "The average <index> to choose from the average file.", "index", "3");
    QCommandLineOption coilTypeOption("coilType", "The coil <type> (for sensor level usage only), i.e. 'grad' or 'mag'.", "type", "grad");
    QCommandLineOption chTypeOption("chType", "The channel <type> (for sensor level usage only), i.e. 'eeg' or 'meg'.", "type", "meg");
    QCommandLineOption tMinOption("tmin", "The time minimum value for averaging in seconds relativ to the trigger onset.", "value", "-0.1");
    QCommandLineOption tMaxOption("tmax", "The time maximum value for averaging in seconds relativ to the trigger onset.", "value", "0.4");
    QCommandLineOption eventsFileOption("eve", "Path to the event <file>.", "file", QCoreApplication::applicationDirPath() + "/MNE-sample-data/MEG/sample/sample_audvis_raw-eve.fif");
    QCommandLineOption rawFileOption("raw", "Path to the raw <file>.", "file", QCoreApplication::applicationDirPath() + "/MNE-sample-data/MEG/sample/sample_audvis_raw.fif");
    QCommandLineOption subjectOption("subj", "Selected <subject> (for source level usage only).", "subject", "sample");
    QCommandLineOption subjectPathOption("subjDir", "Selected <subjectPath> (for source level usage only).", "subjectPath", QCoreApplication::applicationDirPath() + "/MNE-sample-data/subjects");
    QCommandLineOption fwdOption("fwd", "Path to forwad solution <file> (for source level usage only).", "file", QCoreApplication::applicationDirPath() + "/MNE-sample-data/MEG/sample/sample_audvis-meg-eeg-oct-6-fwd.fif");
    QCommandLineOption covFileOption("cov", "Path to the covariance <file> (for source level usage only).", "file", QCoreApplication::applicationDirPath() + "/MNE-sample-data/MEG/sample/sample_audvis-cov.fif");

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

    bool bDoSourceLoc, bDoClust = false;
    if(parser.value(sourceLocOption) == "false" || parser.value(sourceLocOption) == "0") {
        bDoSourceLoc =false;
    } else if(parser.value(sourceLocOption) == "true" || parser.value(sourceLocOption) == "1") {
        bDoSourceLoc = true;
    }

    if(parser.value(clustOption) == "false" || parser.value(clustOption) == "0") {
        bDoClust =false;
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

    QFile coordTransfile(QCoreApplication::applicationDirPath() + "/MNE-sample-data/MEG/sample/all-trans.fif");
    //QFile coordTransfile(QCoreApplication::applicationDirPath() + "/MNE-sample-data/subjects/sample/mri/brain-neuromag/sets/COR.fif");

    FiffCoordTrans coordTrans(coordTransfile);

    bool keep_comp = false;
    fiff_int_t dest_comp = 0;

    // Create sensor level data
    QFile t_fileRaw(sRaw);
    FiffRawData raw(t_fileRaw);

    int samplesToCutOut = abs(fTMin * raw.info.sfreq);
    QSharedPointer<ConnectivitySettingsManager> pConnectivitySettingsManager;

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
    MNEEpochDataList data = MNEEpochDataList::readEpochs(raw,
                                                         events,
                                                         fTMin,
                                                         fTMax,
                                                         iEvent,
                                                         150*pow(10.0,-06),
                                                         "eog");
    data.dropRejected();
    QPair<QVariant, QVariant> pair(QVariant(fTMin), QVariant("0.0"));
    data.applyBaselineCorrection(pair);

    // Average epochs. Do not use SSPs.
    FiffEvoked evoked = data.average(raw.info,
                                     0,
                                     data.first()->epoch.cols()-1);

    MNESourceEstimate sourceEstimateEvoked;

    if(!bDoSourceLoc) {
        // Pick relevant channels
        QStringList exclude;
        exclude << raw.info.bads << raw.info.ch_names.filter("EOG");

        if(sChType.contains("EEG", Qt::CaseInsensitive)) {
            picks = raw.info.pick_types(false,true,false,QStringList(),exclude);
        } else if(sCoilType.contains("grad", Qt::CaseInsensitive)) {
            // Only pick every second gradiometer which are not marked as bad.
            RowVectorXi picksTmp = raw.info.pick_types(QString("grad"),false,false);
            picks.resize(0);

            for(int i = 0; i < picksTmp.cols()-1; i+=2) {
                if(!raw.info.bads.contains(raw.info.ch_names.at(picksTmp(i)))) {
                    picks.conservativeResize(picks.cols()+1);
                    picks(picks.cols()-1) = picksTmp(i);
                }
            }
        } else if (sCoilType.contains("mag", Qt::CaseInsensitive)) {
            picks = raw.info.pick_types(QString("mag"),false,false,QStringList(),exclude);
        }

        // Transform to a more generic data matrix list, pick only channels of interest and remove EOG channel
        MatrixXd matData;
        int iNumberRows = picks.cols(); //picks.cols() 32

        for(int i = 0; i < data.size(); ++i) {
            matData.resize(iNumberRows, data.at(i)->epoch.cols());

            for(qint32 j = 0; j < iNumberRows; ++j) {
                matData.row(j) = data.at(i)->epoch.row(picks(j));
            }
            matDataList << matData;
        }

        // Generate network nodes
        pConnectivitySettingsManager = QSharedPointer<ConnectivitySettingsManager>::create(matDataList.first().cols()-samplesToCutOut);
        pConnectivitySettingsManager->m_settings.setNodePositions(raw.info, picks);
    } else {
        //Create source level data
        QFile t_fileFwd(sFwd);
        t_Fwd = MNEForwardSolution(t_fileFwd);

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
            t_clusteredFwd = t_Fwd.cluster_forward_solution(tAnnotSet, 200);
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
        minimumNorm.doInverseSetup(1,false);

        picks = raw.info.pick_types(QString("all"),true,false,QStringList(),QStringList() << raw.info.bads << "EOG61");
        data.pick_channels(picks);
        for(int i = 0; i < data.size(); i++) {
            sourceEstimate = minimumNorm.calculateInverse(data.at(i)->epoch,
                                                          0.0f,
                                                          1.0/raw.info.sfreq);

            if(sourceEstimate.isEmpty()) {
                printf("Source estimate is empty");
            } else {
                matDataList << sourceEstimate.data;
            }
        }

        MinimumNorm minimumNormEvoked(inverse_operator, lambda2, method);
        sourceEstimateEvoked = minimumNormEvoked.calculateInverse(evoked);
        //sourceEstimateEvoked = sourceEstimateEvoked.reduce(0.24*evoked.info.sfreq,1);

        // Generate network nodes
        pConnectivitySettingsManager = QSharedPointer<ConnectivitySettingsManager>::create(matDataList.first().cols()-samplesToCutOut);
        pConnectivitySettingsManager->m_settings.setNodePositions(t_clusteredFwd, tSurfSetInflated);
    }

    //Do connectivity estimation and visualize results
    pConnectivitySettingsManager->m_settings.setConnectivityMethods(QStringList() << sConnectivityMethod);
    pConnectivitySettingsManager->m_settings.setSamplingFrequency(raw.info.sfreq);
    pConnectivitySettingsManager->m_settings.setWindowType("hanning");

    ConnectivitySettings::IntermediateTrialData connectivityData;
    for(int i = 0; i < matDataList.size(); i++) {
        // Only calculate connectivity for post stim
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
                     pConnectivitySettingsManager.data(), &ConnectivitySettingsManager::onNumberTrialsChanged);

    QObject::connect(tNetworkView.getConnectivitySettingsView().data(), &ConnectivitySettingsView::freqBandChanged,
                     pConnectivitySettingsManager.data(), &ConnectivitySettingsManager::onFreqBandChanged);

    QObject::connect(pConnectivitySettingsManager.data(), &ConnectivitySettingsManager::newConnectivityResultAvailable,
                     [&](const QString& a, const QString& b, const Network& c) {if(NetworkTreeItem* pNetworkTreeItem = tNetworkView.addData(a,b,c)) {
                                                                                    pNetworkTreeItem->setThresholds(QVector3D(0.9,0.95,1.0));
                                                                                }});

    //Read and show sensor helmets
    if(!bDoSourceLoc && sChType.contains("meg", Qt::CaseInsensitive)) {
        QFile t_filesensorSurfaceVV(QCoreApplication::applicationDirPath() + "/resources/general/sensorSurfaces/306m_rt.fif");
        MNEBem t_sensorSurfaceVV(t_filesensorSurfaceVV);
        tNetworkView.getTreeModel()->addMegSensorInfo("Sensors",
                                                      "VectorView",
                                                      raw.info.chs,
                                                      t_sensorSurfaceVV,
                                                      raw.info.bads);
    } else {
        //Add source loc data and init some visualization values
        if(MneEstimateTreeItem* pRTDataItem = tNetworkView.getTreeModel()->addSourceData("sample",
                                                                                         evoked.comment,
                                                                                         sourceEstimateEvoked,
                                                                                         t_clusteredFwd,
                                                                                         tSurfSetInflated,
                                                                                         tAnnotSet)) {
            pRTDataItem->setLoopState(true);
            pRTDataItem->setTimeInterval(17);
            pRTDataItem->setNumberAverages(17);
            pRTDataItem->setStreamingState(false);
            pRTDataItem->setThresholds(QVector3D(0.0f,0.5f,10.0f));
            pRTDataItem->setVisualizationType("Interpolation based");
            pRTDataItem->setColormapType("Jet");
            pRTDataItem->setAlpha(0.25f);
        }
    }

    tNetworkView.getConnectivitySettingsView()->setNumberTrials(1);
    pConnectivitySettingsManager->onNumberTrialsChanged(1);

//    // ------- TEMP data -------

//    QMatrix4x4 matrix;
//    matrix.setToIdentity();
//    matrix.translate(0,-0.02,0.05200);
//    Qt3DCore::QTransform transformtrans;
//    transformtrans.setMatrix(matrix);

//    //Read and show BEM
//    QFile t_fileBem(QCoreApplication::applicationDirPath() + "/MNE-sample-data/subjects/sample/bem/sample-5120-5120-5120-bem.fif");
//    MNEBem t_Bem(t_fileBem);
////    tNetworkView.getTreeModel()->addBemData(parser.value(subjectOption), "BEM", t_Bem);

//    QFile t_fileBemhead(QCoreApplication::applicationDirPath() + "/MNE-sample-data/subjects/sample/bem/sample-head.fif");
//    MNEBem t_Bemhead(t_fileBemhead);

//    //Read and show sensor helmets
//    QFile t_filesensorSurfaceVV(QCoreApplication::applicationDirPath() + "/resources/general/sensorSurfaces/306m.fif");
//    MNEBem t_sensorSurfaceVV(t_filesensorSurfaceVV);

//    SensorSetTreeItem* pSensorSetTreeItem = tNetworkView.getTreeModel()->addMegSensorInfo("Sensors", "VectorView", evoked.info.chs, t_sensorSurfaceVV, evoked.info.bads);
//    pSensorSetTreeItem->applyTransform(transformtrans);

//    // Read, co-register and show digitizer points
////    QFile t_fileDig(QCoreApplication::applicationDirPath() + "/MNE-sample-data/MEG/sample/sample_audvis_raw.fif");
////    FiffDigPointSet t_Dig(t_fileDig);

////    DigitizerSetTreeItem* pDigitizerSetTreeItem = tNetworkView.getTreeModel()->addDigitizerData(parser.value(subjectOption), evoked.comment, t_Dig);
////    pDigitizerSetTreeItem->applyTransform(coordTrans, true);

//    //add sensor item for MEG data
//    if (SensorDataTreeItem* pMegSensorTreeItem = tNetworkView.getTreeModel()->addSensorData(parser.value(subjectOption),
//                                                                                            evoked.comment,
//                                                                                            //evoked.data.block(0,0.24*evoked.info.sfreq,evoked.data.rows(),1),
//                                                                                            evoked.data,
//                                                                                            t_sensorSurfaceVV[0],
//                                                                                            evoked.info,
//                                                                                            "MEG")) {
//        pMegSensorTreeItem->setLoopState(true);
//        pMegSensorTreeItem->setTimeInterval(17);
//        pMegSensorTreeItem->setNumberAverages(17);
//        pMegSensorTreeItem->setStreamingState(true);
//        pMegSensorTreeItem->setThresholds(QVector3D(0.0f, 13.0e-13f*0.5f, 13.0e-14f));
//        pMegSensorTreeItem->setColormapType("Jet");
//        pMegSensorTreeItem->setSFreq(evoked.info.sfreq);

//        // Apply head to device transformation
//        //pMegSensorTreeItem->applyTransform(coordTrans,true);
//        //pMegSensorTreeItem->applyTransform(raw.info.dev_head_t);
//        pMegSensorTreeItem->setTransform(transformtrans);
//    }

//    //add sensor item for EEG data

//    //Co-Register EEG points in order to correctly map them to the scalp
//    for(int i = 0; i < evoked.info.chs.size(); ++i) {
//        if(evoked.info.chs.at(i).kind == FIFFV_EEG_CH) {
//            Vector4f tempvec;
//            tempvec(0) = evoked.info.chs.at(i).chpos.r0(0);
//            tempvec(1) = evoked.info.chs.at(i).chpos.r0(1);
//            tempvec(2) = evoked.info.chs.at(i).chpos.r0(2);
//            tempvec(3) = 1;
//            tempvec = coordTrans.invtrans * tempvec;
//            evoked.info.chs[i].chpos.r0(0) = tempvec(0);
//            evoked.info.chs[i].chpos.r0(1) = tempvec(1);
//            evoked.info.chs[i].chpos.r0(2) = tempvec(2);
//        }
//    }

//    if (SensorDataTreeItem* pEegSensorTreeItem = tNetworkView.getTreeModel()->addSensorData(parser.value(subjectOption),
//                                                                                            evoked.comment,
//                                                                                            //evoked.data.block(0,0.24*evoked.info.sfreq,evoked.data.rows(),1),
//                                                                                            evoked.data,
//                                                                                            t_Bem[0],
//                                                                                            evoked.info,
//                                                                                            "EEG")) {
//        pEegSensorTreeItem->setLoopState(true);
//        pEegSensorTreeItem->setTimeInterval(17);
//        pEegSensorTreeItem->setNumberAverages(17);
//        pEegSensorTreeItem->setStreamingState(true);
//        pEegSensorTreeItem->setThresholds(QVector3D(0.0f, 3.0e-6f, 6.0e-6f));
//        pEegSensorTreeItem->setColormapType("Jet");
//        pEegSensorTreeItem->setSFreq(evoked.info.sfreq);
//        pEegSensorTreeItem->setCancelDistance(0.20);

//        // Apply head to device transformation
//        //pEegSensorTreeItem->applyTransform(coordTrans);
//        //pEegSensorTreeItem->applyTransform(raw.info.dev_head_t, true);
//    }
//    qDebug() << "-----------------t_Bemhead[0].np" << t_Bemhead[0].np;
//    qDebug() << "-----------------t_Bem[0].np" << t_Bem[0].np;
//    qDebug() << "-----------------t_sensorSurfaceVV[0].np" << t_sensorSurfaceVV[0].np;
//    qDebug() << "-----------------tSurfSetInflated[0].rr().rows()" << tSurfSetInflated[0].rr().rows();
//    qDebug() << "-----------------tSurfSetInflated[1].rr().rows()" << tSurfSetInflated[1].rr().rows();

//    // ------- TEMP data END -------

    return a.exec();
}
