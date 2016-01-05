//=============================================================================================================
/**
* @file     main.cpp
* @author   Jana Kiesel <jana.kiesel@tu-ilmenau.de>
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     November, 2015
* @section  LICENSE
*
* Copyright (C) 2015, Jana Kiesel, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Example to test the warp class
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

//#include <fstream>
#include <iostream>
#include <cstdlib>
#include <time.h>

#include <utils/warp.h>
#include <fiff/fiff.h>
#include <mne/mne_bem.h>

#include <QFile>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore/QCoreApplication>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;
using namespace FIFFLIB;

using namespace std;


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

#define MAXBUFSIZE  ((int) 1e6)

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

//    Matrix<double,6,3> sLm;
//    sLm <<  1,1,1,
//            2,1,1,
//            3,1,1,
//            1,2,1,
//            2,2,1,
//            3,2,1;
//    std::cout << "Here is the matrix sLm:" << std::endl << sLm << std::endl;

//    Matrix<double,6,3> dLm;
//    dLm <<  1,1,2,
//            2,1,3,
//            3,1,2,
//            1,2,2,
//            2,2,3,
//            3,2,2;

//    Matrix<double,16,3> sVert;
//    sVert <<1.0,0.5,1,
//            1.5,0.5,1,
//            2.0,0.5,1,
//            2.3,0.5,1,
//            2.7,0.5,1,
//            3.0,0.5,1,
//            3.5,0.5,1,
//            4.0,0.5,1,
//            1.0,1,1,
//            1.5,1,1,
//            2.0,1,1,
//            2.3,1,1,
//            2.7,1,1,
//            3.0,1,1,
//            3.5,1,1,
//            4.0,1,1;;

    //
    // Read Electrode Positions from fiff
    //
    QFile t_fileRaw("./MNE-sample-data/MEG/sample/sample_audvis_raw.fif");
    FiffRawData raw(t_fileRaw);
    QList<FiffChInfo> ChannelInfo= raw.info.chs;
    QList<QVector<double>> tempElecInfo;
    QList<QString> ElecChanName;
    for (int i=0; i<ChannelInfo.length();i++)
    {
        if (ChannelInfo[i].kind==2)
        {
            QVector<double> temp;
            ElecChanName.append(ChannelInfo[i].ch_name);
            temp.append(ChannelInfo[i].loc(0,0));
            temp.append(ChannelInfo[i].loc(1,0));
            temp.append(ChannelInfo[i].loc(2,0));
            tempElecInfo.append(temp);
        }
    }
    MatrixXd ElecPos(tempElecInfo.length(),3);
    for (int i=0; i<tempElecInfo.length();i++)
    {
        ElecPos(i,0)=tempElecInfo[i][0];
        ElecPos(i,1)=tempElecInfo[i][1];
        ElecPos(i,2)=tempElecInfo[i][2];
    }

//    std::cout << "Here is the matrix ElecPos:" << std::endl << ElecPos << std::endl;

    //
    // read electrode positions from Database
    //
//    QString electrodeFileName= "./MNE-sample-data/MriDatabase/FirstYear/Sources/Electrodes/AVG3-0Months_10-5_Electrodes.txt";
//    Matrix<double, Dynamic ,3> ElecPos = test.readsLm(electrodeFileName);
//    std::cout << "Here is the matrix ElecPos:" << std::endl << ElecPos << std::endl;

    //
    //Read BEM from fiff
    //
    QFile t_fileBem("./MNE-sample-data/subjects/sample/bem/sample-5120-5120-5120-bem.fif");
    MNELIB::MNEBem t_Bem (t_fileBem) ;

    //
    // prepare warp
    //
    MatrixXd sLm=ElecPos;
    MatrixXd random(ElecPos.rows(),3);
    srand(time(NULL));
    for (int i=0;i<ElecPos.rows();i++)
    {

        for (int j=0; j<3;j++)
        {
            random(i,j)=rand();
            random(i,j)/=RAND_MAX;
            random(i,j)*=0.06;
            random(i,j)-=0.03;
            //            random(i,j)=0.005*(rand()/RAND_MAX);
        }
    }
//    std::cout << "Here are the first row of the matrix random:" << std::endl << random.topRows(9) << std::endl;
    MatrixXd dLm=sLm+random;
    MNELIB::MNEBemSurface skin=t_Bem[0];
    MatrixXd sVert=skin.rr.cast<double>();

    std::cout << "Here are the first row of the matrix skin.rr bevor warp:" << std::endl << skin.rr.topRows(9) << std::endl;


    //
    // calculate Warp
    //
    Warp test;
    MatrixXd wVert(sVert.rows(),3);
    wVert = test.calculate(sLm, dLm, sVert);

    //
    // WRITE NEW VERTICES BACK TO BEM
    //
    skin.rr=wVert.cast<float>();
    skin.addVertexNormals();

    std::cout << "Here are the first row of the matrix skin.rr after warp:" << std::endl << skin.rr.topRows(9) << std::endl;
//    std::cout << "Here is the first row of the final matrix skin.tris:" << std::endl << skin.tris.topRows(9) << std::endl;
//    std::cout << "Here is the last row of the final matrix skin.tris:" << std::endl << skin.tris.bottomRows(1) << std::endl;

    MNELIB::MNEBem t_BemWarpedA;
    t_BemWarpedA<<skin;
    QFile t_fileBemWarped("./MNE-sample-data/subjects/sample/bem/sample-5120-5120-5120-bem-warped.fif");
    t_BemWarpedA.write(t_fileBemWarped);
    t_fileBemWarped.close();

    MNELIB::MNEBem t_BemWarpedB (t_fileBemWarped) ;
    MNELIB::MNEBemSurface skinWarped=t_BemWarpedB[0];
    return a.exec();
}
