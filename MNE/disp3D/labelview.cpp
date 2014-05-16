//=============================================================================================================
/**
* @file     labelview.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
* @version  1.0
* @date     March, 2013
*
* @section  LICENSE
*
* Copyright (C) 2013, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Implementation of the LabelView class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "labelview.h"

#include <fs/label.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include "qglbuilder.h"
#include "qglcube.h"

#include <QtCore/qurl.h>
#include <QArray>
#include <QTimer>


//*************************************************************************************************************
//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <iostream>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISP3DLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

LabelView::LabelView(SurfaceSet &p_surfSet, QList<Label> &p_qListLabels, QList<RowVector4i> &p_qListRGBAs, QWindow *parent)
: QGLView(parent)
, m_surfSet(p_surfSet)
, m_qListLabels(p_qListLabels)
, m_qListRGBAs(p_qListRGBAs)
, m_bStereo(true)
, m_pSceneNodeBrain(0)
, m_pSceneNode(0)
, m_nTSteps(0)
{
    m_pCameraFrontal = new QGLCamera(this);
    m_pCameraFrontal->setAdjustForAspectRatio(false);


    m_timer = new QTimer(this);
    QObject::connect(m_timer, &QTimer::timeout, this, &LabelView::updateData);
}


//*************************************************************************************************************

LabelView::~LabelView()
{
    delete m_pSceneNode;
}


//*************************************************************************************************************

void LabelView::pushSourceEstimate(MNESourceEstimate &p_sourceEstimate)
{
    m_timer->stop();
    m_curSourceEstimate = p_sourceEstimate;

    m_nTSteps = m_curSourceEstimate.times.size();

    qDebug() << "#########" << m_curSourceEstimate.data.rows() << m_curSourceEstimate.data.cols() << m_nTSteps;

    m_vecFirstLabelSourceEstimate = m_curSourceEstimate.data.block(0,0,3,m_curSourceEstimate.data.cols()).colwise().sum();

    m_dMaxSourceEstimate = m_vecFirstLabelSourceEstimate.maxCoeff();


    m_timer->start(m_curSourceEstimate.tstep*1000);
}


//*************************************************************************************************************

void LabelView::initializeGL(QGLPainter *painter)
{
    // in the constructor construct a builder on the stack
    QGLBuilder builder;

    float fac = 10.0f;

    builder << QGL::Faceted;
    m_pSceneNodeBrain = builder.currentNode();

    builder.pushNode();

    // Collor palette
    qint32 index;
    QSharedPointer<QGLMaterialCollection> palette = builder.sceneNode()->palette(); // register color palette within the root node

    //
    // Build each hemisphere in its separate node
    //
    for(qint32 h = 0; h < 2; ++h)
    {
        builder.newNode();//create new hemisphere node
        {
            MatrixX3i tris;
            MatrixX3f rr = m_surfSet[h].rr();

            builder.pushNode();
            //
            // Create each ROI in its own node
            //
            for(qint32 k = 0; k < m_qListLabels.size(); ++k)
            {
                //check if label hemi fits current hemi
                if(m_qListLabels[k].hemi != h)
                    continue;

                //Ggenerate label tri information
                tris = m_qListLabels[k].selectTris(m_surfSet[h]);

                // add new ROI node when current ROI node is not empty
                if(builder.currentNode()->count() > 0)
                    builder.newNode();


                QGeometryData t_GeometryDataTri;


                MatrixXf t_TriCoords(3,3*tris.rows());

                for(qint32 i = 0; i < tris.rows(); ++i)
                {
                    t_TriCoords.col(i*3) = rr.row( tris(i,0) ).transpose();
                    t_TriCoords.col(i*3+1) = rr.row( tris(i,1) ).transpose();
                    t_TriCoords.col(i*3+2) = rr.row( tris(i,2) ).transpose();
                }

                t_TriCoords *= fac;
                t_GeometryDataTri.appendVertexArray(QArray<QVector3D>::fromRawData( reinterpret_cast<const QVector3D*>(t_TriCoords.data()), t_TriCoords.cols() ));

                //
                // If triangles are available.
                //
                if (t_GeometryDataTri.count() > 0)
                {

                    //
                    //  Add triangles to current node
                    //
                    builder.addTriangles(t_GeometryDataTri);

                    //
                    // Colorize ROI
                    //
                    QGLMaterial *t_pMaterialROI = new QGLMaterial();
                    int r, g, b;
                    r = m_qListRGBAs[k][0];
                    g = m_qListRGBAs[k][1];
                    b = m_qListRGBAs[k][2];

                    t_pMaterialROI->setColor(QColor(r,g,b,200));
//                        t_pMaterialROI->setEmittedLight(QColor(100,100,100,255));
//                        t_pMaterialROI->setSpecularColor(QColor(10,10,10,20));


                    index = palette->addMaterial(t_pMaterialROI);
                    builder.currentNode()->setMaterialIndex(index);
                }
            }
        }
        // Go one level up
        builder.popNode();
    }
    // Go one level up
    builder.popNode();

    // Optimze current scene for display and calculate lightning normals
    m_pSceneNode = builder.finalizedSceneNode();
    m_pSceneNode->setParent(this);

    //
    // Create light models
    //
    m_pLightModel = new QGLLightModel(this);
    m_pLightModel->setAmbientSceneColor(Qt::white);
    m_pLightModel->setViewerPosition(QGLLightModel::LocalViewer);

    m_pLightModel = new QGLLightModel(this);

    m_pLightParametersScene = new QGLLightParameters(this);
    m_pLightParametersScene->setPosition(QVector3D(0.0f, 0.0f, 3.0f));
    painter->setMainLight(m_pLightParametersScene);



    simCount = 0;

    //
    // Set stereo type
    //
    if (m_bStereo) {
        this->setStereoType(QGLView::RedCyanAnaglyph);
        camera()->setEyeSeparation(0.4f);
        m_pCameraFrontal->setEyeSeparation(0.1f);
    }

}


//*************************************************************************************************************

void LabelView::paintGL(QGLPainter *painter)
{
    glEnable(GL_BLEND); // enable transparency

    //    painter->modelViewMatrix().rotate(45.0f, 1.0f, 1.0f, 1.0f);



    painter->modelViewMatrix().push();
    painter->projectionMatrix().push();

    painter->setStandardEffect(QGL::LitMaterial);
//        painter->setCamera(m_pCameraFrontal);
    painter->setLightModel(m_pLightModel);

//        material.bind(painter);
//        material.prepareToDraw(painter, painter->attributes());


//    qint32 iVal = (simCount%m_nTSteps)*(255.0/m_nTSteps);

    if(m_nTSteps > 0)
    {
        qint32 iVal = (m_vecFirstLabelSourceEstimate[simCount%m_nTSteps]/m_dMaxSourceEstimate) * 255;
        m_pSceneNode->palette()->material(15)->setSpecularColor(QColor(iVal,iVal,iVal,1));
    }

    m_pSceneNode->draw(painter);


    painter->modelViewMatrix().pop();
    painter->projectionMatrix().pop();
}


//*************************************************************************************************************

//QGLSceneNode *LabelView::createScene()
//{
//    QGLBuilder builder;
//    QGLSceneNode *root = builder.sceneNode();

//    //completed building, so finalise
//    return builder.finalizedSceneNode();
//}


//*************************************************************************************************************

void LabelView::updateData()
{
//    qDebug() << simCount%m_nTSteps;

    ++simCount;

    this->update();
}
