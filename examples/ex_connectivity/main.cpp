//=============================================================================================================
/**
* @file     main.cpp
* @author   Lorenz Esch Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
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

#include <connectivity/connectivity.h>
#include <connectivity/connectivitysettings.h>
#include <connectivity/network/network.h>

#include <disp3D/engine/model/items/network/networktreeitem.h>
#include <disp3D/engine/model/items/freesurfer/fssurfacetreeitem.h>
#include <disp3D/engine/model/items/sourcedata/mneestimatetreeitem.h>

#include <fiff/fiff_raw_data.h>

#include <fs/label.h>
#include <fs/annotationset.h>
#include <fs/surfaceset.h>

#include <mne/mne_sourceestimate.h>
#include <mne/mne_epoch_data_list.h>
#include <mne/mne.h>

#include <inverse/minimumNorm/minimumnorm.h>


#include <disp/viewers/connectivitysettingsview.h>


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

    QCommandLineParser parser;
    parser.setApplicationDescription("Connectivity Example");
    parser.addHelpOption();

    QCommandLineOption annotOption("annotType", "Annotation <type> (for source level usage only).", "type", "aparc.a2009s");
    QCommandLineOption sourceLocOption("doSourceLoc", "Do source localization (for source level usage only).", "doSourceLoc", "true");
    QCommandLineOption clustOption("doClust", "Do clustering of source space (for source level usage only).", "doClust", "true");
    QCommandLineOption sourceLocMethodOption("sourceLocMethod", "Inverse estimation <method> (for source level usage only), i.e., 'MNE', 'dSPM' or 'sLORETA'.", "method", "dSPM");
    QCommandLineOption connectMethodOption("connectMethod", "Connectivity <method>, i.e., 'COR', 'XCOR.", "method", "COR");
    QCommandLineOption snrOption("snr", "The SNR <value> used for computation (for source level usage only).", "value", "1.0");
    QCommandLineOption evokedIndexOption("aveIdx", "The average <index> to choose from the average file.", "index", "1");
    QCommandLineOption coilTypeOption("coilType", "The coil <type> (for sensor level usage only), i.e. 'grad' or 'mag'.", "type", "grad");
    QCommandLineOption chTypeOption("chType", "The channel <type> (for sensor level usage only), i.e. 'eeg' or 'meg'.", "type", "meg");
    QCommandLineOption tMinOption("tmin", "The time minimum value for averaging in seconds relativ to the trigger onset.", "value", "-0.1");
    QCommandLineOption tMaxOption("tmax", "The time maximum value for averaging in seconds relativ to the trigger onset.", "value", "0.5");
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
    MatrixX3f matNodePositions;
    MatrixXi events;
    RowVectorXi picks;

    MNEForwardSolution t_clusteredFwd;
    MNEForwardSolution t_Fwd;

    SurfaceSet tSurfSetInflated (sSubj, 2, "inflated", sSubjDir);
    AnnotationSet tAnnotSet(sSubj, 2, sAnnotType, sSubjDir);

    QFile t_fileCov(sCov);
    FiffCov noise_cov(t_fileCov);

    bool keep_comp = false;
    fiff_int_t dest_comp = 0;

    // Create sensor level data
    QFile t_fileRaw(sRaw);
    FiffRawData raw(t_fileRaw);

    // Select bad channels
    //raw.info.bads << "MEG2412" << "MEG2413";

    MNE::setup_compensators(raw,
                            dest_comp,
                            keep_comp);

    // Read the events
    MNE::read_events(sEve,
                     sRaw,
                     events);

    // Read the epochs and reject bad epochs
    MNEEpochDataList data = MNEEpochDataList::readEpochs(raw,
                                                         events,
                                                         fTMin,
                                                         fTMax,
                                                         iEvent,
                                                         150.0*0.0000010,
                                                         "eog");
    data.dropRejected();

    FiffEvoked evoked = data.average(raw.info,
                                     0,
                                     data.first()->epoch.cols()-1,
                                     VectorXi(),
                                     true);
    MNESourceEstimate sourceEstimateEvoked;

    if(!bDoSourceLoc) {
        // Pick relevant channels
        if(sChType.contains("EEG", Qt::CaseInsensitive)) {
            picks = raw.info.pick_types(false,true,false,QStringList(),QStringList() << raw.info.bads << "EOG61");
        } else if(sCoilType == "grad", Qt::CaseInsensitive) {
            picks = raw.info.pick_types(QString("grad"),false,false,QStringList(),QStringList() << raw.info.bads << "EOG61");
        } else if (sCoilType == "mag", Qt::CaseInsensitive) {
            picks = raw.info.pick_types(QString("mag"),false,false,QStringList(),QStringList() << raw.info.bads << "EOG61");
        }

        // Generate nodes for 3D network visualization
        matNodePositions = MatrixX3f(picks.cols(),3);

        // Get the 3D positions and exclude EOG channels
        qint32 kind;
        for(int i = 0; i < picks.cols(); ++i) {
            kind = raw.info.chs.at(i).kind;
            if(kind == FIFFV_EEG_CH ||
               kind == FIFFV_MEG_CH) {
                matNodePositions(i,0) = raw.info.chs.at(picks(i)).chpos.r0(0);
                matNodePositions(i,1) = raw.info.chs.at(picks(i)).chpos.r0(1);
                matNodePositions(i,2) = raw.info.chs.at(picks(i)).chpos.r0(2);
            }
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
    } else {
        //Create source level data
        QFile t_fileFwd(sFwd);
        t_Fwd = MNEForwardSolution(t_fileFwd);

        // Load data
        MNESourceEstimate sourceEstimate;

        double lambda2 = 1.0 / pow(dSnr, 2);
        QString method(sSourceLocMethod);

        // regularize noise covariance
        noise_cov = noise_cov.regularize(raw.info, 0.05, 0.05, 0.1, true);

        // Cluster forward solution;
        if(bDoClust) {
            t_clusteredFwd = t_Fwd.cluster_forward_solution(tAnnotSet, 40);
        } else {
            t_clusteredFwd = t_Fwd;
        }

        MNEInverseOperator inverse_operator(raw.info, t_clusteredFwd, noise_cov, 0.2f, 0.8f);

        // Compute inverse solution
        MinimumNorm minimumNorm(inverse_operator, lambda2, method);
        minimumNorm.doInverseSetup(1,false);

        picks = raw.info.pick_types(true,true,false,QStringList(),QStringList() << raw.info.bads << "EOG61");
        data.pick_channels(picks);

        for(int i = 0; i < data.size(); i++) {
            sourceEstimate = minimumNorm.calculateInverse(data.at(i)->epoch,
                                                          0.0f,
                                                          1/raw.info.sfreq);

            if(sourceEstimate.isEmpty()) {
                printf("Source estimate is empty");
            }

            matDataList << sourceEstimate.data;
        }

        MinimumNorm minimumNormEvoked(inverse_operator, lambda2, method);
        sourceEstimateEvoked = minimumNormEvoked.calculateInverse(evoked);

        //Generate node vertices
        MatrixX3f matNodeVertLeft, matNodeVertRight;

        if(bDoClust) {
            matNodeVertLeft.resize(t_clusteredFwd.src[0].cluster_info.centroidVertno.size(),3);

            for(int j = 0; j < matNodeVertLeft.rows(); ++j) {
                matNodeVertLeft.row(j) = tSurfSetInflated[0].rr().row(t_clusteredFwd.src[0].cluster_info.centroidVertno.at(j)) - tSurfSetInflated[0].offset().transpose();
            }

            matNodeVertRight.resize(t_clusteredFwd.src[1].cluster_info.centroidVertno.size(),3);
            for(int j = 0; j < matNodeVertRight.rows(); ++j) {
                matNodeVertRight.row(j) = tSurfSetInflated[1].rr().row(t_clusteredFwd.src[1].cluster_info.centroidVertno.at(j)) - tSurfSetInflated[1].offset().transpose();
            }
        } else {
            matNodeVertLeft.resize(t_Fwd.src[0].vertno.rows(),3);
            for(int j = 0; j < matNodeVertLeft.rows(); ++j) {
                matNodeVertLeft.row(j) = tSurfSetInflated[0].rr().row(t_Fwd.src[0].vertno(j)) - tSurfSetInflated[0].offset().transpose();
            }

            matNodeVertRight.resize(t_Fwd.src[1].vertno.rows(),3);
            for(int j = 0; j < matNodeVertRight.rows(); ++j) {
                matNodeVertRight.row(j) = tSurfSetInflated[1].rr().row(t_Fwd.src[1].vertno(j)) - tSurfSetInflated[1].offset().transpose();
            }
        }

        matNodePositions.resize(matNodeVertLeft.rows()+matNodeVertRight.rows(),3);
        matNodePositions << matNodeVertLeft, matNodeVertRight;
    }

    //Do connectivity estimation and visualize results
    QSharedPointer<ConnectivitySettingsManager> pConnectivitySettingsManager = QSharedPointer<ConnectivitySettingsManager>::create(matDataList.first().cols(), raw.info.sfreq);

    pConnectivitySettingsManager->m_settings.m_sConnectivityMethods << sConnectivityMethod;
    pConnectivitySettingsManager->m_settings.m_matDataList = matDataList;
    pConnectivitySettingsManager->m_matDataListOriginal = matDataList;
    pConnectivitySettingsManager->m_settings.m_matNodePositions = matNodePositions;
    pConnectivitySettingsManager->m_settings.m_iNfft = -1;
    pConnectivitySettingsManager->m_settings.m_sWindowType = "hanning";

    //Create NetworkView and add extra control widgets to output data (will be used by QuickControlView in RealTimeConnectivityEstimateWidget)
    NetworkView tNetworkView;

    ConnectivitySettingsView::SPtr pConnectivitySettingsView = ConnectivitySettingsView::SPtr::create();

    QList<QSharedPointer<QWidget> > lWidgets;
    lWidgets << pConnectivitySettingsView;
    tNetworkView.setQuickControlWidgets(lWidgets);
    tNetworkView.show();

    QObject::connect(pConnectivitySettingsView.data(), &ConnectivitySettingsView::connectivityMetricChanged,
                     pConnectivitySettingsManager.data(), &ConnectivitySettingsManager::onConnectivityMetricChanged);

    QObject::connect(pConnectivitySettingsView.data(), &ConnectivitySettingsView::numberTrialsChanged,
                     pConnectivitySettingsManager.data(), &ConnectivitySettingsManager::onNumberTrialsChanged);

    QObject::connect(pConnectivitySettingsView.data(), &ConnectivitySettingsView::freqBandChanged,
                     pConnectivitySettingsManager.data(), &ConnectivitySettingsManager::onFreqBandChanged);

    QObject::connect(pConnectivitySettingsManager.data(), &ConnectivitySettingsManager::newConnectivityResultAvailable,
                     &tNetworkView, &NetworkView::addData);

    //Read and show sensor helmets
    if(!bDoSourceLoc && sChType.contains("meg", Qt::CaseInsensitive)) {
        QFile t_filesensorSurfaceVV(QCoreApplication::applicationDirPath() + "/resources/general/sensorSurfaces/306m_rt.fif");
        MNEBem t_sensorSurfaceVV(t_filesensorSurfaceVV);
        tNetworkView.getTreeModel()->addMegSensorInfo("Sensors",
                                                      "VectorView",
                                                      raw.info.chs,
                                                      t_sensorSurfaceVV);
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
            pRTDataItem->setNumberAverages(1);
            pRTDataItem->setStreamingState(false);
            pRTDataItem->setThresholds(QVector3D(0.0f,0.5f,10.0f));
            pRTDataItem->setVisualizationType("Interpolation based");
            pRTDataItem->setColormapType("Jet");
            pRTDataItem->setAlpha(0.5f);
        }
    }

    pConnectivitySettingsView->setNumberTrials(1);
    pConnectivitySettingsManager->onNumberTrialsChanged(1);

    return a.exec();
}
