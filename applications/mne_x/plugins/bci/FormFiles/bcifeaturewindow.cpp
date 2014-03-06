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

    ui.m_graphicsView_featureVisualization->setScene(&m_scene);

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
    m_scene.clear();

    m_dFeatureMax = 0;
}


//*************************************************************************************************************

void BCIFeatureWindow::paintFeaturesToScene(MyQList features, bool bTriggerActivated)
{
    //std::cout<<"features.size()"<<features.size()<<endl;
    if(features.first().size() == 2) // Only plot when 2D case with two electrodes
    {
        m_scene.clear();

        // If trigger was activated during feature calculation -> change scenes brush color
        if(bTriggerActivated)
            m_scene.setBackgroundBrush(Qt::blue);
        else
            m_scene.setBackgroundBrush(Qt::white);

        // Add items
        int lineSize = 500;
        m_scene.addLine(0,0,lineSize,lineSize);
        QGraphicsTextItem* leftElectrode = m_scene.addText(m_pBCI->m_slChosenFeatureSensor.at(0));
        leftElectrode->setPos(QPointF(lineSize/2, 10));
        QGraphicsTextItem* rightElectrode = m_scene.addText(m_pBCI->m_slChosenFeatureSensor.at(1));
        rightElectrode->setPos(QPointF(10,lineSize/2));

        for(int i = 0; i<features.size(); i++)
        {
            double featureA = features.at(i).at(0);
            double featureB = features.at(i).at(1);

            if(featureA > m_dFeatureMax)
                m_dFeatureMax = featureA;

            if(featureB > m_dFeatureMax)
                m_dFeatureMax = featureB;

            QRectF rect(featureA*(lineSize/m_dFeatureMax), featureB*(lineSize/m_dFeatureMax), 2, 2);
//            std::cout<<"Scaled: "<< featureA*(lineSize/m_dFeatureMax) <<" "<< featureB*(lineSize/m_dFeatureMax) << endl;
//            std::cout<<"Unscaled: "<< featureA <<" "<< featureB << endl;
            // If NAN -> don't plot features -> case when working with the Refa device without any channels connected
            if(featureA == featureA && featureB == featureB)
                m_scene.addEllipse(rect);
        }

        ui.m_graphicsView_featureVisualization->fitInView(m_scene.sceneRect());
    }
}

