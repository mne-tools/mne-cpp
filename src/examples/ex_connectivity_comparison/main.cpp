//=============================================================================================================
/**
 * @file     main.cpp
 * @author   Gabriel B Motta <gabrielbenmotta@gmail.com>;
 *           Juan Garcia-Prieto <juangpc@gmail.com>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2016
 *
 * @section  LICENSE
 *
 * Copyright (C) 2016, Gabriel B Motta, Juan Garcia-Prieto, Lorenz Esch. All rights reserved.
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
 * @brief     Example to compare connectivity methods
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <disp3D/viewers/networkview.h>
#include <disp3D/engine/model/data3Dtreemodel.h>

#include <connectivity/connectivity.h>
#include <connectivity/connectivitysettings.h>
#include <connectivity/network/network.h>
#include <connectivity/metrics/abstractmetric.h>

#include <disp3D/engine/model/items/network/networktreeitem.h>
#include <disp3D/engine/model/items/freesurfer/fssurfacetreeitem.h>
#include <disp3D/engine/model/items/sourcedata/mnedatatreeitem.h>

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

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QApplication>
#include <QMainWindow>
#include <QCommandLineParser>
#include <QDebug>
#include <QFile>
#include <QObject>

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

    AbstractMetric::m_bStorageModeIsActive = false;
//    AbstractMetric::m_iNumberBinStart = 8;
//    AbstractMetric::m_iNumberBinAmount = 4;

    QCommandLineParser parser;
    parser.setApplicationDescription("Connectivity Comparison Example");
    parser.addHelpOption();

    QCommandLineOption annotOption("annotType", "Annotation <type> (for source level usage only).", "type", "aparc.a2009s");
    QCommandLineOption sourceLocOption("doSourceLoc", "Do source localization (for source level usage only).", "doSourceLoc", "true");
    QCommandLineOption clustOption("doClust", "Do clustering of source space (for source level usage only).", "doClust", "true");
    QCommandLineOption sourceLocMethodOption("sourceLocMethod", "Inverse estimation <method> (for source level usage only), i.e., 'MNE', 'dSPM' or 'sLORETA'.", "method", "dSPM");
    QCommandLineOption snrOption("snr", "The SNR <value> used for computation (for source level usage only).", "value", "1.0");
    QCommandLineOption evokedIndexOption("aveIdx", "The average <index> to choose from the average file.", "index", "3");
    QCommandLineOption coilTypeOption("coilType", "The coil <type> (for sensor level usage only), i.e. 'grad' or 'mag'.", "type", "grad");
    QCommandLineOption chTypeOption("chType", "The channel <type> (for sensor level usage only), i.e. 'eeg' or 'meg'.", "type", "meg");
    QCommandLineOption tMinOption("tmin", "The time minimum value for averaging in seconds relativ to the trigger onset.", "value", "-0.1");
    QCommandLineOption tMaxOption("tmax", "The time maximum value for averaging in seconds relativ to the trigger onset.", "value", "0.4");
    QCommandLineOption eventsFileOption("eve", "Path to the event <file>.", "file", QCoreApplication::applicationDirPath() + "/../resources/data/MNE-sample-data/MEG/sample/sample_audvis_raw-eve.fif");
    QCommandLineOption rawFileOption("raw", "Path to the raw <file>.", "file", QCoreApplication::applicationDirPath() + "/../resources/data/MNE-sample-data/MEG/sample/sample_audvis_raw.fif");
    QCommandLineOption subjectOption("subj", "Selected <subject> (for source level usage only).", "subject", "sample");
    QCommandLineOption subjectPathOption("subjDir", "Selected <subjectPath> (for source level usage only).", "subjectPath", QCoreApplication::applicationDirPath() + "/../resources/data/MNE-sample-data/subjects");
    QCommandLineOption fwdOption("fwd", "Path to forwad solution <file> (for source level usage only).", "file", QCoreApplication::applicationDirPath() + "/../resources/data/MNE-sample-data/MEG/sample/sample_audvis-meg-eeg-oct-6-fwd.fif");
    QCommandLineOption covFileOption("cov", "Path to the covariance <file> (for source level usage only).", "file", QCoreApplication::applicationDirPath() + "/../resources/data/MNE-sample-data/MEG/sample/sample_audvis-cov.fif");
    parser.addOption(annotOption);
    parser.addOption(subjectOption);
    parser.addOption(subjectPathOption);
    parser.addOption(fwdOption);
    parser.addOption(sourceLocOption);
    parser.addOption(clustOption);
    parser.addOption(covFileOption);
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

    QFile t_fileCov(sCov);
    FiffCov noise_cov(t_fileCov);

    bool keep_comp = false;
    fiff_int_t dest_comp = 0;

    ConnectivitySettings conSettings;

    // Create sensor level data
    QFile t_fileRaw(sRaw);
    FiffRawData raw(t_fileRaw);

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
    QMap<QString,double> mapReject;
    mapReject.insert("eog", 300e-06);

    MNEEpochDataList data = MNEEpochDataList::readEpochs(raw,
                                                         events,
                                                         fTMin,
                                                         fTMax,
                                                         iEvent,
                                                         mapReject);
    data.dropRejected();
    QPair<float, float> pair(fTMin, 0.0f);
    data.applyBaselineCorrection(pair);

    FiffEvoked evoked = data.average(raw.info,
                                     0,
                                     data.first()->epoch.cols()-1);
    MNESourceEstimate sourceEstimateEvoked;

    QStringList exclude;
    exclude << raw.info.bads << raw.info.ch_names.filter("EOG");

    if(!bDoSourceLoc) {
        // Pick relevant channels
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

        // Generate network nodes
        conSettings.setNodePositions(raw.info, picks);

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
        t_Fwd = MNEForwardSolution(t_fileFwd, false, true);

        // Load data
        MNESourceEstimate sourceEstimate;

        double lambda2 = 1.0 / pow(dSnr, 2);
        QString method(sSourceLocMethod);

        // regularize noise covariance
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
        minimumNorm.doInverseSetup(1,true);

        picks = raw.info.pick_types(QString("all"),true,false,QStringList(),exclude);
        data.pick_channels(picks);
        for(int i = 0; i < data.size(); i++) {
            sourceEstimate = minimumNorm.calculateInverse(data.at(i)->epoch,
                                                          0.0f,
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

        // Generate network nodes
        conSettings.setNodePositions(t_clusteredFwd, tSurfSetInflated);
    }

    // Compute the connectivity estimates for the methods to be compared
    conSettings.setConnectivityMethods(QStringList() << "COH" << "COR" << "XCOR" << "PLI" << "IMAGCOH" << "PLV" << "WPLI" << "USPLI" << "DSWPLI");

    for(int i = 0; i < matDataList.size(); i++) {
        // Only calculate connectivity for post stim
        int samplesToCutOut = abs(fTMin*raw.info.sfreq);
        conSettings.append(matDataList.at(i).block(0,
                                                  samplesToCutOut,
                                                  matDataList.at(i).rows(),
                                                  matDataList.at(i).cols()-samplesToCutOut));
    }

    conSettings.setSamplingFrequency(raw.info.sfreq);
    conSettings.setWindowType("hanning");

    QList<Network> lNetworks = Connectivity::calculate(conSettings);

    QMap<QString,Vector4i> mColor;
    mColor.insert("COR",Vector4i(90, 26, 100, 1));
    mColor.insert("XCOR",Vector4i(255, 20, 80, 1));
    mColor.insert("PLI",Vector4i(255, 255, 0, 1));
    mColor.insert("COH",Vector4i(2, 89, 100, 1));
    mColor.insert("IMAGCOH",Vector4i(50, 255, 48, 1));
    mColor.insert("PLV",Vector4i(0, 255, 255, 1));
    mColor.insert("WPLI",Vector4i(255, 0, 100, 1));
    mColor.insert("USPLI",Vector4i(255, 89, 200, 1));
    mColor.insert("DSWPLI",Vector4i(25, 10, 255, 1));

    for(int j = 0; j < lNetworks.size(); ++j) {
        lNetworks[j].setFrequencyRange(7.0f, 13.0f);
        lNetworks[j].normalize();
        VisualizationInfo visInfo = lNetworks.at(j).getVisualizationInfo();
        visInfo.sMethod = "Color";
        visInfo.colNodes = mColor[lNetworks[j].getConnectivityMethod()];
        visInfo.colEdges = mColor[lNetworks[j].getConnectivityMethod()];
        lNetworks[j].setVisualizationInfo(visInfo);
    }

    //Create NetworkView
    NetworkView tNetworkView;
    tNetworkView.show();
    QList<NetworkTreeItem*> lNetworkTreeItems = tNetworkView.addData("sample",
                                                                     evoked.comment,
                                                                     lNetworks);

    for(int j = 0; j < lNetworkTreeItems.size(); ++j) {
        lNetworkTreeItems.at(j)->setThresholds(QVector3D(0.9,0.95,1.0));
    }

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
            pRTDataItem->setAlpha(0.25f);
        }
    }

    return a.exec();
}
