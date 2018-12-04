//=============================================================================================================
/**
* @file     view3danalyze.cpp
* @author   Franco Polo <Franco-Joel.Polo@tu-ilmenau.de>;
*			Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
*           Jens Haueisen <jens.haueisen@tu-ilmenau.de>
* @version  1.0
* @date     January, 2015
*
* @section  LICENSE
*
* Copyright (C) 2015, Franco Polo, Lorenz Esch, Christoph Dinh, Matti Hamalainen and Jens Haueisen. All rights reserved.
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
* @brief
*
*@file
*       view3danalyze.h
*/
//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "view3danalyze.h"

#include <QCoreApplication>


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
// View3D constructor
//=============================================================================================================

View3DAnalyze::View3DAnalyze(int surface_type)
{
    //Resizing the created QMdiSubwindow to fit the minimun size of the container used later
    this->resize(256,256);
    //QGridLayout is used so the container constantly resizes to the size of the QMdiSubwindow
    m_view3d_gridlayout = new QGridLayout(this);

    //*************************************************************************************************************
    //=============================================================================================================
    // Loading a FreeSurfer example from BrainView class
    //=============================================================================================================

    switch(surface_type){
    case 1:
    //
    // pial
    //
        m_BrainView = new View3D();
        m_p3DDataModel = Data3DTreeModel::SPtr(new Data3DTreeModel());

        m_BrainView->setModel(m_p3DDataModel);
        m_p3DDataModel->addSurfaceSet("Subject01", "Set", SurfaceSet("sample", 2, "orig", QCoreApplication::applicationDirPath() + "/MNE-sample-data/subjects"));
        break;
    case 2:
    //
    // inflated
    //
        m_BrainView = new View3D();
        m_p3DDataModel = Data3DTreeModel::SPtr(new Data3DTreeModel());
        m_BrainView->setModel(m_p3DDataModel);
        m_p3DDataModel->addSurfaceSet("Subject01", "Set", SurfaceSet("sample", 2, "inflated", QCoreApplication::applicationDirPath() + "/MNE-sample-data/subjects"));
        break;
    case 3:
    //
    // orig
    //
        m_BrainView = new View3D();
        m_p3DDataModel = Data3DTreeModel::SPtr(new Data3DTreeModel());
        m_BrainView->setModel(m_p3DDataModel);
        m_p3DDataModel->addSurfaceSet("Subject01", "Set", SurfaceSet("sample", 2, "orig", QCoreApplication::applicationDirPath() + "/MNE-sample-data/subjects"));
        break;
    case 4:
    //
    // white
    //
        m_BrainView = new View3D();
        m_p3DDataModel = Data3DTreeModel::SPtr(new Data3DTreeModel());
        m_BrainView->setModel(m_p3DDataModel);
        m_p3DDataModel->addSurfaceSet("Subject01", "Set", SurfaceSet("sample", 2, "white", QCoreApplication::applicationDirPath() + "/MNE-sample-data/subjects"));
        break;
    }
    //A container is created to contain the QWindow that comes from BrainView, then a minimum size is set
    m_view3d_container = QWidget::createWindowContainer(m_BrainView);
    m_view3d_container->setMinimumSize(256,256);
    //m_view3d_container->setMaximumSize(256,256);
    m_view3d_container->setFocusPolicy(Qt::TabFocus);
    //The loaded surfaces, as a QWindow is added to the created container
    m_view3d_gridlayout->addWidget(m_view3d_container);
}
//*************************************************************************************************************
//=============================================================================================================
// View3DAnalyze destructor
//=============================================================================================================

View3DAnalyze::~View3DAnalyze()
{

}

