
//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================


#include "sourcelab.h"

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


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNELIB;
using namespace FSLIB;


SourceLab::SourceLab(QObject *parent)
: QObject(parent)
{
    MNEForwardSolution* t_pFwd = NULL;

    QString t_sFileName = "./MNE-sample-data/MEG/sample/sample_audvis-meg-eeg-oct-6-fwd.fif";

    QFile* t_pFile = new QFile(t_sFileName);

    if(!MNEForwardSolution::read_forward_solution(t_pFile, t_pFwd))
    {
        delete t_pFile;
        if(t_pFwd)
            delete t_pFwd;
//        return -1;
        return;
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
}
