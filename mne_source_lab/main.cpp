//=============================================================================================================
/**
* @file     main.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     July, 2012
*
* @section  LICENSE
*
* Copyright (C) 2012, Christoph Dinh and Matti Hamalainen. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that
* the following conditions are met:
*     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
*       following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
*       the following disclaimer in the documentation and/or other materials provided with the distribution.
*     * Neither the name of the Massachusetts General Hospital nor the names of its contributors may be used
*       to endorse or promote products derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MASSACHUSETTS GENERAL HOSPITAL BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*
* @brief    Implements the main() application function.
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <iostream>
#include <vector>

#include "../MNE/mne/mne.h"
#include "../MNE/fs/annotation.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================
//#include <QDebug>
//#include <QDir>
//#include <QPluginLoader>

#include <QtCore/QCoreApplication>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNELIB;
using namespace FSLIB;


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

    MNEForwardSolution* t_pFwd = NULL;

    QString t_sFileName = "./MNE-sample-data/MEG/sample/sample_audvis-meg-eeg-oct-6-fwd.fif";

    QFile* t_pFile = new QFile(t_sFileName);

    if(!MNEForwardSolution::read_forward_solution(t_pFile, t_pFwd))
    {
        delete t_pFile;
        if(t_pFwd)
            delete t_pFwd;
        return -1;
    }

    //Cluster the forward solution

    MNEForwardSolution* t_pFwdClustered = NULL;

    QString t_sLHAnnotFileName = "./MNE-sample-data/subjects/sample/label/lh.aparc.a2009s.annot";
    Annotation* t_pLHAnnotation= new Annotation(t_sLHAnnotFileName);


    QString t_sRHAnnotFileName = "./MNE-sample-data/subjects/sample/label/rh.aparc.a2009s.annot";
    Annotation* t_pRHAnnotation= new Annotation(t_sRHAnnotFileName);

    t_pFwd->cluster_forward_solution(t_pFwdClustered, t_pLHAnnotation, t_pRHAnnotation, 40);

    delete t_pFile;
    delete t_pFwd;

    return a.exec();
}

//*************************************************************************************************************
//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

