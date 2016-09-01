//=============================================================================================================
/**
* @file     main.cpp
* @author   Jana Kiesel <jana.kiesel@tu-ilmenau.de>
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     Mai, 2015
*
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
* @brief    Example of reading BEM data
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <iostream>
#include <mne/mne.h>
#include <utils/ioutils.h>


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
    parser.setApplicationDescription("Read BEM Example");
    parser.addHelpOption();
    QCommandLineOption sampleBEMFileOption("f", "Path to BEM <file>.", "file", "./MNE-sample-data/subjects/sample/bem/sample-head.fif");
//    "./MNE-sample-data/subjects/sample/bem/sample-5120-5120-5120-bem.fif"
//    "./MNE-sample-data/subjects/sample/bem/sample-all-src.fif"
//    "./MNE-sample-data/subjects/sample/bem/sample-5120-bem-sol.fif"
//    "./MNE-sample-data/subjects/sample/bem/sample-5120-bem.fif"
    parser.addOption(sampleBEMFileOption);
    parser.process(app);

    //########################################################################################
    // Read the BEM
    QFile t_fileBem(parser.value(sampleBEMFileOption));
    MNEBem t_Bem(t_fileBem);

    if( t_Bem.size() > 0 )
    {
        qDebug() << "t_Bem[0].tri_nn:" << t_Bem[0].tri_nn(0,0) << t_Bem[0].tri_nn(0,1) << t_Bem[0].tri_nn(0,2);
        qDebug() << "t_Bem[0].tri_nn:" << t_Bem[0].tri_nn(2,0) << t_Bem[0].tri_nn(2,1) << t_Bem[0].tri_nn(2,2);
        qDebug() << "t_Bem[0].rr:" << t_Bem[0].rr(2,0) << t_Bem[0].rr(2,1) << t_Bem[0].rr(2,2);
    }

    //Read and write Iso2Mesh Bem
    QString folder = "./MNE-sample-data/warping/AVG4-0Years_segmented_BEM3/bem/";

    MatrixXd help;

    MNEBem t_BemIso2Mesh;
    MNEBemSurface  p_Brain;
    p_Brain.id = FIFFV_BEM_SURF_ID_BRAIN;
    QString path=folder;
    IOUtils::read_eigen_matrix(help, path.append("inner_skull_vert.txt"));
    p_Brain.rr= help.cast<float>();
    path=folder;
    IOUtils::read_eigen_matrix(help, path.append("inner_skull_tri.txt"));
    p_Brain.tris = help.cast<int>();
    p_Brain.np = p_Brain.rr.rows();
    p_Brain.ntri = p_Brain.tris.rows();
    p_Brain.addTriangleData();
    p_Brain.addVertexNormals();
    t_BemIso2Mesh<<p_Brain;

    MNEBemSurface  p_Skull;
    p_Skull.id = FIFFV_BEM_SURF_ID_SKULL;
    path=folder;
    IOUtils::read_eigen_matrix(help,path.append("outer_skull_vert.txt"));
    p_Skull.rr =  help.cast<float>();
    path=folder;
    IOUtils::read_eigen_matrix(help,path.append("outer_skull_tri.txt"));
    p_Skull.tris =  help.cast<int>();
    p_Skull.np = p_Skull.rr.rows();
    p_Skull.ntri = p_Skull.tris.rows();
    p_Skull.addTriangleData();
    p_Skull.addVertexNormals();
    t_BemIso2Mesh<<p_Skull;

    MNEBemSurface  p_Head;
    p_Head.id = FIFFV_BEM_SURF_ID_HEAD;
    path=folder;
    IOUtils::read_eigen_matrix(help,path.append("skin_vert.txt"));
    p_Head.rr =  help.cast<float>();
    path=folder;
    IOUtils::read_eigen_matrix(help,path.append("skin_tri.txt"));
    p_Head.tris =  help.cast<int>();
    p_Head.np = p_Head.rr.rows();
    p_Head.ntri = p_Head.tris.rows();
    p_Head.addTriangleData();
    p_Head.addVertexNormals();
    t_BemIso2Mesh<<p_Head;

    QFile t_fileIso2MeshBem("./Iso2MeshBem/AVG4-0Years_segmented_BEM3.fiff");
    t_BemIso2Mesh.write(t_fileIso2MeshBem);
    t_fileIso2MeshBem.close();

    // Write the BEM
    QFile t_fileBemTest("./MNE-sample-data/subjects/sample/bem/sample-head-test.fif");
    t_Bem.write(t_fileBemTest);
    t_fileBemTest.close();

    MNEBem t_BemTest (t_fileBemTest) ;

    if( t_BemTest.size() > 0 )
    {
        qDebug() << "t_BemTest[0].tri_nn:" << t_BemTest[0].tri_nn(0,0) << t_BemTest[0].tri_nn(0,1) << t_BemTest[0].tri_nn(0,2);
        qDebug() << "t_BemTest[0].tri_nn:" << t_BemTest[0].tri_nn(2,0) << t_BemTest[0].tri_nn(2,1) << t_BemTest[0].tri_nn(2,2);
    }

    qDebug() << "Put your stuff your interest in here";

    return app.exec();
}
