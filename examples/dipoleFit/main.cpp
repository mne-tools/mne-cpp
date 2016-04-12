//=============================================================================================================
/**
* @file     main.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     April, 2016
*
* @section  LICENSE
*
* Copyright (C) 2016, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Example of dipole fit
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <iostream>
#include <vector>
#include <math.h>


#include <fiff/fiff.h>
#include <mne/mne.h>
#include <utils/sphere.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore/QCoreApplication>
#include <QCommandLineParser>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

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
    QCoreApplication app(argc, argv);




//    // Command Line Parser
//    QCommandLineParser parser;
//    parser.setApplicationDescription("Clustered Inverse Example");
//    parser.addHelpOption();

//    QCommandLineOption sampleEvokedFileOption("e", "Path to evoked <file>.", "file", "./MNE-sample-data/MEG/sample/sample_audvis-ave.fif");
//    QCommandLineOption sampleCovFileOption("c", "Path to covariance <file>.", "file", "./MNE-sample-data/MEG/sample/sample_audvis-cov.fif");
//    QCommandLineOption sampleBemFileOption("b", "Path to BEM <file>.", "file", "./MNE-sample-data/subjects/sample/bem/sample-5120-bem-sol.fif");
//    QCommandLineOption sampleTransFileOption("t", "Path to trans <file>.", "file", "./MNE-sample-data/MEG/sample/sample_audvis_raw-trans.fif");
//    parser.addOption(sampleEvokedFileOption);
//    parser.addOption(sampleCovFileOption);
//    parser.addOption(sampleBemFileOption);
//    parser.addOption(sampleTransFileOption);
//    parser.process(app);

//    //########################################################################################
//    // Source Estimate
//    QFile fileEvoked(parser.value(sampleEvokedFileOption));
//    QFile fileCov(parser.value(sampleCovFileOption));
//    QFile fileBem(parser.value(sampleBemFileOption));
//    QFile fileTrans(parser.value(sampleTransFileOption));

//    // === Load data ===
//    // Evoked
//    std::cout << std::endl << "### Evoked ###" << std::endl;
//    fiff_int_t setno = 0;
//    QPair<QVariant, QVariant> baseline(QVariant(), 0);
//    FiffEvoked evoked(fileEvoked, setno, baseline);
//    if(evoked.isEmpty())
//        return 1;

//    // Cov
//    std::cout << std::endl << "### Covariance ###" << std::endl;
//    FiffCov noise_cov(fileCov);

//    // BEM
//    std::cout << std::endl << "### BEM ###" << std::endl;
//    MNEBem bem(fileBem);
//    if( bem.isEmpty() ) {
//        return -1;
//    }

//    // Trans
//    std::cout << std::endl << "### Transformation ###" << std::endl;
//    FiffCoordTrans trans(fileTrans);
//    if( trans.isEmpty() )
//    {
//        trans.from = FIFFV_COORD_HEAD;
//        trans.to = FIFFV_COORD_MRI;
//    }

//    // === Dipole Fit ===

//    //FIFFV_BEM_SURF_ID_BRAIN      1 -> Inner Skull
//    //FIFFV_BEM_SURF_ID_SKULL      3 -> Outer Skull
//    //FIFFV_BEM_SURF_ID_HEAD       4 -> Head
//    qDebug() << "bem" << bem[0].id;


//    Eigen::MatrixX3d testMat(3,3);

//    testMat <<  1, 3, 2,
//                3, 5, 1,
//                8, 7, 9;


    Eigen::MatrixX3d testMat(30,3);
    testMat << 0.537667139546100, 0.888395631757642, -1.08906429505224,
            1.83388501459509, -1.14707010696915, 0.0325574641649735,
            -2.25884686100365, -1.06887045816803, 0.552527021112224,
            0.862173320368121, -0.809498694424876, 1.10061021788087,
            0.318765239858981, -2.94428416199490, 1.54421189550395,
            -1.30768829630527, 1.43838029281510, 0.0859311331754255,
            -0.433592022305684, 0.325190539456198, -1.49159031063761,
            0.342624466538650, -0.754928319169703, -0.742301837259857,
            3.57839693972576, 1.37029854009523, -1.06158173331999,
            2.76943702988488, -1.71151641885370, 2.35045722400204,
            -1.34988694015652, -0.102242446085491, -0.615601881466894,
            3.03492346633185, -0.241447041607358, 0.748076783703985,
            0.725404224946106, 0.319206739165502, -0.192418510588264,
            -0.0630548731896562, 0.312858596637428, 0.888610425420721,
            0.714742903826096, -0.864879917324457, -0.764849236567874,
            -0.204966058299775, -0.0300512961962686, -1.40226896933876,
            -0.124144348216312, -0.164879019209038, -1.42237592509150,
            1.48969760778547, 0.627707287528727, 0.488193909859941,
            1.40903448980048, 1.09326566903948, -0.177375156618825,
            1.41719241342961, 1.10927329761440, -0.196053487807333,
            0.671497133608081, -0.863652821988714, 1.41931015064255,
            -1.20748692268504, 0.0773590911304249, 0.291584373984183,
            0.717238651328839, -1.21411704361541, 0.197811053464361,
            1.63023528916473, -1.11350074148676, 1.58769908997406,
            0.488893770311789, -0.00684932810334806, -0.804465956349547,
            1.03469300991786, 1.53263030828475, 0.696624415849607,
            0.726885133383238, -0.769665913753682, 0.835088165072682,
            -0.303440924786016, 0.371378812760058, -0.243715140377952,
            0.293871467096658, -0.225584402271252, 0.215670086403744,
            -0.787282803758638, 1.11735613881447, -1.16584393148205;

//    std::cout << "testMat" << std::endl << testMat << std::endl;

    Sphere sp = Sphere::fit_sphere(testMat);

    std::cout << "center" << std::endl << sp.center() << std::endl;
    std::cout << "radius" << std::endl << sp.radius() << std::endl;

    return app.exec();
}

//*************************************************************************************************************
//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================
