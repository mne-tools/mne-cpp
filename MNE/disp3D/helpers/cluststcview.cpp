//=============================================================================================================
/**
* @file     cluststcview.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Daniel Strohmeier <daniel.strohmeier@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
* @version  1.0
* @date     June, 2014
*
* @section  LICENSE
*
* Copyright (C) 2014, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Implementation of the ClustStcView class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "cluststcview.h"
#include "cluststcmodel.h"

#include <disp/helpers/colormap.h>


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include "qglbuilder.h"
#include "qglcube.h"
#include <QMouseEvent>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;
using namespace DISP3DLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ClustStcView::ClustStcView(bool showRegions, bool isStereo, QGLView::StereoType stereoType, QWindow *parent)
: QGLView(parent)
, m_pModel(Q_NULLPTR)
, m_bIsInitialized(false)
, m_bShowRegions(showRegions)
, m_bStereo(isStereo)
, m_stereoType(stereoType)//QGLView::StretchedLeftRight)//QGLView::RedCyanAnaglyph
, m_pSceneNodeBrain(Q_NULLPTR)
, m_pSceneNode(Q_NULLPTR)
, m_pLightModel(Q_NULLPTR)
, m_pLightParametersScene(Q_NULLPTR)
{
    m_fOffsetZ = -100.0f;
    m_fOffsetZEye = 60.0f;
}


//*************************************************************************************************************

ClustStcView::~ClustStcView()
{

}


//*************************************************************************************************************

void ClustStcView::dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles)
{
    if(!m_bIsInitialized)
        return;

    //check wether realtive stc data column (3) has changed
    if(topLeft.column() > 3 || bottomRight.column() < 3)
        return;

    for(qint32 i = 0; i < m_pSceneNode->palette()->size(); ++i)
    {
        //ToDo label id check -> necessary?
        //qint32 colorIdx = m_qListMapLabelIdIndex[h][labelId];

        qint32 iVal = m_pModel->data(i,3).value<VectorXd>().maxCoeff() * 255;

        iVal = iVal > 255 ? 255 : iVal < 0 ? 0 : iVal;

        QRgb qRgb;
        qRgb = ColorMap::valueToHotNegative1((double)iVal/255.0);
//        qRgb = ColorMap::valueToHotNegative2((double)iVal/255.0);
//        qRgb = ColorMap::valueToHot((double)iVal/255.0);

        m_pSceneNode->palette()->material(i)->setSpecularColor(QColor(qRgb));
    }

    this->update();

    Q_UNUSED( roles );

}


//*************************************************************************************************************

void ClustStcView::setModel(ClustStcModel* model)
{
    m_pModel = model;
    connect(m_pModel, &ClustStcModel::dataChanged, this, &ClustStcView::dataChanged);
}


//*************************************************************************************************************

void ClustStcView::initializeGL(QGLPainter *painter)
{
    if(!m_pModel)
        return;

    // in the constructor construct a builder on the stack
    QGLBuilder builder;

    float fac = 100.0f; // too small vertices distances cause clipping errors --> 100 is a good value for freesurfer brain measures

    builder << QGL::Faceted;
    if(m_pSceneNodeBrain)
        delete m_pSceneNodeBrain;
    m_pSceneNodeBrain = builder.currentNode();

    builder.pushNode();

    // Collor palette
    qint32 index;
    QSharedPointer<QGLMaterialCollection> palette = builder.sceneNode()->palette(); // register color palette within the root node

    m_qMapLabelIdIndex.clear();

//    //
//    // Build each hemisphere in its separate node
//    //
//    for(qint32 h = 0; h < 1; ++h)
//    {
        builder.newNode();//create new hemisphere node
        {
            //
            // Create each ROI in its own node
            //
            for(qint32 k = 0; k < m_pModel->rowCount(); ++k)
            {
                // add new ROI node when current ROI node is not empty
                if(builder.currentNode()->count() > 0)
                    builder.newNode();

                QGeometryData t_GeometryDataTri;

                Matrix3Xf t_TriCoords = m_pModel->data(k,6,Qt::DisplayRole).value<Matrix3Xf>();

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

                    if(m_bShowRegions)
                        t_pMaterialROI->setColor(m_pModel->data(k,5,Qt::DisplayRole).value<QColor>());
                    else
                        t_pMaterialROI->setColor(QColor(100,100,100,230));

                    index = palette->addMaterial(t_pMaterialROI);
                    builder.currentNode()->setMaterialIndex(index);

                    m_qMapLabelIdIndex.insert(m_pModel->data(k,4,Qt::DisplayRole).value<Label>().label_id, index);
                }
            }
        }
        // Go one level up
        builder.popNode();
//    }
//    // Go one level up
//    builder.popNode();

    // Optimze current scene for display and calculate lightning normals
    if(m_pSceneNode)
        delete m_pSceneNode;
    m_pSceneNode = builder.finalizedSceneNode();
    m_pSceneNode->setParent(this);

    //
    // Create light models
    //
    if(m_pLightModel)
        delete m_pLightModel;
    m_pLightModel = new QGLLightModel(this);
    m_pLightModel->setAmbientSceneColor(Qt::white);
    m_pLightModel->setViewerPosition(QGLLightModel::LocalViewer);

    m_pLightModel = new QGLLightModel(this);

    if(m_pLightParametersScene)
        delete m_pLightParametersScene;
    m_pLightParametersScene = new QGLLightParameters(this);
    m_pLightParametersScene->setPosition(QVector3D(0.0f, 0.0f, 3.0f));
    painter->setMainLight(m_pLightParametersScene);

    //
    // Set stereo type
    //
    if (m_bStereo) {
        this->setStereoType(m_stereoType);
//        camera()->setEyeSeparation(0.4f);
//        m_pCameraFrontal->setEyeSeparation(0.1f);

        //LNdT DEMO
        camera()->setCenter(QVector3D(0,0,m_fOffsetZ));//0.8f*fac));
        camera()->setEyeSeparation(0.4f);
        camera()->setFieldOfView(30);
        camera()->setEye(QVector3D(0,0,m_fOffsetZEye));
        //LNdT DEMO end
    }
    else
    {
        camera()->setCenter(QVector3D(0,0,m_fOffsetZ));
        camera()->setFieldOfView(30);
        camera()->setEye(QVector3D(0,0,m_fOffsetZEye));
    }

//    //set background to light grey-blue
//    glClearColor(0.8f, 0.8f, 1.0f, 0.0f);

//    //set background to light white
    glClearColor(1.0f, 1.0f, 1.0f, 0.0f);

    m_bIsInitialized = true;
}


//*************************************************************************************************************

void ClustStcView::paintGL(QGLPainter *painter)
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

void ClustStcView::keyPressEvent(QKeyEvent *e)
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

void ClustStcView::mouseMoveEvent(QMouseEvent *e)
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

void ClustStcView::mousePressEvent(QMouseEvent *e)
{
    if(e->buttons() & Qt::RightButton)
    {
        float normEye = sqrt(pow(camera()->eye().x(),2) + pow(camera()->eye().y(),2) + pow(camera()->eye().z(),2));
        camera()->setCenter(QVector3D(0,0,m_fOffsetZ));
        camera()->setEye(QVector3D(0,0,normEye));
    }

    QGLView::mousePressEvent(e);
}
