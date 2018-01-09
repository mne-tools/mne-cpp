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

#include <fiff/fiff_evoked.h>


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
    settings.m_sConnectivityMethod = "COR";
    settings.m_sChType = "meg";
    settings.m_sCoilType = "mag";

    // Load the data
    MatrixXd matData;
    MatrixX3f matNodePos;
    matData.resize(0,0);
    matData.resize(0,0);

    QPair<QVariant, QVariant> baseline(QVariant(), 0);
    QFile t_fileEvoked(settings.m_sMeas);
    FiffEvoked evoked(t_fileEvoked, settings.m_iAveIdx, baseline);

    bool bPick = false;
    qint32 unit;
    int counter = 0;

    for(int i = 0; i < evoked.info.chs.size(); ++i) {
        unit = evoked.info.chs.at(i).unit;

        if(unit == FIFF_UNIT_T_M &&
            settings.m_sChType == "meg" &&
            settings.m_sCoilType == "grad") {
            bPick = true;
        } else if(unit == FIFF_UNIT_T &&
                    settings.m_sChType == "meg" &&
                    settings.m_sCoilType == "mag") {
            bPick = true;
        } else if (unit == FIFF_UNIT_V &&
                    settings.m_sChType == "eeg") {
            bPick = true;
        }

        if(bPick) {
            //Get the data
            matData.conservativeResize(matData.rows()+1, evoked.data.cols());
            matData.row(counter) = evoked.data.row(i);

            //Get the positions
            matNodePos.conservativeResize(matNodePos.rows()+1, 3);
            matNodePos(counter,0) = evoked.info.chs.at(i).chpos.r0(0);
            matNodePos(counter,1) = evoked.info.chs.at(i).chpos.r0(1);
            matNodePos(counter,2) = evoked.info.chs.at(i).chpos.r0(2);

            counter++;
        }

        bPick = false;
    }

    // Choose the connectivity measure and perform the actual computation
    Network finalNetwork;

    if(settings.m_sConnectivityMethod == "COR") {
        finalNetwork = ConnectivityMeasures::pearsonsCorrelationCoeff(matData, matNodePos);
    } else if(settings.m_sConnectivityMethod == "XCOR") {
        finalNetwork = ConnectivityMeasures::crossCorrelation(matData, matNodePos);
    } else if(settings.m_sConnectivityMethod == "PLI") {
        finalNetwork = ConnectivityMeasures::phaseLagIndex(matData, matNodePos);
    }

    // Visualize the network
    NetworkView tNetworkView(finalNetwork);
    tNetworkView.show();

    return a.exec();
}
