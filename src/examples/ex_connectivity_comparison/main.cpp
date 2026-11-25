//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2016-2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           johaenns <j.vorw01@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     July, 2016
 * @brief     Compare connectivity metrics side by side on the same averaged sensor- or source-level data.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/disp3D/view/brainview.h>
#include <mne/base/string_utils.h>
#include <mne/disp3D/model/braintreemodel.h>
#include <mne/disp3D/model/items/networktreeitem.h>
#include <mne/connectivity/compute_connectivity.h>
#include <mne/connectivity/connectivitysettings.h>
#include <mne/connectivity/network/network.h>
#include <mne/connectivity/metrics/abstractmetric.h>
#include <mne/fiff/fiff_raw_data.h>
#include <mne/fs/fs_label.h>
#include <mne/fs/fs_annotationset.h>
#include <mne/fs/fs_surfaceset.h>
#include <mne/inv/inv_source_estimate.h>
#include <mne/mne/mne_epoch_data_list.h>
#include <mne/mne/mne.h>
#include <mne/inv/minimum_norm/inv_minimum_norm.h>
#include <mne/base/log.h>
#include <mne/fiff/fiff_events.h>
#include <mne/disp/viewers/connectivitysettingsview.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QApplication>
#include <QMainWindow>
#include <QCommandLineParser>
#include <QDir>
#include <QFile>
#include <QObject>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <memory>
#include <map>
#include <string>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::disp;
using namespace mne;
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
int main(int argc, char* argv[])
{
#ifdef STATICBUILD
// Q_INIT_RESOURCE(mne_disp3d);
#endif

    QApplication a(argc, argv);


    QCommandLineParser parser;
    parser.setApplicationDescription("Connectivity Comparison Example");
    parser.addHelpOption();

    QCommandLineOption annotOption("annotType", "FsAnnotation <type> (for source level usage only).", "type", "aparc.a2009s");
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
    QCommandLineOption rawFileOption("fileIn", "Path to the raw <file>.", "file", QCoreApplication::applicationDirPath() + "/../resources/data/MNE-sample-data/MEG/sample/sample_audvis_raw.fif");
    QCommandLineOption subjectOption("subj", "Selected <subject> (for source level usage only).", "subject", "sample");
    QCommandLineOption subjectPathOption("subjDir", "Selected <subjectPath> (for source level usage only).", "subjectPath", QCoreApplication::applicationDirPath() + "/../resources/data/MNE-sample-data/subjects");
    QCommandLineOption fwdOption("fwd", "Path to forwad solution <file> (for source level usage only).", "file", QCoreApplication::applicationDirPath() + "/../resources/data/MNE-sample-data/MEG/sample/sample_audvis-meg-eeg-oct-6-fwd.fif");
    QCommandLineOption covFileOption("cov", "Path to the covariance <file> (for source level usage only).", "file", QCoreApplication::applicationDirPath() + "/../resources/data/MNE-sample-data/MEG/sample/sample_audvis-cov.fif");
    QCommandLineOption trialOption("ntrials", "The number of averaged trials.", "value", "200");
    QCommandLineOption freqMinOption("freqmin", "The lower frequency bound for connectivity computation.", "value", "7.0");
    QCommandLineOption freqMaxOption("freqmax", "The upper frequency bound for connectivity computation.", "value", "13.0");
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
    parser.addOption(trialOption);
    parser.addOption(freqMinOption);
    parser.addOption(freqMaxOption);

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
    int iNumTrials = parser.value(trialOption).toInt();
    double dFreqMin = parser.value(freqMinOption).toDouble();
    double dFreqMax = parser.value(freqMaxOption).toDouble();

    bool bDoSourceLoc = false;
    bool bDoClust = false;
    if (parser.value(sourceLocOption) == "false" || parser.value(sourceLocOption) == "0") {
        bDoSourceLoc = false;
    } else if (parser.value(sourceLocOption) == "true" || parser.value(sourceLocOption) == "1") {
        bDoSourceLoc = true;
    }

    if (parser.value(clustOption) == "false" || parser.value(clustOption) == "0") {
        bDoClust = false;
    } else if (parser.value(clustOption) == "true" || parser.value(clustOption) == "1") {
        bDoClust = true;
    }

    //Set parameters
    QList<MatrixXd> matDataList;
    MatrixXi events;
    RowVectorXi picks;

    MNEForwardSolution t_clusteredFwd;
    MNEForwardSolution t_Fwd;

    FsSurfaceSet tSurfSetInflated(sSubj.toStdString(), 2, "inflated", sSubjDir.toStdU16String());
    FsAnnotationSet tAnnotSet(sSubj.toStdString(), 2, sAnnotType.toStdString(), sSubjDir.toStdU16String());

    FiffCov noise_cov(sCov.toStdString());

    bool keep_comp = false;
    fiff_int_t dest_comp = 0;

    ConnectivitySettings conSettings;

    // Create sensor level data
    FiffRawData raw(sRaw.toStdString());

    // Select bad channels
    //raw.info.bads << "MEG 2412" << "MEG 2413";

    // Setup compensators and projectors so they get applied while reading
    raw.activate_projections();
    if (!keep_comp)
        raw.apply_gradient_compensation(dest_comp).value();

    // Read the events
    events = FiffEvents::read(sEve.toStdU16String()).value().events;

    // Read the epochs and reject bad epochs. Note, that SSPs are automatically applied to the data if activate_projections() was called beforehand.
    std::map<std::string, double> mapReject;
    mapReject.emplace("eog", 100e-06);
    mapReject.emplace("grad", 3000e-13);
    mapReject.emplace("mag", 3.5e-12);

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
                                     fTMin,
                                     fTMax);
    InvSourceEstimate sourceEstimateEvoked;

    StringList exclude = raw.info.bads;
    for (const std::string& name : raw.info.ch_names)
        if (name.find("EOG") != std::string::npos)
            exclude.push_back(name);

    if (!bDoSourceLoc) {
        // Pick relevant channels
        if (sChType.contains("EEG", Qt::CaseInsensitive)) {
            picks = raw.info.pick_types(false, true, false, {}, exclude);
        } else if (sCoilType.contains("grad", Qt::CaseInsensitive)) {
            // Only pick every second gradiometer which are not marked as bad.
            RowVectorXi picksTmp = raw.info.pick_types("grad", false, false);
            picks.resize(0);

            for (int i = 0; i < picksTmp.cols() - 1; i += 2) {
                if (!std::ranges::contains(raw.info.bads, raw.info.ch_names.at(picksTmp(i)))) {
                    picks.conservativeResize(picks.cols() + 1);
                    picks(picks.cols() - 1) = picksTmp(i);
                }
            }
        } else if (sCoilType.contains("mag", Qt::CaseInsensitive)) {
            picks = raw.info.pick_types("mag", false, false, {}, exclude);
        }

        // Generate network nodes
        conSettings.setNodePositions(raw.info, picks);

        // Transform to a more generic data matrix list, pick only channels of interest and remove EOG channel
        MatrixXd matData;
        int iNumberRows = picks.cols(); //picks.cols() 32

        for (int i = 0; i < std::ssize(data); ++i) {
            matData.resize(iNumberRows, data.at(i)->epoch.cols());

            for (qint32 j = 0; j < iNumberRows; ++j) {
                matData.row(j) = data.at(i)->epoch.row(picks(j));
            }
            matDataList << matData;
        }
    } else {
        //Create source level data
        t_Fwd = MNEForwardSolution(sFwd.toStdU16String(), {.surf_ori = true});

        // Load data
        InvSourceEstimate sourceEstimate;

        double lambda2 = 1.0 / pow(dSnr, 2);
        QString method(sSourceLocMethod);

        // regularize noise covariance
        noise_cov = noise_cov.regularize(raw.info,
                                         0.05,
                                         0.05,
                                         0.1,
                                         true);

        // Cluster forward solution;
        if (bDoClust) {
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
        InvMinimumNorm minimumNorm(inverse_operator, lambda2, method.toStdString());
        minimumNorm.doInverseSetup(1, true);

        picks = raw.info.pick_types("all", true, false, {}, exclude);
        data.pick_channels(picks);
        for (int i = 0; i < std::ssize(data); i++) {
            sourceEstimate = minimumNorm.calculateInverse(data.at(i)->epoch,
                                                          0.0f,
                                                          1.0 / raw.info.sfreq,
                                                          true);

            if (sourceEstimate.empty()) {
                printf("Source estimate is empty");
            } else {
                matDataList << sourceEstimate.data;
            }
        }

        InvMinimumNorm minimumNormEvoked(inverse_operator, lambda2, method.toStdString());
        sourceEstimateEvoked = minimumNormEvoked.calculateInverse(evoked, false);

        // Generate network nodes
        conSettings.setNodePositions(t_clusteredFwd, tSurfSetInflated);
    }

    // Compute the connectivity estimates for the methods to be compared
    conSettings.setConnectivityMethods({"COH", "COR", "XCOR", "PLI", "IMAGCOH", "PLV", "WPLI", "USPLI", "DSWPLI"});

    iNumTrials = qMin(iNumTrials, matDataList.size());
    for (int i = 0; i < iNumTrials; i++) {
        // Only calculate connectivity for post stim
        int samplesToCutOut = abs(fTMin * raw.info.sfreq);
        conSettings.append(matDataList.at(i).block(0,
                                                   samplesToCutOut,
                                                   matDataList.at(i).rows(),
                                                   matDataList.at(i).cols() - samplesToCutOut));
    }

    conSettings.setSamplingFrequency(raw.info.sfreq);
    conSettings.setWindowType("hanning");

    std::vector<Network> lNetworks = compute_connectivity(conSettings);

    std::map<std::string, Vector4i> mColor;
    mColor["COR"] = Vector4i(90, 26, 100, 255);
    mColor["XCOR"] = Vector4i(255, 20, 80, 255);
    mColor["PLI"] = Vector4i(255, 255, 0, 255);
    mColor["COH"] = Vector4i(2, 89, 100, 255);
    mColor["IMAGCOH"] = Vector4i(50, 255, 48, 255);
    mColor["PLV"] = Vector4i(0, 255, 255, 255);
    mColor["WPLI"] = Vector4i(255, 0, 100, 255);
    mColor["USPLI"] = Vector4i(255, 89, 200, 255);
    mColor["DSWPLI"] = Vector4i(25, 10, 255, 255);

    for (int j = 0; j < std::ssize(lNetworks); ++j) {
        lNetworks[j].set_frequency_range(dFreqMin, dFreqMax);
        lNetworks[j].normalize();
        VisualizationInfo visInfo = lNetworks[j].visualization_info();
        visInfo.coloring = VisualizationInfo::Coloring::fixed;
        visInfo.node_color = mColor[lNetworks[j].method()];
        visInfo.edge_color = visInfo.node_color;
        lNetworks[j].set_visualization_info(std::move(visInfo));
    }

    // Create BrainView for visualization
    BrainView* pBrainView = new BrainView();
    BrainTreeModel* pModel = new BrainTreeModel();
    pBrainView->setModel(pModel);

    // Load all networks
    for (int j = 0; j < std::ssize(lNetworks); ++j) {
        pBrainView->loadNetwork(lNetworks[j], QString::fromStdString(lNetworks[j].method()));
    }
    pBrainView->setNetworkThreshold(0.9);

    // Create ConnectivitySettingsView as standalone widget
    std::shared_ptr<ConnectivitySettingsView> pConnSettingsView = std::make_shared<ConnectivitySettingsView>("EX_CONNECTIVITY_COMP");
    pConnSettingsView->setNumberTrials(iNumTrials);
    pConnSettingsView->setFrequencyBand(dFreqMin, dFreqMax);

    //Read and show sensor helmets
    if (!bDoSourceLoc && sChType.contains("meg", Qt::CaseInsensitive)) {
        pBrainView->loadSensors((QCoreApplication::applicationDirPath() + "/../resources/general/sensorSurfaces/306m_rt.fif").toStdU16String());
    } else {
        // Add source estimate for source-level visualization
        int nVertLh = t_clusteredFwd.src[0].nuse;
        InvSourceEstimate stcLh, stcRh;
        stcLh.data = sourceEstimateEvoked.data.topRows(nVertLh);
        stcLh.vertices = sourceEstimateEvoked.vertices.head(nVertLh);
        stcLh.tmin = sourceEstimateEvoked.tmin;
        stcLh.tstep = sourceEstimateEvoked.tstep;
        stcLh.times = sourceEstimateEvoked.times;
        stcRh.data = sourceEstimateEvoked.data.bottomRows(sourceEstimateEvoked.data.rows() - nVertLh);
        stcRh.vertices = sourceEstimateEvoked.vertices.tail(sourceEstimateEvoked.vertices.size() - nVertLh);
        stcRh.tmin = sourceEstimateEvoked.tmin;
        stcRh.tstep = sourceEstimateEvoked.tstep;
        stcRh.times = sourceEstimateEvoked.times;

        QString tmpDir = QDir::tempPath();
        QString lhStcPath = tmpDir + "/mnecpp_connectivity_comparison-lh.stc";
        QString rhStcPath = tmpDir + "/mnecpp_connectivity_comparison-rh.stc";
        if (auto written = stcLh.write(lhStcPath.toStdU16String()); !written) {
            Log::warning("{}", written.error().what());
        }
        if (auto written = stcRh.write(rhStcPath.toStdU16String()); !written) {
            Log::warning("{}", written.error().what());
        }

        // Add hemisphere surfaces
        for (auto it = tSurfSetInflated.data().cbegin(); it != tSurfSetInflated.data().cend(); ++it) {
            int hIdx = it->first;
            QString hemi = (it->second.hemi() == 0) ? "lh" : "rh";
            QString surfType = QString::fromStdString(it->second.surf().empty() ? "inflated" : it->second.surf());
            pModel->addSurface("sample", hemi, surfType, it->second);
            if (tAnnotSet.size() > hIdx)
                pModel->addAnnotation("sample", hemi, tAnnotSet[hIdx]);
        }

        // Load source estimate and configure
        QObject::connect(pBrainView, &BrainView::sourceEstimateLoaded, [&](int /*nTimePoints*/) {
            pBrainView->setSourceColormap("Jet");
            pBrainView->setSourceThresholds(0.0f, 0.5f, 10.0f);
        });
        pBrainView->loadSourceEstimate(lhStcPath.toStdU16String(), rhStcPath.toStdU16String());
    }

    QObject::connect(pConnSettingsView.get(), &ConnectivitySettingsView::numberTrialsChanged,
                     [&pConnSettingsView, iNumTrials]() { pConnSettingsView->setNumberTrials(iNumTrials); });

    QObject::connect(pConnSettingsView.get(), &ConnectivitySettingsView::freqBandChanged,
                     [&pConnSettingsView, dFreqMin, dFreqMax]() { pConnSettingsView->setFrequencyBand(dFreqMin, dFreqMax); });

    pConnSettingsView->show();
    pBrainView->show();

    return a.exec();
}
