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

#include <disp3D/adapters/networkview.h>

#include <connectivity/connectivity.h>
#include <connectivity/connectivitysettings.h>
#include <connectivity/connectivitymeasures.h>
#include <connectivity/network/network.h>

#include <mne/mne_epoch_data_list.h>
#include <mne/mne.h>

#include <fiff/fiff_raw_data.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QApplication>
#include <QMainWindow>
#include <QCommandLineParser>
#include <QDebug>
#include <QFile>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISP3DLIB;
using namespace INVERSELIB;
using namespace Eigen;
using namespace CONNECTIVITYLIB;
using namespace FIFFLIB;
using namespace MNELIB;


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

    //Do connectivity estimation and visualize results
    ConnectivitySettings settings(QApplication::arguments());
    settings.m_sConnectivityMethod = "PLI";
    settings.m_sChType = "eeg";
    settings.m_sCoilType = "grad";


    // Load data
    MNEEpochDataList epochDataList;
    MatrixX3f matNodePos;

    QFile t_fileRaw(settings.m_sRaw);
    qint32 event = settings.m_iAveIdx;
    QString t_sEventName = settings.m_sEve;
    float tmin = settings.m_dTMin;
    float tmax = settings.m_dTMax;
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

        if(settings.m_sChType == "meg") {
            want_meg = true;
            want_eeg = false;
            want_stim = false;

            picks = raw.info.pick_types(settings.m_sCoilType, want_eeg, want_stim, include, raw.info.bads);
        } else if (settings.m_sChType == "eeg") {
            want_meg = false;
            want_eeg = true;
            want_stim = false;

            picks = raw.info.pick_types(want_meg, want_eeg, want_stim, include, raw.info.bads);
        }
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

    // Choose the connectivity measure and perform the actual computation
    Network finalNetwork;

    if(settings.m_sConnectivityMethod == "COR") {
        finalNetwork = ConnectivityMeasures::pearsonsCorrelationCoeff(epochDataList, matNodePos);
    } else if(settings.m_sConnectivityMethod == "XCOR") {
        finalNetwork = ConnectivityMeasures::crossCorrelation(epochDataList, matNodePos);
    } else if(settings.m_sConnectivityMethod == "PLI") {
        finalNetwork = ConnectivityMeasures::phaseLagIndex(epochDataList, matNodePos);
    }

    // Visualize the network
    NetworkView tNetworkView(finalNetwork);
    tNetworkView.show();

    return a.exec();
}
