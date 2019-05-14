//=============================================================================================================
/**
* @file     music.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     February, 2017
*
* @section  LICENSE
*
* Copyright (C) 2017 Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Definition of the Music class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "music.h"
#include "FormFiles/musiccontrol.h"


#include <fs/label.h>
#include <fs/surface.h>
#include <fs/surfaceset.h>
#include <fs/annotationset.h>

#include <fiff/fiff_evoked.h>
#include <fiff/fiff.h>

#include <mne/mne.h>
#include <mne/mne_sourceestimate.h>

#include <inverse/rapMusic/rapmusic.h>

#include <disp3D/engine/view/view3D.h>
#include <disp3D/engine/model/data3Dtreemodel.h>
#include <disp3D/engine/model/items/sourcedata/mnedatatreeitem.h>

#include <disp/viewers/control3dview.h>

#include <utils/mnemath.h>

#include <iostream>


#include <QApplication>
#include <QCommandLineParser>



//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MUSICEXTENSION;
using namespace ANSHAREDLIB;
using namespace MNELIB;
using namespace FSLIB;
using namespace FIFFLIB;
using namespace INVERSELIB;
using namespace UTILSLIB;
using namespace DISP3DLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Music::Music()
: m_pControl(Q_NULLPTR)
, m_pMusicControl(Q_NULLPTR)
{

}


//*************************************************************************************************************

Music::~Music()
{

}


//*************************************************************************************************************

QSharedPointer<IExtension> Music::clone() const
{
    QSharedPointer<Music> pMusicClone(new Music);
    return pMusicClone;
}


//*************************************************************************************************************

void Music::init()
{
    m_pMusicControl = new MusicControl;
}


//*************************************************************************************************************

void Music::unload()
{

}


//*************************************************************************************************************

QString Music::getName() const
{
    return "RAP-MUSIC";
}


//*************************************************************************************************************

QMenu *Music::getMenu()
{
    return Q_NULLPTR;
}


//*************************************************************************************************************

QDockWidget *Music::getControl()
{
    if(!m_pControl) {
        m_pControl = new QDockWidget(tr("MUSIC"));
        m_pControl->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
        m_pControl->setMinimumWidth(180);
        m_pControl->setWidget(m_pMusicControl);
    }

    return m_pControl;
}


//*************************************************************************************************************

QWidget *Music::getView()
{
    return Q_NULLPTR;
}


//*************************************************************************************************************

void Music::calculate()
{
    QString fwdFileOption(QCoreApplication::applicationDirPath() + "/MNE-sample-data/MEG/sample/sample_audvis-meg-eeg-oct-6-fwd.fif");
    QString evokedFileOption(QCoreApplication::applicationDirPath() + "/MNE-sample-data/MEG/sample/sample_audvis-ave.fif");
    QString subjectOption("sample");
    QString subjectDirectoryOption(QCoreApplication::applicationDirPath() + "/MNE-sample-data/subjects");

    QString t_sFileNameStc("");//"RapMusic.stc");
    QString annotOption("aparc.a2009s");

    QString surfOption("orig");

    qint32 numDipolePairs = 1;

    bool doMovie = false;

    //Load data
    QFile t_fileFwd(fwdFileOption);
    QFile t_fileEvoked(evokedFileOption);
    QString subject(subjectOption);
    QString subjectDir(subjectDirectoryOption);

    AnnotationSet t_annotationSet(subject, 2, annotOption, subjectDir);
    SurfaceSet t_surfSet(subject, 2, surfOption, subjectDir);


    // Load data
    fiff_int_t setno = 1;
    QPair<QVariant, QVariant> baseline(QVariant(), 0);
    FiffEvoked evoked(t_fileEvoked, setno, baseline);
    if(evoked.isEmpty())
        return;

    std::cout << "evoked first " << evoked.first << "; last " << evoked.last << std::endl;

    MNEForwardSolution t_Fwd(t_fileFwd);
    if(t_Fwd.isEmpty())
        return;

    QStringList ch_sel_names = t_Fwd.info.ch_names;
    FiffEvoked pickedEvoked = evoked.pick_channels(ch_sel_names);

    //
    // Cluster forward solution;
    //
    MNEForwardSolution t_clusteredFwd = t_Fwd.cluster_forward_solution(t_annotationSet, 20);//40);

//    std::cout << "Size " << t_clusteredFwd.sol->data.rows() << " x " << t_clusteredFwd.sol->data.cols() << std::endl;
//    std::cout << "Clustered Fwd:\n" << t_clusteredFwd.sol->data.row(0) << std::endl;

    RapMusic t_rapMusic(t_clusteredFwd, false, numDipolePairs);

    int iWinSize = 200;
    if(doMovie) {
        t_rapMusic.setStcAttr(iWinSize, 0.6f);
    }

    MNESourceEstimate sourceEstimate = t_rapMusic.calculateInverse(pickedEvoked);

    if(doMovie) {
        //Select only the activations once
        MatrixXd dataPicked(sourceEstimate.data.rows(), int(std::floor(sourceEstimate.data.cols()/iWinSize)));

        for(int i = 0; i < dataPicked.cols(); ++i) {
            dataPicked.col(i) = sourceEstimate.data.col(i*iWinSize);
        }

        sourceEstimate.data = dataPicked;
    }

    //Select only the activations once
    MatrixXd dataPicked(sourceEstimate.data.rows(), int(std::floor(sourceEstimate.data.cols()/iWinSize)));

    for(int i = 0; i < dataPicked.cols(); ++i) {
        dataPicked.col(i) = sourceEstimate.data.col(i*iWinSize);
    }

    sourceEstimate.data = dataPicked;

    if(sourceEstimate.isEmpty())
        return;

//    //Visualize the results
//    View3D::SPtr testWindow = View3D::SPtr(new View3D());
//    Data3DTreeModel::SPtr p3DDataModel = Data3DTreeModel::SPtr(new Data3DTreeModel());

//    testWindow->setModel(p3DDataModel);

//    p3DDataModel->addSurfaceSet(parser.value(subjectOption), evoked.comment, t_surfSet, t_annotationSet);

//    //Add rt source loc data and init some visualization values
//    if(mnedatatreeitem* pRTDataItem = p3DDataModel->addSourceData(parser.value(subjectOption), evoked.comment, sourceEstimate, t_clusteredFwd)) {
//        pRTDataItem->setLoopState(true);
//        pRTDataItem->setTimeInterval(17);
//        pRTDataItem->setNumberAverages(1);
//        pRTDataItem->setStreamingState(true);
//        pRTDataItem->setNormalization(QVector3D(0.01,0.5,1.0));
//        pRTDataItem->setVisualizationType("Annotation based");
//        pRTDataItem->setColortable("Hot");
//    }
//    testWindow->show();

//    Control3DView::SPtr control3DWidget = Control3DView::SPtr(new Control3DView());
//    control3DWidget->init(p3DDataModel, testWindow);
//    control3DWidget->show();

    if(!t_sFileNameStc.isEmpty())
    {
        QFile t_fileClusteredStc(t_sFileNameStc);
        sourceEstimate.write(t_fileClusteredStc);
    }

//    return a.exec();
}
