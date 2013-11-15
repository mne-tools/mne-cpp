//=============================================================================================================
/**
* @file     geometryview.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Daniel Strohmeier <daniel.strohmeier@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     July, 2012
*
* @section  LICENSE
*
* Copyright (C) 2012, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Implementation of the GeometryView Class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "geometryview.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include "qglbuilder.h"
#include "qglcube.h"

#include <QtCore/qurl.h>
#include <QArray>
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


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

GeometryView::GeometryView(const MNESourceSpace &p_sourceSpace, QWindow *parent)
: QGLView(parent)
, m_sourceSpace(p_sourceSpace)
, m_bStereo(true)
, m_fOffsetZ(-100.0f)
, m_fOffsetZEye(60.0f)
, m_pSceneNodeBrain(0)
, m_pSceneNode(0)
{
    m_vecAnnotation.append(Annotation::SPtr(new Annotation("./MNE-sample-data/subjects/sample/label/lh.aparc.a2009s.annot")));
    m_vecAnnotation.append(Annotation::SPtr(new Annotation("./MNE-sample-data/subjects/sample/label/rh.aparc.a2009s.annot")));

//    m_pCameraFrontal = new QGLCamera(this);
//    m_pCameraFrontal->setAdjustForAspectRatio(false);
}


//*************************************************************************************************************

GeometryView::~GeometryView()
{
    delete m_pSceneNode;
}

//*************************************************************************************************************

void GeometryView::initializeGL(QGLPainter *painter)
{
    if(!m_sourceSpace.isEmpty())
    {
        // in the constructor construct a builder on the stack
        QGLBuilder builder;

        float fac = 100.0f;

        builder << QGL::Faceted;
        m_pSceneNodeBrain = builder.currentNode();

        builder.pushNode();

        // Collor palette
        qint32 index;
        QSharedPointer<QGLMaterialCollection> palette = builder.sceneNode()->palette(); // register color palette within the root node

        //get bounding box // ToDo separate function
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

                MatrixX3i tris = m_sourceSpace[h].tris;
                MatrixX3f rr = m_sourceSpace[h].rr;

                //Centralize
                rr.col(0) = rr.col(0).array() - m_vecBoundingBoxCenter.x();
                rr.col(1) = rr.col(1).array() - m_vecBoundingBoxCenter.y();
                rr.col(2) = rr.col(2).array() - m_vecBoundingBoxCenter.z();

                builder.pushNode();
                //
                // Create each ROI in its own node
                //
                for(qint32 k = 0; k < m_vecAnnotation[h]->getColortable().numEntries; ++k)
                {
                    // add new ROI node when current ROI node is not empty
                    if(builder.currentNode()->count() > 0)
                        builder.newNode();

                    QGeometryData t_GeometryDataTri;

                    MatrixXf t_TriCoords(3,3*tris.rows());
                    qint32 t_size = 0;
                    qint32 t_label_ids = m_vecAnnotation[h]->getColortable().table(k,4);

                    for(qint32 i = 0; i < tris.rows(); ++i)
                    {
                        if(m_vecAnnotation[h]->getLabelIds()(tris(i,0)) == t_label_ids || m_vecAnnotation[h]->getLabelIds()(tris(i,1)) == t_label_ids || m_vecAnnotation[h]->getLabelIds()(tris(i,2)) == t_label_ids)
                        {
                            t_TriCoords.col(t_size*3) = rr.row( tris(i,0) ).transpose();
                            t_TriCoords.col(t_size*3+1) = rr.row( tris(i,1) ).transpose();
                            t_TriCoords.col(t_size*3+2) = rr.row( tris(i,2) ).transpose();
                            ++t_size;
                        }
                    }
                    t_TriCoords.conservativeResize(3, 3*t_size);
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
                        r = m_vecAnnotation[h]->getColortable().table(k,0);
                        g = m_vecAnnotation[h]->getColortable().table(k,1);
                        b = m_vecAnnotation[h]->getColortable().table(k,2);

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

        //
        // Set stereo type
        //
        if (m_bStereo) {
            this->setStereoType(QGLView::RedCyanAnaglyph);
//            this->setStereoType(QGLView::StretchedLeftRight);
//            camera()->setEyeSeparation(0.4f);
//            m_pCameraFrontal->setEyeSeparation(0.1f);

            //LNdT DEMO
            camera()->setCenter(QVector3D(0,0,m_fOffsetZ));//0.8f*fac));
            camera()->setEyeSeparation(0.4f);
            camera()->setFieldOfView(30);
            camera()->setEye(QVector3D(0,0,m_fOffsetZEye));
            //LNdT DEMO end

//            m_pCameraFrontal->setEyeSeparation(0.1f);
        }

        //set background to white
        glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
//        //set background to light grey-blue
//        glClearColor(0.8f, 0.8f, 1.0f, 0.0f);
    }

}

//*************************************************************************************************************

void GeometryView::paintGL(QGLPainter *painter)
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

void GeometryView::keyPressEvent(QKeyEvent *e)
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

void GeometryView::mouseMoveEvent(QMouseEvent *e)
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

void GeometryView::mousePressEvent(QMouseEvent *e)
{

    if(e->buttons() & Qt::RightButton)
    {
        float normEye = sqrt(pow(camera()->eye().x(),2) + pow(camera()->eye().y(),2) + pow(camera()->eye().z(),2));
        camera()->setCenter(QVector3D(0,0,m_fOffsetZ));
        camera()->setEye(QVector3D(0,0,normEye));
    }

    QGLView::mousePressEvent(e);
}

