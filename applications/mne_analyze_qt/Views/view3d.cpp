//=============================================================================================================
/**
* @file     view3d.cpp
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
*       view3d.h
*/

#include <QWindow>
#include <QQuickView>
#include <QPushButton>
#include <disp3D/brainview.h>

#include "view3d.h"

using namespace FSLIB;
using namespace DISP3DLIB;

View3D::View3D()
{
    this->resize(128,128);
    m_view3d_gridlayout = new QGridLayout(this);
    m_view3d_hboxlayout = new QHBoxLayout(this);

    //FreeSurfer example
    //
    // pial
    //
    BrainView t_pialBrainView("sample", 2, "pial", "./MNE-sample-data/subjects");

    if (t_pialBrainView.stereoType() != QGLView::RedCyanAnaglyph)
        t_pialBrainView.camera()->setEyeSeparation(0.3f);

    //t_pialBrainView.setTitle(QString("Pial surface"));
    //t_pialBrainView.show();

    m_view3d_container = QWidget::createWindowContainer(&t_pialBrainView);
    m_view3d_container->setMinimumSize(256,256);
    m_view3d_container->setMaximumSize(256,256);
    m_view3d_container->setFocusPolicy(Qt::TabFocus);
    //m_view3d_container->show();

    m_view3d_hboxlayout->addWidget(m_view3d_container);
    m_view3d_gridlayout->addLayout(m_view3d_hboxlayout,0,0,1,1);

    this->setLayout(m_view3d_gridlayout);
}

View3D::~View3D()
{

}

