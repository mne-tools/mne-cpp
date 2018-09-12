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
#include <disp3D/engine/model/items/sourcespace/sourcespacetreeitem.h>

#include <fiff/fiff_raw_data.h>

#include <fs/label.h>
#include <fs/annotationset.h>

#include <mne/mne_sourceestimate.h>
#include <mne/mne_epoch_data_list.h>
#include <mne/mne.h>

#include <inverse/minimumNorm/minimumnorm.h>

#include <realtime/rtProcessing/rtconnectivity.h>

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
using namespace REALTIMELIB;


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
    QCommandLineOption subjectOption("subj", "Selected <subject> (for source level usage only).", "subject", "sample");
    QCommandLineOption subjectPathOption("subjDir", "Selected <subjectPath> (for source level usage only).", "subjectPath", QCoreApplication::applicationDirPath() + "/MNE-sample-data/subjects");
    QCommandLineOption fwdOption("fwd", "Path to forwad solution <file> (for source level usage only).", "file", QCoreApplication::applicationDirPath() + "/MNE-sample-data/MEG/sample/sample_audvis-meg-eeg-oct-6-fwd.fif");
    QCommandLineOption sourceLocOption("doSourceLoc", "Do source localization (for source level usage only).", "doSourceLoc", "true");
    QCommandLineOption clustOption("doClust", "Do clustering of source space (for source level usage only).", "doClust", "true");
    QCommandLineOption covFileOption("cov", "Path to the covariance <file> (for source level usage only).", "file", QCoreApplication::applicationDirPath() + "/MNE-sample-data/MEG/sample/sample_audvis-cov.fif");
    QCommandLineOption evokedFileOption("ave", "Path to the evoked/average <file>.", "file", QCoreApplication::applicationDirPath() + "/MNE-sample-data/MEG/sample/sample_audvis-ave.fif");
    QCommandLineOption sourceLocMethodOption("sourceLocMethod", "Inverse estimation <method> (for source level usage only), i.e., 'MNE', 'dSPM' or 'sLORETA'.", "method", "dSPM");
    QCommandLineOption connectMethodOption("connectMethod", "Connectivity <method>, i.e., 'COR', 'XCOR.", "method", "IMAGCOH");
    QCommandLineOption snrOption("snr", "The SNR <value> used for computation (for source level usage only).", "value", "3.0");
    QCommandLineOption evokedIndexOption("aveIdx", "The average <index> to choose from the average file.", "index", "1");
    QCommandLineOption coilTypeOption("coilType", "The coil <type> (for sensor level usage only), i.e. 'grad' or 'mag'.", "type", "mag");
    QCommandLineOption chTypeOption("chType", "The channel <type> (for sensor level usage only), i.e. 'eeg' or 'meg'.", "type", "meg");
    QCommandLineOption eventsFileOption("eve", "Path to the event <file>.", "file", QCoreApplication::applicationDirPath() + "/MNE-sample-data/MEG/sample/sample_audvis_raw-eve.fif");
    QCommandLineOption rawFileOption("raw", "Path to the raw <file>.", "file", QCoreApplication::applicationDirPath() + "/MNE-sample-data/MEG/sample/sample_audvis_raw.fif");
    QCommandLineOption tMinOption("tmin", "The time minimum value for averaging in seconds relativ to the trigger onset.", "value", "-0.3");
    QCommandLineOption tMaxOption("tmax", "The time maximum value for averaging in seconds relativ to the trigger onset.", "value", "0.6");

    parser.addOption(annotOption);
    parser.addOption(subjectOption);
    parser.addOption(subjectPathOption);
    parser.addOption(fwdOption);
    parser.addOption(sourceLocOption);
    parser.addOption(clustOption);
    parser.addOption(covFileOption);
    parser.addOption(evokedFileOption);
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
    QString sAve = parser.value(evokedFileOption);
    QString sCoilType = parser.value(coilTypeOption);
    QString sChType = parser.value(chTypeOption);
    QString sEve = parser.value(eventsFileOption);
    QString sRaw = parser.value(rawFileOption);
    float fTMin = parser.value(tMinOption).toFloat();
    float fTMax = parser.value(tMaxOption).toFloat();

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

    double dSnr = parser.value(snrOption).toDouble();
    int iAveIdx = parser.value(evokedIndexOption).toInt();

    //Prepare the data
    QList<MatrixXd> matDataList;
    MatrixX3f matNodePositions;

    MNEForwardSolution t_clusteredFwd;
    MNEForwardSolution t_Fwd;

    QFile t_fileCov(sCov);
    FiffCov noise_cov(t_fileCov);

    // Create sensor level data
    QFile t_fileRaw(sRaw);
    qint32 event = iAveIdx;
    QString t_sEventName =sEve ;
    float tmin = fTMin;
    float tmax = fTMax;
    bool keep_comp = false;
    fiff_int_t dest_comp = 0;
    bool pick_all = false;
    qint32 k, p;

    // Setup for reading the raw data
    FiffRawData raw(t_fileRaw);

    RowVectorXi picks;

    if (pick_all) {
        // Pick all
        picks.resize(raw.info.nchan);

        for(k = 0; k < raw.info.nchan; ++k) {
            picks(k) = k;
        }
    } else if (!bDoSourceLoc) {
        QStringList include;
        bool want_meg, want_eeg, want_stim;

        if(sChType == "meg") {
            want_meg = true;
            want_eeg = false;
            want_stim = false;

            picks = raw.info.pick_types(sCoilType, want_eeg, want_stim, include, raw.info.bads);
        } else if (sChType == "eeg") {
            want_meg = false;
            want_eeg = true;
            want_stim = false;

            picks = raw.info.pick_types(want_meg, want_eeg, want_stim, include, raw.info.bads);
        }
    } else {
        picks = raw.info.pick_channels(raw.info.ch_names, noise_cov.names, noise_cov.bads);
    }

    QStringList pickedChNames;
    for(k = 0; k < picks.cols(); ++k) {
        pickedChNames << raw.info.ch_names[picks(0,k)];
    }

    // Set up projection
    if (raw.info.projs.size() == 0) {
        printf("No projector specified for these data\n");
    } else {
        // Activate the projection items
        for (k = 0; k < raw.info.projs.size(); ++k) {
            raw.info.projs[k].active = true;
        }

        // Create the projector
        fiff_int_t nproj = raw.info.make_projector(raw.proj);

        if (nproj == 0) {
            printf("The projection vectors do not apply to these channels\n");
        } else {
            printf("Created an SSP operator (subspace dimension = %d)\n",nproj);
        }
    }

    // Set up the CTF compensator
    qint32 current_comp = raw.info.get_current_comp();
    if(current_comp > 0) {
        printf("Current compensation grade : %d\n",current_comp);
    }

    if(keep_comp) {
        dest_comp = current_comp;
    }

    if (current_comp != dest_comp) {
        qDebug() << "This part needs to be debugged";
        if(MNE::make_compensator(raw.info, current_comp, dest_comp, raw.comp)) {
            raw.info.set_current_comp(dest_comp);
            printf("Appropriate compensator added to change to grade %d.\n",dest_comp);
        } else {
            printf("Could not make the compensator\n");
        }
    }

    // Read the events
    QFile t_EventFile;
    MatrixXi events;

    if (t_sEventName.size() == 0) {
        p = t_fileRaw.fileName().indexOf(".fif");

        if (p > 0) {
            t_sEventName = t_fileRaw.fileName().replace(p, 4, "-eve.fif");
        } else {
            printf("Raw file name does not end properly\n");
        }

        t_EventFile.setFileName(t_sEventName);
        MNE::read_events(t_EventFile, events);
        printf("Events read from %s\n",t_sEventName.toUtf8().constData());
    } else {
        // Binary file
        p = t_fileRaw.fileName().indexOf(".fif");
        if (p > 0)
        {
            t_EventFile.setFileName(t_sEventName);
            if(!MNE::read_events(t_EventFile, events))
            {
                printf("Error while read events.\n");
            }
            printf("Binary event file %s read\n",t_sEventName.toUtf8().constData());
        } else {
            // Text file
            printf("Text file %s is not supported jet.\n",t_sEventName.toUtf8().constData());
        }
    }

    // Select the desired events
    qint32 count = 0;
    MatrixXi selected = MatrixXi::Zero(1, events.rows());
    for (p = 0; p < events.rows(); ++p) {
        if (events(p,1) == 0 && events(p,2) == event) {
            selected(0,count) = p;
            ++count;
        }
    }

    selected.conservativeResize(1, count);
    if (count > 0) {
        printf("%d matching events found\n",count);
    } else {
        printf("No desired events found.\n");
    }

    fiff_int_t event_samp, from, to;
    MatrixXd timesDummy;

    MatrixXd matData;

    for (p = 0; p < count; ++p) {
        // Read a data segment
        event_samp = events(selected(p),0);
        from = event_samp + tmin*raw.info.sfreq;
        to = event_samp + floor(tmax*raw.info.sfreq + 0.5);

        if(raw.read_raw_segment(matData, timesDummy, from, to, picks)) {
            matDataList.append(matData);
        } else  {
            printf("Can't read the event data segments");
        }
    }

    if(!bDoSourceLoc) {
        // Generate 3D node for 3D network visualization
        int counter = 0;

        for(int i = 0; i < raw.info.chs.size(); ++i) {
            if (pickedChNames.contains(raw.info.chs.at(i).ch_name)) {
                // Get the 3D positions
                matNodePositions.conservativeResize(matNodePositions.rows()+1, 3);
                matNodePositions(counter,0) = raw.info.chs.at(i).chpos.r0(0);
                matNodePositions(counter,1) = raw.info.chs.at(i).chpos.r0(1);
                matNodePositions(counter,2) = raw.info.chs.at(i).chpos.r0(2);

                counter++;
            }
        }
    } else {
        //Create source level data
        AnnotationSet tAnnotSet(sSubj, 2, sAnnotType, sSubjDir);

        QFile t_fileFwd(sFwd);
        t_Fwd = MNEForwardSolution(t_fileFwd);

        // Load data
        QPair<QVariant, QVariant> baseline(QVariant(), 0);
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

        for(int i = 0; i < matDataList.size(); i++) {
            sourceEstimate = minimumNorm.calculateInverse(matDataList.at(i),
                                                          0.0f,
                                                          1/raw.info.sfreq);

            if(sourceEstimate.isEmpty()) {
                printf("Source estimate is empty");
            }

            matDataList.replace(i, sourceEstimate.data);
        }

        //Generate node vertices
        MatrixX3f matNodeVertLeft, matNodeVertRight;

        if(bDoClust) {
            matNodeVertLeft.resize(t_clusteredFwd.src[0].cluster_info.centroidVertno.size(),3);

            for(int j = 0; j < matNodeVertLeft.rows(); ++j) {
                matNodeVertLeft.row(j) = t_clusteredFwd.src[0].rr.row(t_clusteredFwd.src[0].cluster_info.centroidVertno.at(j));
            }

            matNodeVertRight.resize(t_clusteredFwd.src[1].cluster_info.centroidVertno.size(),3);
            for(int j = 0; j < matNodeVertRight.rows(); ++j) {
                matNodeVertRight.row(j) = t_clusteredFwd.src[1].rr.row(t_clusteredFwd.src[1].cluster_info.centroidVertno.at(j));
            }
        } else {
            matNodeVertLeft.resize(t_Fwd.src[0].vertno.rows(),3);
            for(int j = 0; j < matNodeVertLeft.rows(); ++j) {
                matNodeVertLeft.row(j) = t_clusteredFwd.src[0].rr.row(t_Fwd.src[0].vertno(j));
            }

            matNodeVertRight.resize(t_Fwd.src[1].vertno.rows(),3);
            for(int j = 0; j < matNodeVertRight.rows(); ++j) {
                matNodeVertRight.row(j) = t_clusteredFwd.src[1].rr.row(t_Fwd.src[1].vertno(j));
            }
        }

        matNodePositions.resize(matNodeVertLeft.rows()+matNodeVertRight.rows(),3);
        matNodePositions << matNodeVertLeft, matNodeVertRight;
    }

    //Do connectivity estimation and visualize results
    QSharedPointer<ConnectivitySettingsManager> pConnectivitySettingsManager = QSharedPointer<ConnectivitySettingsManager>::create();

    pConnectivitySettingsManager->m_settings.m_sConnectivityMethods << sConnectivityMethod;
    pConnectivitySettingsManager->m_settings.m_matDataList = matDataList;
    pConnectivitySettingsManager->m_settings.m_matNodePositions = matNodePositions;
    pConnectivitySettingsManager->m_settings.m_iNfft = -1;
    pConnectivitySettingsManager->m_settings.m_sWindowType = "hanning";

    //Create NetworkView and add extra control widgets to output data (will be used by QuickControlView in RealTimeConnectivityEstimateWidget)
    NetworkView tNetworkView;
    ConnectivitySettingsView* pConnectivitySettingsView = new ConnectivitySettingsView();
    QList<QWidget*> lWidgets;
    lWidgets << pConnectivitySettingsView;
    tNetworkView.setQuickControlWidgets(lWidgets);
    tNetworkView.show();

    QObject::connect(pConnectivitySettingsView, &ConnectivitySettingsView::connectivityMetricChanged,
                     pConnectivitySettingsManager.data(), &ConnectivitySettingsManager::onConnectivityMetricChanged);

    QObject::connect(pConnectivitySettingsManager->m_pRtConnectivity.data(), &RtConnectivity::newConnectivityResultAvailable,
                     &tNetworkView, &NetworkView::addData);

    if(bDoSourceLoc) {
        QList<SourceSpaceTreeItem*> pSourceSpaceTreeItem;

        if(bDoClust) {
            pSourceSpaceTreeItem = tNetworkView.getTreeModel()->addForwardSolution(parser.value(subjectOption), "ClusteredForwardSolution", t_clusteredFwd);
        } else {
            pSourceSpaceTreeItem = tNetworkView.getTreeModel()->addForwardSolution(parser.value(subjectOption), "FullForwardSolution", t_Fwd);
        }

        for(int i = 0; i < pSourceSpaceTreeItem.size(); i++) {
            pSourceSpaceTreeItem.at(i)->setAlpha(0.3f);
        }
    }

    pConnectivitySettingsManager->m_pRtConnectivity->append(pConnectivitySettingsManager->m_settings);

    return a.exec();
}
