//=============================================================================================================
/**
 * @file     bcifeaturewindow.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Gabriel B Motta <gabrielbenmotta@gmail.com>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  1.0
 * @date     March, 2014
 *
 * @section  LICENSE
 *
 * Copyright (C) 2014, Christoph Dinh, Gabriel B Motta, Lorenz Esch. All rights reserved.
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
#include "../bci.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace BCIPLUGIN;
using namespace Eigen;

//*************************************************************************************************************

BCIFeatureWindow::BCIFeatureWindow(BCI* pBCI, QWidget *parent)
: QWidget(parent)
,  m_pBCI(pBCI)
,  m_dFeatureMax(15)
,  m_iScale(600)
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

    m_dFeatureMax =  m_pBCI->m_dDisplayRangeBoundary;//15;//1e-08;
    m_dFeatureMax = 15;
    m_iScale = 600;

    addBoundaryLineToScene();
}


//*************************************************************************************************************
void BCIFeatureWindow::addBoundaryLineToScene()
{
    m_scene.addLine(0*(m_iScale/m_dFeatureMax), boundaryValue(0)*(m_iScale/m_dFeatureMax), 15*(m_iScale/m_dFeatureMax), boundaryValue(15)*(m_iScale/m_dFeatureMax));

    // Add items
//        QGraphicsTextItem* leftElectrode = m_scene.addText(m_pBCI->m_slChosenFeatureSensor.at(0));
//        leftElectrode->setPos(QPointF(m_iScale/2, 10));
//        QGraphicsTextItem* rightElectrode = m_scene.addText(m_pBCI->m_slChosenFeatureSensor.at(1));
//        rightElectrode->setPos(QPointF(10,m_iScale/2));

//        QGraphicsTextItem* leftUnit = m_scene.addText("1e-08");
//        leftUnit->setPos(QPointF(m_iScale-100, 10));
//        QGraphicsTextItem* rightUnit = m_scene.addText("1e-08");
//        rightUnit->setPos(QPointF(10, m_iScale-100));
}


//*************************************************************************************************************

double BCIFeatureWindow::boundaryValue(double x)
{
    QVector<VectorXd> boundary = m_pBCI->m_vLoadedSensorBoundary;

    double K = boundary.at(0)(0);
    double L1 = boundary.at(1)(0);
    double L2 = boundary.at(1)(1);

    double y;

    y = (K+L1*x)/L2;

    return (-1*y);
}

//*************************************************************************************************************

void BCIFeatureWindow::paintFeaturesToScene(MyQList features, bool bTriggerActivated)
{
//    std::cout<<"features.size()"<<features.size()<<endl;
    if(features.first().size() == 2) // Only plot when 2D case with two electrodes
    {
        if(m_scene.items().size() > m_pBCI->m_iNumberFeaturesToDisplay)
        {
            m_scene.clear();
            addBoundaryLineToScene();
        }

//        // If trigger was activated during feature calculation -> change scenes brush color
//        if(bTriggerActivated)
//            m_scene.setBackgroundBrush(Qt::blue);
//        else
//            m_scene.setBackgroundBrush(Qt::white);

        for(int i = 0; i<features.size(); i++)
        {
            double featureA = features.at(i).at(0);
            double featureB = features.at(i).at(1);

            if(featureA > m_dFeatureMax)
                m_dFeatureMax = featureA;

            if(featureB > m_dFeatureMax)
                m_dFeatureMax = featureB;

            QRectF rect(featureA*(m_iScale/m_dFeatureMax), featureB*(m_iScale/m_dFeatureMax), 5, 5);

            std::cout<<"Scaled: "<< featureA*(m_iScale/m_dFeatureMax) <<" "<< featureB*(m_iScale/m_dFeatureMax) << endl;
            std::cout<<"Unscaled: "<< featureA <<" "<< featureB << endl;
            //std::cout<<"m_dFeatureMax: "<< m_dFeatureMax <<endl;

            // Add ellipse to scene
            if(bTriggerActivated)
                m_scene.addEllipse(rect, QPen(Qt::red));
            else
                m_scene.addEllipse(rect, QPen(Qt::black));
        }

        //ui.m_graphicsView_featureVisualization->fitInView(m_scene.sceneRect());
    }
}

