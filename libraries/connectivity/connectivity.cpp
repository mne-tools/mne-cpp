//=============================================================================================================
/**
* @file     connectivity.cpp
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     March, 2017
*
* @section  LICENSE
*
* Copyright (C) 2017, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
* @brief    Connectivity class definition.
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "connectivity.h"

#include "connectivitysettings.h"
#include "network/network.h"
#include "connectivitymeasures.h"

#include <fs/label.h>
#include <fs/annotationset.h>

#include <mne/mne_sourceestimate.h>
#include <mne/mne_epoch_data_list.h>
#include <mne/mne.h>

#include <inverse/minimumNorm/minimumnorm.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace CONNECTIVITYLIB;
using namespace FSLIB;
using namespace MNELIB;
using namespace INVERSELIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Connectivity::Connectivity(const ConnectivitySettings& connectivitySettings)
: m_pConnectivitySettings(ConnectivitySettings::SPtr(new ConnectivitySettings(connectivitySettings)))
{
}


//*************************************************************************************************************

Network Connectivity::calculateConnectivity() const
{
    MNEEpochDataList epochDataList;
    MatrixX3f matNodePos;

    if(m_pConnectivitySettings->m_bDoSourceLoc) {
        //generateSourceLevelData(matData, matNodePos);
    } else {
        generateSensorLevelData(epochDataList, matNodePos);
    }

    if(m_pConnectivitySettings->m_sConnectivityMethod == "COR") {
        return ConnectivityMeasures::pearsonsCorrelationCoeff(epochDataList, matNodePos);
    } else if(m_pConnectivitySettings->m_sConnectivityMethod == "XCOR") {
        return ConnectivityMeasures::crossCorrelation(epochDataList, matNodePos);
    } else if(m_pConnectivitySettings->m_sConnectivityMethod == "PLI") {
        return ConnectivityMeasures::phaseLagIndex(matData, matNodePos);
    }


    return Network();
}


//*************************************************************************************************************

void Connectivity::generateSensorLevelData(MNEEpochDataList& epochDataList, MatrixX3f& matNodePos) const
{
    matNodePos.resize(0,0);
    epochDataList.clear();

    // Load data
    QFile t_fileRaw(m_pConnectivitySettings->m_sRaw);
    qint32 event = m_pConnectivitySettings->m_iAveIdx;
    QString t_sEventName = m_pConnectivitySettings->m_sEve;
    float tmin = m_pConnectivitySettings->m_dTMin;
    float tmax = m_pConnectivitySettings->m_dTMax;
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
    } else {
        QStringList include;
        bool want_meg, want_eeg, want_stim;

        if(m_pConnectivitySettings->m_sChType == "meg") {
            want_meg = true;
            want_eeg = false;
            want_stim = false;
        } else if (m_pConnectivitySettings->m_sChType == "eeg") {
            want_meg = false;
            want_eeg = true;
            want_stim = false;
        }

        picks = raw.info.pick_types(m_pConnectivitySettings->m_sCoilType, want_eeg, want_stim, include, raw.info.bads);
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
            return;
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
            return;
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
                return;
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
        return;
    }

    fiff_int_t event_samp, from, to;
    MatrixXd timesDummy;

    MNEEpochData::SPtr pEpoch;
    MatrixXd times;

    for (p = 0; p < count; ++p) {
        // Read a data segment
        event_samp = events(selected(p),0);
        from = event_samp + tmin*raw.info.sfreq;
        to = event_samp + floor(tmax*raw.info.sfreq + 0.5);

        pEpoch = MNEEpochData::SPtr(new MNEEpochData());

        if(raw.read_raw_segment(pEpoch->epoch, timesDummy, from, to, picks)) {
            if (p == 0) {
                times.resize(1, to-from+1);
                for (qint32 i = 0; i < times.cols(); ++i)
                    times(0, i) = ((float)(from-event_samp+i)) / raw.info.sfreq;
            }

            pEpoch->event = event;
            pEpoch->tmin = ((float)(from)-(float)(raw.first_samp))/raw.info.sfreq;
            pEpoch->tmax = ((float)(to)-(float)(raw.first_samp))/raw.info.sfreq;

            epochDataList.append(pEpoch);
        } else  {
            printf("Can't read the event data segments");
            return;
        }
    }

    // Generate 3D node for 3D network visualization
    int counter = 0;

    for(int i = 0; i < raw.info.chs.size(); ++i) {
        if (pickedChNames.contains(raw.info.chs.at(i).ch_name)) {
            // Get the 3D positions
            matNodePos.conservativeResize(matNodePos.rows()+1, 3);
            matNodePos(counter,0) = raw.info.chs.at(i).chpos.r0(0);
            matNodePos(counter,1) = raw.info.chs.at(i).chpos.r0(1);
            matNodePos(counter,2) = raw.info.chs.at(i).chpos.r0(2);

            counter++;
        }
    }
}


//*************************************************************************************************************

void Connectivity::generateSourceLevelData(MatrixXd& matData, MatrixX3f& matNodePos) const
{
    AnnotationSet tAnnotSet(m_pConnectivitySettings->m_sSubj,
                             2,
                             m_pConnectivitySettings->m_sAnnotType,
                             m_pConnectivitySettings->m_sSubjDir);

    QFile t_fileFwd(m_pConnectivitySettings->m_sFwd);
    MNEForwardSolution t_Fwd(t_fileFwd);
    MNEForwardSolution t_clusteredFwd;

    QFile t_fileCov(m_pConnectivitySettings->m_sCov);
    QFile t_fileEvoked(m_pConnectivitySettings->m_sAve);

    // Load data
    QPair<QVariant, QVariant> baseline(QVariant(), 0);
    MNESourceEstimate sourceEstimate;
    FiffEvoked evoked(t_fileEvoked, m_pConnectivitySettings->m_iAveIdx, baseline);

    double snr = m_pConnectivitySettings->m_dSnr;
    double lambda2 = 1.0 / pow(snr, 2);
    QString method(m_pConnectivitySettings->m_sSourceLocMethod);

    t_fileEvoked.close();

    if(evoked.isEmpty() || t_Fwd.isEmpty()) {
        return;
    }

    FiffCov noise_cov(t_fileCov);

    // regularize noise covariance
    noise_cov = noise_cov.regularize(evoked.info, 0.05, 0.05, 0.1, true);

    // Cluster forward solution;
    if(m_pConnectivitySettings->m_bDoClust) {
        t_clusteredFwd = t_Fwd.cluster_forward_solution(tAnnotSet, 40);
    } else {
        t_clusteredFwd = t_Fwd;
    }

    // make an inverse operators
    FiffInfo info = evoked.info;

    MNEInverseOperator inverse_operator(info, t_clusteredFwd, noise_cov, 0.2f, 0.8f);

    // Compute inverse solution
    MinimumNorm minimumNorm(inverse_operator, lambda2, method);
    sourceEstimate = minimumNorm.calculateInverse(evoked);

    if(sourceEstimate.isEmpty()) {
        return;
    }

    matData = sourceEstimate.data;

    //Generate node vertices
    MatrixX3f matNodeVertLeft, matNodeVertRight;

    if(m_pConnectivitySettings->m_bDoClust) {
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

    matNodePos.resize(matNodeVertLeft.rows()+matNodeVertRight.rows(),3);
    matNodePos << matNodeVertLeft, matNodeVertRight;
}
