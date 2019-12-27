//=============================================================================================================
/**
* @file     main.cpp
* @author   Gabriel Motta <gbmotta@mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     December, 2019
*
* @section  LICENSE
*
* Copyright (C) 2019, Gabriel Motta and Matti Hamalainen. All rights reserved.
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
* @brief    Example of interfacing with the fieldtrip example buffer and sine2ft.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <math.h>
#include <disp/plots/plot.h>
#include <utils/spectral.h>
#include <mne/mne.h>


//*************************************************************************************************************
//=============================================================================================================
// Eigen
//=============================================================================================================

#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QApplication>
#include <QtMath>
#include <QTcpSocket>
#include <QHostAddress>
#include <QTcpServer>
#include <QTest>

//*************************************************************************************************************
//=============================================================================================================
// FT INCLUDES
//=============================================================================================================

#include <FtBuffer.h>
#include <MultiChannelFilter.h>
#include <buffer.h>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace DISPLIB;
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
    QCoreApplication a(argc, argv);

    qDebug() << "============== FieldTrip Buffer Example ==============";
    qDebug() << "Ensure the ft buffer source files are present and their respective 'make' files have been called.";
    //Create a parameter-less server
    //QTcpServer *ft_buff_server = new QTcpServer();

    //Get local host -- equvalent to just setting to 127.0.0.1
    QHostAddress *addr = new QHostAddress(QHostAddress::LocalHost);

    //Set port -- buffer example is configured to port 1972 by default
    int port = 1972;

    QTcpSocket *mysocket = new QTcpSocket();

/*
    qDebug() << "Attempting to connect to host on port" << port <<"on" << *addr;

    mysocket->connectToHost(*addr, port);

    qDebug() << "Connection state:" << mysocket->state();

    while (mysocket->state() != QAbstractSocket::ConnectedState){
        qDebug() << "Connecting, please wait.";
        QTest::qSleep(500);
    }
    qDebug() << mysocket->state();
*/
/*
    if (ft_buff_server->listen(*addr, port)){
        qDebug() << "Listening to port" << port <<"on" << *addr;
        while(1){
            if (ft_buff_server->hasPendingConnections()) {
                qDebug() << "Connection found on port" << port;
                QTcpSocket *ft_buff_socket = ft_buff_server->nextPendingConnection();
                qDebug() << "Socket created.";
            }
        }

    } else {
        qDebug() << "Something went wrong"; //Change this error message to something useful
    }

*/

    SimpleStorage chunkBuffer;
    headerdef_t header_def;
    FtBufferRequest request;
    FtBufferResponse response;
    FtConnection ftCon;

    request.prepGetHeader();

    if (tcprequest(ftCon.getSocket(), request.out(), response.in()) < 0) {
        qDebug() << "Error in communication - check buffer server";
    }

    if (!response.checkGetHeader(header_def, &chunkBuffer)) {
        qDebug() << "Could not read header.";
    }


    /*if (tcprequest(ftCon.getSocket(), request.out(), response.in()) < 0) {
        qDebug() << "Error in communication - check buffer server";
    }

    if (!response.checkGetHeader(header_def, &chunkBuffer)) {
        qDebug() << "Could not read header.";
    }
*/


    return a.exec();
}
