//=============================================================================================================
/**
* @file     bcifeaturewindow.cpp
* @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     March, 2014
*
* @section  LICENSE
*
* Copyright (C) 2013, Lorenz Esch, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Contains the implementation of the BCIAboutWidget class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "bcifeaturewindow.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace BCIPlugin;

BCIFeatureWindow::BCIFeatureWindow(BCI* pBCI, QWidget *parent)
: QWidget(parent)
,  m_pBCI(pBCI)
{
    ui.setupUi(this);

    qRegisterMetaType<MyQList>("MyQList");

    connect(m_pBCI, &BCI::paintFeatures,
            this, &BCIFeatureWindow::paintFeaturesToScene);
}


//*************************************************************************************************************

BCIFeatureWindow::~BCIFeatureWindow()
{
}


//*************************************************************************************************************

void BCIFeatureWindow::initGui()
{
    QSize size = ui.m_graphicsView_featureVisualization->size();
    m_scene.addLine(0,0,500,500);

    ui.m_graphicsView_featureVisualization->setScene(&m_scene);
    ui.m_graphicsView_featureVisualization->setSceneRect(QRectF(0,0,500,500));
    ui.m_graphicsView_featureVisualization->setInteractive(true);
}


//*************************************************************************************************************

void BCIFeatureWindow::paintFeaturesToScene(MyQList features)
{
    std::cout<<"features.size()"<<features.size()<<endl;
    for(int i = 0; i<features.size()-1; i=i+2)
        for(int t = 0; t<features.at(i).second.size(); t++)
        {
            QRectF rect(features.at(i).second.at(t)/500, features.at(i+1).second.at(t)/500, 2, 2);
            m_scene.addEllipse(rect);
        }
}

