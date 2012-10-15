//=============================================================================================================
/**
* @file     geometryview.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hämäläinen <msh@nmr.mgh.harvard.edu>
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
* @brief    ToDo Documentation...
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



#include <iostream>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

GeometryView::GeometryView(QWindow *parent)
: QGLView(parent)
, t_ForwardSolution(0)
, hemisphere(0)
, scene(0)
{
    QString t_sFile = "./MNE-sample-data/MEG/sample/sample_audvis-meg-eeg-oct-6-fwd.fif";

    if(!MNE::read_forward_solution(t_sFile, t_ForwardSolution))
    {
        if( t_ForwardSolution )
            delete t_ForwardSolution;

        t_ForwardSolution = NULL;
    }

    hemisphereFrontalCamera = new QGLCamera(this);
    hemisphereFrontalCamera->setAdjustForAspectRatio(false);

}

//*************************************************************************************************************

GeometryView::~GeometryView()
{
    delete hemisphere;
}

//*************************************************************************************************************

void GeometryView::initializeGL(QGLPainter *painter)
{
    if(t_ForwardSolution)
    {
        // in the constructor construct a builder on the stack
        QGLBuilder builder;

        float fac = 10.0f;

        builder << QGL::Faceted;
        hemisphere = builder.currentNode();
        builder.pushNode();
        QGLSceneNode *leftHemisphere = builder.newNode();
        {
            QGeometryData tri;

            MatrixXf* triCoords = t_ForwardSolution->src->hemispheres.at(0)->getTriCoords(fac);

            tri.appendVertexArray(QArray<QVector3D>::fromRawData( reinterpret_cast<const QVector3D*>(triCoords->data()), triCoords->cols() ));

            //builder << *(t_ForwardSolution->src->hemispheres.at(0)->getGeometryData(fac));
            //builder.addTriangles(*t_ForwardSolution->src->hemispheres.at(0)->getGeometry(fac));
            builder.addTriangles(tri);// << tri;
        }

        QGLSceneNode *rightHemisphere = builder.newNode();
        {
            QGeometryData tri;

            MatrixXf* triCoords = t_ForwardSolution->src->hemispheres.at(1)->getTriCoords(fac);

            tri.appendVertexArray(QArray<QVector3D>::fromRawData( reinterpret_cast<const QVector3D*>(triCoords->data()), triCoords->cols()));

            builder.addTriangles(tri);// << tri;
        }
        builder.popNode();



        qint32 index;
        QSharedPointer<QGLMaterialCollection> palette = builder.sceneNode()->palette();

        QGLMaterial *matLH = new QGLMaterial();
        matLH->setDiffuseColor(Qt::white);
        index = palette->addMaterial(matLH);
        leftHemisphere->setMaterialIndex(index);

        QGLMaterial *matRH = new QGLMaterial();
        matRH->setDiffuseColor(Qt::yellow);
        index = palette->addMaterial(matRH);
        rightHemisphere->setMaterialIndex(index);

        scene = builder.finalizedSceneNode();

        scene->setParent(this);

        hemisphereLightModel = new QGLLightModel(this);
        hemisphereLightModel->setAmbientSceneColor(Qt::white);
        hemisphereLightModel->setViewerPosition(QGLLightModel::LocalViewer);

        hemisphereLightModel = new QGLLightModel(this);

        lightParameters = new QGLLightParameters(this);
        lightParameters->setPosition(QVector3D(0.0f, 0.0f, 3.0f));
        painter->setMainLight(lightParameters);

    //    if (stereo) {
            this->setStereoType(QGLView::RedCyanAnaglyph);
            camera()->setEyeSeparation(0.4f);
            hemisphereFrontalCamera->setEyeSeparation(0.1f);
    //    }

    }

}

//*************************************************************************************************************

void GeometryView::paintGL(QGLPainter *painter)
{
    if(t_ForwardSolution)
    {
    //    painter->modelViewMatrix().rotate(45.0f, 1.0f, 1.0f, 1.0f);

        painter->modelViewMatrix().push();
        painter->projectionMatrix().push();

        painter->setStandardEffect(QGL::LitMaterial);
    //    painter->setCamera(hemisphereFrontalCamera);
        painter->setLightModel(hemisphereLightModel);

        hemisphere->draw(painter);

        painter->modelViewMatrix().pop();
        painter->projectionMatrix().pop();
    }
}
