//=============================================================================================================
/**
* @file     inverseview.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Daniel Strohmeier <daniel.strohmeier@tu-ilmenau.de>;
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
* @brief    Implementation of the InverseView class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "inverseview.h"
#include "inverseviewproducer.h"

#include <fs/label.h>
#include <disp/colormap.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include "qglbuilder.h"
#include "qglcube.h"

#include <QtCore/qurl.h>
#include <QArray>
#include <QTimer>
#include <QMouseEvent>


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
using namespace DISPLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

InverseView::InverseView(const MNESourceSpace &p_sourceSpace, QList<Label> &p_qListLabels, QList<RowVector4i> &p_qListRGBAs, qint32 p_iFps, bool p_bLoop, bool p_bStereo, QWindow *parent)
: QGLView(parent)
, m_pInverseViewProducer(new InverseViewProducer(p_iFps, p_bLoop))
, m_sourceSpace(p_sourceSpace)
, m_qListLabels(p_qListLabels)
, m_qListRGBAs(p_qListRGBAs)
, m_iColorMode(0)
, m_bStereo(p_bStereo)
, m_fOffsetZ(-100.0f)
, m_fOffsetZEye(60.0f)
, m_pSceneNodeBrain(0)
, m_pSceneNode(0)
{
    qRegisterMetaType<QSharedPointer<Eigen::VectorXd> >("QSharedPointer<Eigen::VectorXd>");


//    m_pCameraFrontal = new QGLCamera(this);
//    m_pCameraFrontal->setAdjustForAspectRatio(false);

//    qDebug() << "p_qListLabels" << p_qListLabels.size();

    QObject::connect(m_pInverseViewProducer.data(), &InverseViewProducer::sourceEstimateSample, this, &InverseView::updateActivation);
}


//*************************************************************************************************************

InverseView::~InverseView()
{
    m_pInverseViewProducer->stop();

    delete m_pSceneNode;
}


//*************************************************************************************************************

void InverseView::pushSourceEstimate(MNESourceEstimate &p_sourceEstimate)
{
    m_pInverseViewProducer->pushSourceEstimate(p_sourceEstimate);
}


//*************************************************************************************************************

void InverseView::initializeGL(QGLPainter *painter)
{
    // in the constructor construct a builder on the stack
    QGLBuilder builder;

    float fac = 100.0f; // too small vertices distances cause clipping errors --> 100 is a good value for freesurfer brain measures

    builder << QGL::Faceted;
    m_pSceneNodeBrain = builder.currentNode();

    builder.pushNode();

    // Collor palette
    qint32 index;
    QSharedPointer<QGLMaterialCollection> palette = builder.sceneNode()->palette(); // register color palette within the root node

    m_qListMapLabelIdIndex << QMap<qint32, qint32>() << QMap<qint32, qint32>();

    //get bounding box
    m_vecBoundingBoxMin.setX(m_sourceSpace[0].rr.col(0).minCoeff()); // X lh min
    m_vecBoundingBoxMin.setY(m_sourceSpace[0].rr.col(1).minCoeff()); // Y lh min
    m_vecBoundingBoxMin.setZ(m_sourceSpace[0].rr.col(2).minCoeff()); // Z lh min
    m_vecBoundingBoxMax.setX(m_sourceSpace[0].rr.col(0).maxCoeff()); // X lh max
    m_vecBoundingBoxMax.setY(m_sourceSpace[0].rr.col(1).maxCoeff()); // Y lh max
    m_vecBoundingBoxMax.setZ(m_sourceSpace[0].rr.col(2).maxCoeff()); // Z lh max

    m_vecBoundingBoxMin.setX(m_vecBoundingBoxMin.x() < m_sourceSpace[1].rr.col(0).minCoeff() ? m_vecBoundingBoxMin.x() : m_sourceSpace[1].rr.col(0).minCoeff()); // X rh min
    m_vecBoundingBoxMin.setY(m_vecBoundingBoxMin.y() < m_sourceSpace[1].rr.col(1).minCoeff() ? m_vecBoundingBoxMin.y() : m_sourceSpace[1].rr.col(1).minCoeff()); // Y rh min
    m_vecBoundingBoxMin.setZ(m_vecBoundingBoxMin.z() < m_sourceSpace[1].rr.col(2).minCoeff() ? m_vecBoundingBoxMin.z() : m_sourceSpace[1].rr.col(2).minCoeff()); // Z rh min
    m_vecBoundingBoxMax.setX(m_vecBoundingBoxMax.x() > m_sourceSpace[1].rr.col(0).maxCoeff() ? m_vecBoundingBoxMax.x() : m_sourceSpace[1].rr.col(0).maxCoeff()); // X rh max
    m_vecBoundingBoxMax.setY(m_vecBoundingBoxMax.y() > m_sourceSpace[1].rr.col(1).maxCoeff() ? m_vecBoundingBoxMax.y() : m_sourceSpace[1].rr.col(1).maxCoeff()); // Y rh max
    m_vecBoundingBoxMax.setZ(m_vecBoundingBoxMax.z() > m_sourceSpace[1].rr.col(2).maxCoeff() ? m_vecBoundingBoxMax.z() : m_sourceSpace[1].rr.col(2).maxCoeff()); // Z rh max

    m_vecBoundingBoxCenter.setX((m_vecBoundingBoxMin.x()+m_vecBoundingBoxMax.x())/2.0f);
    m_vecBoundingBoxCenter.setY((m_vecBoundingBoxMin.y()+m_vecBoundingBoxMax.y())/2.0f);
    m_vecBoundingBoxCenter.setZ((m_vecBoundingBoxMin.z()+m_vecBoundingBoxMax.z())/2.0f);


    //
    // Build each hemisphere in its separate node
    //
    for(qint32 h = 0; h < 2; ++h)
    {
        builder.newNode();//create new hemisphere node
        {
            MatrixX3i tris;
            MatrixX3f rr = m_sourceSpace[h].rr;

            //Centralize
            rr.col(0) = rr.col(0).array() - m_vecBoundingBoxCenter.x();
            rr.col(1) = rr.col(1).array() - m_vecBoundingBoxCenter.y();
            rr.col(2) = rr.col(2).array() - m_vecBoundingBoxCenter.z();

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
                tris = m_qListLabels[k].selectTris(m_sourceSpace[h].tris);

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

                    if(m_iColorMode == 0)
                    {
                        r = 100;
                        g = 100;
                        b = 100;
                    }
                    else if(m_iColorMode == 1)
                    {
                        r = m_qListRGBAs[k][0];
                        g = m_qListRGBAs[k][1];
                        b = m_qListRGBAs[k][2];
                    }

//                    t_pMaterialROI->setColor(QColor(r,g,b,200));
                    t_pMaterialROI->setColor(QColor(r,g,b,230));
//                        t_pMaterialROI->setEmittedLight(QColor(100,100,100,255));
//                        t_pMaterialROI->setSpecularColor(QColor(10,10,10,20));

                    index = palette->addMaterial(t_pMaterialROI);
                    builder.currentNode()->setMaterialIndex(index);

                    m_qListMapLabelIdIndex[h].insert(m_qListLabels[k].label_id, index);
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

    //
    // Set stereo type
    //
    if (m_bStereo) {
//        this->setStereoType(QGLView::RedCyanAnaglyph);
        this->setStereoType(QGLView::StretchedLeftRight);
//        camera()->setEyeSeparation(0.4f);
//        m_pCameraFrontal->setEyeSeparation(0.1f);

        //LNdT DEMO
        camera()->setCenter(QVector3D(0,0,m_fOffsetZ));//0.8f*fac));
        camera()->setEyeSeparation(0.4f);
        camera()->setFieldOfView(30);
        camera()->setEye(QVector3D(0,0,m_fOffsetZEye));
        //LNdT DEMO end

    }

//    //set background to light grey-blue
//    glClearColor(0.8f, 0.8f, 1.0f, 0.0f);

//    //set background to light white
    glClearColor(1.0f, 1.0f, 1.0f, 0.0f);

    //start the producer
    m_pInverseViewProducer->start();
}


//*************************************************************************************************************

void InverseView::paintGL(QGLPainter *painter)
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

    m_pSceneNode->draw(painter);


    painter->modelViewMatrix().pop();
    painter->projectionMatrix().pop();
}


//*************************************************************************************************************

void InverseView::keyPressEvent(QKeyEvent *e)
{
    camera()->setCenter(QVector3D(0,0,0));

    float normEyeOld = sqrt(pow(camera()->eye().x(),2) + pow(camera()->eye().y(),2) + pow(camera()->eye().z(),2));

    QGLView::keyPressEvent(e);

    float dx = (camera()->eye().x()*m_fOffsetZ)/m_fOffsetZEye;
    float dy = (camera()->eye().y()*m_fOffsetZ)/m_fOffsetZEye;
    float dz = (camera()->eye().z()*m_fOffsetZ)/m_fOffsetZEye;

    float normEye = sqrt(pow(camera()->eye().x(),2) + pow(camera()->eye().y(),2) + pow(camera()->eye().z(),2));
    float scaleEye = normEyeOld/normEye;//m_fOffsetZEye/normEye;
    camera()->setEye(QVector3D(camera()->eye().x()*scaleEye,camera()->eye().y()*scaleEye,camera()->eye().z()*scaleEye));

    camera()->setCenter(QVector3D(dx,dy,dz));
}


//*************************************************************************************************************

void InverseView::mouseMoveEvent(QMouseEvent *e)
{
    camera()->setCenter(QVector3D(0,0,0));

    float normEyeOld = sqrt(pow(camera()->eye().x(),2) + pow(camera()->eye().y(),2) + pow(camera()->eye().z(),2));

    QGLView::mouseMoveEvent(e);

    float dx = (camera()->eye().x()*m_fOffsetZ)/m_fOffsetZEye;
    float dy = (camera()->eye().y()*m_fOffsetZ)/m_fOffsetZEye;
    float dz = (camera()->eye().z()*m_fOffsetZ)/m_fOffsetZEye;

    float normEye = sqrt(pow(camera()->eye().x(),2) + pow(camera()->eye().y(),2) + pow(camera()->eye().z(),2));
    float scaleEye = normEyeOld/normEye;//m_fOffsetZEye/normEye;
    camera()->setEye(QVector3D(camera()->eye().x()*scaleEye,camera()->eye().y()*scaleEye,camera()->eye().z()*scaleEye));

    camera()->setCenter(QVector3D(dx,dy,dz));
}


//*************************************************************************************************************

void InverseView::mousePressEvent(QMouseEvent *e)
{

    if(e->buttons() & Qt::RightButton)
    {
        float normEye = sqrt(pow(camera()->eye().x(),2) + pow(camera()->eye().y(),2) + pow(camera()->eye().z(),2));
        camera()->setCenter(QVector3D(0,0,m_fOffsetZ));
        camera()->setEye(QVector3D(0,0,normEye));
    }

    QGLView::mousePressEvent(e);
}



//*************************************************************************************************************

//QGLSceneNode *InverseView::createScene()
//{
//    QGLBuilder builder;
//    QGLSceneNode *root = builder.sceneNode();

//    //completed building, so finalise
//    return builder.finalizedSceneNode();
//}


//*************************************************************************************************************

void InverseView::updateActivation(QSharedPointer<Eigen::VectorXd> p_pVecActivation)
{
    VectorXd t_curLabelActivation = VectorXd::Zero(m_pSceneNode->palette()->size());

    for(qint32 h = 0; h < 2; ++h)
    {
        for(qint32 i = 0; i < m_sourceSpace[h].cluster_info.numClust(); ++i)
        {
            qint32 labelId = m_sourceSpace[h].cluster_info.clusterLabelIds[i];
            qint32 colorIdx = m_qListMapLabelIdIndex[h][labelId];
            //search for max activation within one label - by checking if there is already an assigned value

            if(abs(t_curLabelActivation[colorIdx]) < abs((*p_pVecActivation.data())[i]))//m_curSourceEstimate.data(i, currentSample)))
                t_curLabelActivation[colorIdx] = (*p_pVecActivation.data())[i];//m_curSourceEstimate.data(i, currentSample);
        }
    }

    for(qint32 i = 0; i < m_pSceneNode->palette()->size(); ++i)
    {
        if(m_pInverseViewProducer->getMaxActivation()[i] != 0)
        {
            qint32 iVal = (t_curLabelActivation[i]/m_pInverseViewProducer->getGlobalMax()) * 255;//1200;//255;

            iVal = iVal > 255 ? 255 : iVal < 0 ? 0 : iVal;

//            int r, g, b;
            QRgb qRgb;
//            if(m_iColorMode == 0)
//            {
////                r = iVal;
////                g = iVal;
////                b = iVal;
//                qRgb = ColorMap::valueToHotNegative1((double)iVal/255.0);
                qRgb = ColorMap::valueToHotNegative2((double)iVal/255.0);
//            }
//            else if(m_iColorMode == 1)
//            {
////                r = iVal;
////                g = iVal;
////                b = iVal;
////                qRgb = ColorMap::valueToHot((double)iVal/255.0);
//                qRgb = ColorMap::valueToHotNegative2((double)iVal/255.0);
//            }

            m_pSceneNode->palette()->material(i)->setSpecularColor(QColor(qRgb));
                        //QColor(r,g,b,200));
        }
    }

    this->update();
}
