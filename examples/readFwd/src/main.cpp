//=============================================================================================================
/**
* @file		main.cpp
* @author	Christoph Dinh <christoph.dinh@live.de>;
* @version	1.0
* @date		October, 2010
*
* @section	LICENSE
*
* Copyright (C) 2010 Christoph Dinh. All rights reserved.
*
* No part of this program may be photocopied, reproduced,
* or translated to another program language without the
* prior written consent of the author.
*
*
* @brief	Implements the main() application function.
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <iostream>
#include <vector>

#include "../../../MNE/mne/mne.h"

//#include "../interfaces/IPlugin.h"
//#include "../interfaces/IMNE.h"
//#include "../interfaces/IFiff.h"


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
//using namespace FIFF;


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


//    std::vector<IPlugin*> s_vecPlugins;
//    IMNE* t_pMNE;
//    IFiff* t_pFiff;


//    QDir pluginsDir(qApp->applicationDirPath());

//    pluginsDir.cd("plugins");
//    foreach (QString fileName, pluginsDir.entryList(QDir::Files)) {
//        QPluginLoader pluginLoader(pluginsDir.absoluteFilePath(fileName));
//        QObject *pPlugin = pluginLoader.instance();
//        if (pPlugin) {
//            s_vecPlugins.push_back(dynamic_cast<IPlugin*>(pPlugin));

//            qDebug() << "try loading plugin " << s_vecPlugins.at(s_vecPlugins.size()-1)->getName();

//            if(dynamic_cast<IMNE*>(pPlugin))
//            {
//                t_pMNE = qobject_cast<IMNE*>(pPlugin);

//                qDebug() << t_pMNE->getName() << " plugin loaded";

//                t_pMNE->read_source_spaces(QString("../../MNE-sample-data/MEG/sample/sample_audvis-eeg-oct-6-fwd.fif"));
//            }

//            if(dynamic_cast<IFiff*>(pPlugin))
//            {
//                t_pFiff = qobject_cast<IFiff*>(pPlugin);

//                qDebug() << t_pFiff->getName() << " plugin loaded";
//            }
//        }
//    }


    MNEForwardSolution* t_ForwardSolution = NULL;

//    MNE::read_forward_solution(QString("C:/Users/Christoph/Documents/SourceLab/sl_MATLAB/Data/MEG/ernie/sef-oct-6p-src-fwd.fif"));
//    MNE::read_forward_solution(QString("../../MNE-sample-data/MEG/sample/sample_audvis-eeg-oct-6-fwd.fif"));
    QString t_sFile = "../../MNE-sample-data/MEG/sample/sample_audvis-meg-eeg-oct-6-fwd.fif";
    MNE::read_forward_solution(t_sFile, t_ForwardSolution);

    std::cout << std::endl << t_ForwardSolution->sol->data.block(0,0,10,10) << std::endl;
    std::cout << std::endl << t_ForwardSolution->source_rr.block(0,0,10,3) << std::endl ;





    
    return a.exec();
}

//*************************************************************************************************************
//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================


