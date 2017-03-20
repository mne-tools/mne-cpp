//=============================================================================================================
/**
* @file     viewerwidget.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     January, 2017
*
* @section  LICENSE
*
* Copyright (C) 2017 Christoph Dinh, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
*/
//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mdiview.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEANALYZE;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MdiView::MdiView(QWidget *parent)
: QWidget(parent)
{
    //QGridLayout is used so the viewer and MdiArea can fit always the size of MainWindow
    m_gridLayout = new QGridLayout(this);
    //Multiple Display Area, created inside ViewerWidget
    m_MdiArea = new QMdiArea(this);
    m_gridLayout->addWidget(m_MdiArea);

    //=============================================================================================================
    //
    //Pial surface
    //
    //A new View3D object is created, in charge of the subwindow and the displaying of the 3D surface
    m_view3d_pial = new View3DAnalyze(1);
    //A new subwindow is created
    m_MdiArea->addSubWindow(m_view3d_pial);
    m_view3d_pial->setWindowTitle("Pial surface");

    //Cascade subwindows
    this->m_MdiArea->cascadeSubWindows();
}


//*************************************************************************************************************

void MdiView::CascadeSubWindows()
{
    //Arrange subwindows in a Tile mode    //Arrange subwindows in a Tile mode
    this->m_MdiArea->cascadeSubWindows();
}


//*************************************************************************************************************

void MdiView::TileSubWindows()
{
    //Arrange subwindows in a Tile mode
    this->m_MdiArea->tileSubWindows();
}


//*************************************************************************************************************

void MdiView::ReloadSurfaces()
{
//    //Not working at the time
//    m_MdiArea->addSubWindow(m_view3d_pial);
//    m_view3d_pial->setWindowTitle("Pial surface");
}


//*************************************************************************************************************

MdiView::~MdiView()
{
}
