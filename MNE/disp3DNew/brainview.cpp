//=============================================================================================================
/**
* @file     brainview.cpp
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     February, 2015
*
* @section  LICENSE
*
* Copyright (C) 2015, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
* @brief    Implementation of the BrainView class with qt3d 2.0 support.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "brainview.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include "qglbuilder.h"
#include "qglcube.h"

#include <QMouseEvent>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISP3DNEWLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

BrainView::BrainView()
: m_pSceneNodeBrain(NULL)
, m_pSceneNode(NULL)
, m_pLightModel(NULL)
, m_pLightParametersScene(NULL)
{
    init();
}


//*************************************************************************************************************

BrainView::BrainView(const QString &subject_id, qint32 hemi, const QString &surf, const QString &subjects_dir)
: m_SurfaceSet(subject_id, hemi, surf, subjects_dir)
, m_pSceneNodeBrain(NULL)
, m_pSceneNode(NULL)
, m_pLightModel(NULL)
, m_pLightParametersScene(NULL)
{
    init();
}


//*************************************************************************************************************

BrainView::BrainView(const QString &subject_id, qint32 hemi, const QString &surf, const QString &atlas, const QString &subjects_dir)
: m_SurfaceSet(subject_id, hemi, surf, subjects_dir)
, m_AnnotationSet(subject_id, hemi, atlas, subjects_dir)
, m_pSceneNodeBrain(NULL)
, m_pSceneNode(NULL)
, m_pLightModel(NULL)
, m_pLightParametersScene(NULL)
{
    init();
}


//*************************************************************************************************************

BrainView::BrainView(const QString& p_sFile)
: m_pSceneNodeBrain(NULL)
, m_pSceneNode(NULL)
, m_pLightModel(NULL)
, m_pLightParametersScene(NULL)
{
    Surface t_Surf(p_sFile);
    m_SurfaceSet.insert(t_Surf);

    init();
}


//*************************************************************************************************************

BrainView::~BrainView()
{
    if(m_pSceneNode)
    {
        delete m_pSceneNode;
        m_pSceneNode = NULL;
    }
}


//*************************************************************************************************************

void BrainView::init()
{
    m_bStereo = true;
    m_bRenderPerVertex = false;
    m_fOffsetZ = -100.0f;
    m_fOffsetZEye = 60.0f;
}


//*************************************************************************************************************

void BrainView::initializeGL(QGLPainter *painter)
{
    //
    // Generate surface scene
    //
    qDebug() << "m_SurfaceSet.surf()" << m_SurfaceSet.surf();
    if(QString::compare(m_SurfaceSet.surf(),"inflated") == 0)// || m_viewOptionFlags.testFlag(BrainView::ShowCurvature))
    {
        genSurfacePerVertex();
    }
    else
    {
        genSurface();
    }


    //
    // Create light models
    //
    m_pLightModel = new QGLLightModel(this);
    m_pLightModel->setAmbientSceneColor(Qt::white);
    m_pLightModel->setViewerPosition(QGLLightModel::LocalViewer);

    m_pLightParametersScene = new QGLLightParameters(this);
    m_pLightParametersScene->setPosition(QVector3D(0.0f, 0.0f, 3.0f));
    painter->setMainLight(m_pLightParametersScene);

    //
    // Set stereo type
    //
    if (m_bStereo) {
//        this->setStereoType(QGLView::RedCyanAnaglyph);
        this->setStereoType(QGLView::StretchedLeftRight);

        camera()->setCenter(QVector3D(0,0,m_fOffsetZ));//0.8f*fac));
        camera()->setEyeSeparation(0.4f);
        camera()->setFieldOfView(30);
        camera()->setEye(QVector3D(0,0,m_fOffsetZEye));
    }
    else
    {
        camera()->setCenter(QVector3D(0,0,m_fOffsetZ));//0.8f*fac));
        camera()->setFieldOfView(30);
        camera()->setEye(QVector3D(0,0,m_fOffsetZEye));
    }

    //set background to light white
    glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
}


//*************************************************************************************************************

void BrainView::paintGL(QGLPainter *painter)
{
    glEnable(GL_BLEND); // enable transparency

    //    painter->modelViewMatrix().rotate(45.0f, 1.0f, 1.0f, 1.0f);


    painter->modelViewMatrix().push();
    painter->projectionMatrix().push();

    painter->setStandardEffect(QGL::LitMaterial);
//        painter->setCamera(m_pCameraFrontal);
    painter->setLightModel(m_pLightModel);

    if(m_bRenderPerVertex)
    {
        material.bind(painter);
        material.prepareToDraw(painter, painter->attributes());
    }

    if(m_pSceneNode)
        m_pSceneNode->draw(painter);


    painter->modelViewMatrix().pop();
    painter->projectionMatrix().pop();
}


//*************************************************************************************************************

void BrainView::keyPressEvent(QKeyEvent *e)
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

void BrainView::mouseMoveEvent(QMouseEvent *e)
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

void BrainView::mousePressEvent(QMouseEvent *e)
{

    if(e->buttons() & Qt::RightButton)
    {
        float normEye = sqrt(pow(camera()->eye().x(),2) + pow(camera()->eye().y(),2) + pow(camera()->eye().z(),2));
        camera()->setCenter(QVector3D(0,0,m_fOffsetZ));
        camera()->setEye(QVector3D(0,0,normEye));
    }

    QGLView::mousePressEvent(e);
}


//private
//*************************************************************************************************************

void BrainView::genSurfacePerVertex()
{
    if(m_SurfaceSet.size() == 0)
        return;

    if(m_pSceneNode)
    {
        delete m_pSceneNode;
        m_pSceneNode = NULL;
    }

    // in the constructor construct a builder on the stack
    QGLBuilder builder;

    float fac = 100.0f; // too small vertices distances cause clipping errors --> 100 is a good value for freesurfer brain measures

    builder << QGL::Smooth;//QGL::Faceted;
    m_pSceneNodeBrain = builder.currentNode();

    builder.pushNode();

    //
    // get bounding box
    //
    calcBoundingBox();

    //
    // Build each surface in its separate node
    //
    QMap<qint32, Surface>::const_iterator it = m_SurfaceSet.data().constBegin();
    for (it = m_SurfaceSet.data().begin(); it != m_SurfaceSet.data().end(); ++it)
    {
        builder.pushNode();
        {
            Matrix3Xf rr = it.value().rr().transpose();

            //Centralize
            for(qint32 i = 0; i < 3; ++i)
                rr.row(i) = rr.row(i).array() - (m_vecBoundingBoxCenter[i] + it.value().offset()[i]);


            QGeometryData t_GeometryDataTri;

            MatrixXf t_TriCoords = MatrixXf::Zero(3,3*(it.value().tris().rows()));
            QArray<QColor4ub> cdata;
            for(qint32 i = 0; i < it.value().tris().rows(); ++i)
            {
                for(qint32 j = 0; j < 3; ++j)
                {
                    t_TriCoords.col(i*3+j) = rr.col( it.value().tris()(i,j) );

                    if(it.value().curv()[it.value().tris()(i,j)] >= 0)
                        cdata.append(QColor( 50, 50, 50, 230));//Sulci
                    else
                        cdata.append(QColor( 200, 200, 200, 230));//Gyri
                }
            }

            t_TriCoords *= fac;
            t_GeometryDataTri.appendVertexArray(QArray<QVector3D>::fromRawData( reinterpret_cast<const QVector3D*>(t_TriCoords.data()), t_TriCoords.cols() ));

            t_GeometryDataTri.appendColorArray(cdata);

            //
            //  Add triangles to current node
            //
            builder.addTriangles(t_GeometryDataTri);
        }
        // Go one level up
        builder.popNode();
    }

    m_bRenderPerVertex = true;

    // Optimze current scene for display and calculate lightning normals
    m_pSceneNode = builder.finalizedSceneNode();

    m_pSceneNode->setParent(this);
}


//*************************************************************************************************************

void BrainView::genSurfacePerRegion()
{
    if(m_SurfaceSet.size() == 0)
        return;

    if(m_pSceneNode)
    {
        delete m_pSceneNode;
        m_pSceneNode = NULL;
    }

    // in the constructor construct a builder on the stack
    QGLBuilder builder;

    float fac = 100.0f; // too small vertices distances cause clipping errors --> 100 is a good value for freesurfer brain measures

    builder << QGL::Smooth;//QGL::Faceted;
    m_pSceneNodeBrain = builder.currentNode();

    builder.pushNode();

    //
    // Collor palette
    //
    qint32 index;
    QSharedPointer<QGLMaterialCollection> palette = builder.sceneNode()->palette(); // register color palette within the root node

    //
    // get bounding box
    //
    calcBoundingBox();

    //
    // Build each surface in its separate node
    //
    QMap<qint32, Surface>::const_iterator it = m_SurfaceSet.data().constBegin();
    for (it = m_SurfaceSet.data().begin(); it != m_SurfaceSet.data().end(); ++it)
    {
        builder.pushNode();
        {
            Matrix3Xf rr = it.value().rr().transpose();

            //Centralize
            for(qint32 i = 0; i < 3; ++i)
                rr.row(i) = rr.row(i).array() - m_vecBoundingBoxCenter[i];


            QGeometryData t_GeometryDataTri;

            MatrixXf t_TriCoords = MatrixXf::Zero(3,3*(it.value().tris().rows()));
            for(qint32 i = 0; i < it.value().tris().rows(); ++i)
                for(qint32 j = 0; j < 3; ++j)
                    t_TriCoords.col(i*3+j) = rr.col( it.value().tris()(i,j) );

            t_TriCoords *= fac;
            t_GeometryDataTri.appendVertexArray(QArray<QVector3D>::fromRawData( reinterpret_cast<const QVector3D*>(t_TriCoords.data()), t_TriCoords.cols() ));

            //
            //  Add triangles to current node
            //
            builder.addTriangles(t_GeometryDataTri);

            //
            // Colorize Surface
            //
            QGLMaterial *t_pMaterialROI = new QGLMaterial();
            t_pMaterialROI->setColor(QColor(100,100,100,230));
            index = palette->addMaterial(t_pMaterialROI);
            builder.currentNode()->setMaterialIndex(index);
        }
        // Go one level up
        builder.popNode();
    }

    m_bRenderPerVertex = false;

    // Optimze current scene for display and calculate lightning normals
    m_pSceneNode = builder.finalizedSceneNode();

    m_pSceneNode->setParent(this);
}


//*************************************************************************************************************

void BrainView::genSurface()
{
    if(m_SurfaceSet.size() == 0)
        return;

    if(m_pSceneNode)
    {
        delete m_pSceneNode;
        m_pSceneNode = NULL;
    }

    // in the constructor construct a builder on the stack
    QGLBuilder builder;

    float fac = 100.0f; // too small vertices distances cause clipping errors --> 100 is a good value for freesurfer brain measures

    builder << QGL::Smooth;//QGL::Faceted;
    m_pSceneNodeBrain = builder.currentNode();

    builder.pushNode();

    //
    // Collor palette
    //
    qint32 index;
    QSharedPointer<QGLMaterialCollection> palette = builder.sceneNode()->palette(); // register color palette within the root node

    //
    // get bounding box
    //
    calcBoundingBox();

    //
    // Build each surface in its separate node
    //

    QMap<qint32, Surface>::const_iterator it = m_SurfaceSet.data().constBegin();
    for (it = m_SurfaceSet.data().begin(); it != m_SurfaceSet.data().end(); ++it)
    {
        builder.pushNode();
        {
            Matrix3Xf rr = it.value().rr().transpose();

            //Centralize
            for(qint32 i = 0; i < 3; ++i)
                rr.row(i) = rr.row(i).array() - m_vecBoundingBoxCenter[i];


            QGeometryData t_GeometryDataTri;

            MatrixXf t_TriCoords = MatrixXf::Zero(3,3*(it.value().tris().rows()));
            for(qint32 i = 0; i < it.value().tris().rows(); ++i)
                for(qint32 j = 0; j < 3; ++j)
                    t_TriCoords.col(i*3+j) = rr.col( it.value().tris()(i,j) );

            t_TriCoords *= fac;
            t_GeometryDataTri.appendVertexArray(QArray<QVector3D>::fromRawData( reinterpret_cast<const QVector3D*>(t_TriCoords.data()), t_TriCoords.cols() ));

            //
            //  Add triangles to current node
            //
            builder.addTriangles(t_GeometryDataTri);

            //
            // Colorize Surface
            //
            QGLMaterial *t_pMaterialROI = new QGLMaterial();
            t_pMaterialROI->setColor(QColor(100,100,100,230));
            index = palette->addMaterial(t_pMaterialROI);
            builder.currentNode()->setMaterialIndex(index);
        }
        // Go one level up
        builder.popNode();
    }

    m_bRenderPerVertex = false;

    // Optimze current scene for display and calculate lightning normals
    m_pSceneNode = builder.finalizedSceneNode();

    m_pSceneNode->setParent(this);
}


//*************************************************************************************************************

void BrainView::calcBoundingBox()
{
    QMap<qint32, Surface>::const_iterator it = m_SurfaceSet.data().constBegin();

    QVector3D min(it.value().rr().col(0).minCoeff(), it.value().rr().col(1).minCoeff(), it.value().rr().col(2).minCoeff());
    QVector3D max(it.value().rr().col(0).maxCoeff(), it.value().rr().col(1).maxCoeff(), it.value().rr().col(2).maxCoeff());

    for (it = m_SurfaceSet.data().begin()+1; it != m_SurfaceSet.data().end(); ++it)
    {
        for(qint32 i = 0; i < 3; ++i)
        {
            min[i] = min[i] > it.value().rr().col(i).minCoeff() ? it.value().rr().col(i).minCoeff() : min[i];
            max[i] = max[i] < it.value().rr().col(i).maxCoeff() ? it.value().rr().col(i).maxCoeff() : max[i];
        }
    }
    m_vecBoundingBoxMin = min;
    m_vecBoundingBoxMax = max;

    for(qint32 i = 0; i < 3; ++i)
        m_vecBoundingBoxCenter[i] = (m_vecBoundingBoxMin[i]+m_vecBoundingBoxMax[i])/2.0f;
}
