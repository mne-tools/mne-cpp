//=============================================================================================================
/**
* @file     deepviewerwidget.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     January, 2017
*
* @section  LICENSE
*
* Copyright (C) 2017, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    DeepViewerWidget class implementation.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "arthurwidgets.h"

#include "deepviewerwidget.h"
#include "deepviewerrenderer.h"
#include "deepviewercontrol.h"


#include <stdio.h>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

DeepViewerWidget::DeepViewerWidget()
{
    setWindowTitle(tr("Path Stroking"));

    // Widget construction and property setting
    m_renderer = new DeepViewerRenderer(this);

    m_controls = new DeepModelViewerControls(0, m_renderer);

    // Layouting
    QHBoxLayout *viewLayout = new QHBoxLayout(this);
    viewLayout->addWidget(m_renderer);

    viewLayout->addWidget(m_controls);

    m_renderer->loadDescription(":res/deepmodelviewer/deepmodelviewer.html");

    connect(m_renderer, SIGNAL(clicked()), this, SLOT(showControls()));
    connect(m_controls, SIGNAL(okPressed()), this, SLOT(hideControls()));
    connect(m_controls, SIGNAL(quitPressed()), QApplication::instance(), SLOT(quit()));
}


//*************************************************************************************************************

void DeepViewerWidget::setModel(CNTK::FunctionPtr &model)
{
    m_pModel = model;
}


//*************************************************************************************************************

void DeepViewerWidget::showControls()
{
    m_controls->showFullScreen();
}


//*************************************************************************************************************

void DeepViewerWidget::hideControls()
{
    m_controls->hide();
}
