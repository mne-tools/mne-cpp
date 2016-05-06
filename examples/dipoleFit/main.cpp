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


    // Command Line Parser
    QCommandLineParser parser;
    parser.setApplicationDescription("Clustered Inverse Example");
    parser.addHelpOption();

    QCommandLineOption sampleEvokedFileOption("e", "Path to evoked <file>.", "file", "./MNE-sample-data/MEG/sample/sample_audvis-ave.fif");
    QCommandLineOption sampleCovFileOption("c", "Path to covariance <file>.", "file", "./MNE-sample-data/MEG/sample/sample_audvis-cov.fif");
    QCommandLineOption sampleBemFileOption("b", "Path to BEM <file>.", "file", "./MNE-sample-data/subjects/sample/bem/sample-5120-bem-sol.fif");
    QCommandLineOption sampleTransFileOption("t", "Path to trans <file>.", "file", "./MNE-sample-data/MEG/sample/sample_audvis_raw-trans.fif");
    parser.addOption(sampleEvokedFileOption);
    parser.addOption(sampleCovFileOption);
    parser.addOption(sampleBemFileOption);
    parser.addOption(sampleTransFileOption);
    parser.process(app);

    //########################################################################################
    // Source Estimate
    QFile fileEvoked(parser.value(sampleEvokedFileOption));
    QFile fileCov(parser.value(sampleCovFileOption));
    QFile fileBem(parser.value(sampleBemFileOption));
    QFile fileTrans(parser.value(sampleTransFileOption));

    // === Load data ===
    // Evoked
    std::cout << std::endl << "### Evoked ###" << std::endl;
    fiff_int_t setno = 0;
    QPair<QVariant, QVariant> baseline(QVariant(), 0);
    FiffEvoked evoked(fileEvoked, setno, baseline);
    if(evoked.isEmpty())
        return 1;

    // Cov
    std::cout << std::endl << "### Covariance ###" << std::endl;
    FiffCov noise_cov(fileCov);

    // BEM
    std::cout << std::endl << "### BEM ###" << std::endl;
    MNEBem bem(fileBem);
    if( bem.isEmpty() ) {
        return -1;
    }

    // Trans
    std::cout << std::endl << "### Transformation ###" << std::endl;
    FiffCoordTrans trans(fileTrans);
    if( trans.isEmpty() )
    {
        trans.from = FIFFV_COORD_HEAD;
        trans.to = FIFFV_COORD_MRI;
    }

    // === Dipole Fit ===

    //FIFFV_BEM_SURF_ID_BRAIN      1 -> Inner Skull
    //FIFFV_BEM_SURF_ID_SKULL      3 -> Outer Skull
    //FIFFV_BEM_SURF_ID_HEAD       4 -> Head
    qDebug() << "bem" << bem[0].id;

    Sphere sp = Sphere::fit_sphere( bem[0].rr );

    std::cout << "sp center" << std::endl << sp.center() << std::endl;
    std::cout << "sp radius" << std::endl << sp.radius() << std::endl;

    Sphere sp_simplex = Sphere::fit_sphere_simplex( bem[0].rr );

    std::cout << "sp simplex center" << std::endl << sp_simplex.center() << std::endl;
    std::cout << "sp simplex radius" << std::endl << sp_simplex.radius() << std::endl;




    return app.exec();
}

//*************************************************************************************************************
//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================
