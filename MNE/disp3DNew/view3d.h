//=============================================================================================================
/**
* @file     view3D.h
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     November, 2015
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
* @brief    View3D class declaration with new Qt3D support
*
*/

#ifndef VIEW3D_H
#define VIEW3D_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <iostream>
#include "disp3dnew_global.h"

#include "3DObjects/brain/brain.h"

#include "helpers/window.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QWidget>
#include <QWindow>
#include <QDebug>

#include <Qt3DCore/QAspectEngine>
#include <Qt3DCore/QCamera>
#include <Qt3DCore/QTransform>

#include <Qt3DRender/QPhongMaterial>
#include <Qt3DRender/QPerVertexColorMaterial>
#include <Qt3DRender/QRenderAspect>
#include <Qt3DRender/QFrameGraph>
#include <Qt3DRender/QForwardRenderer>
#include <Qt3DRender/QPointLight>
#include <Qt3DRender/QCylinderMesh>

#include <Qt3DInput/QInputAspect>


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE DISP3DNEWLIB
//=============================================================================================================

namespace DISP3DNEWLIB
{

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


//=============================================================================================================
/**
* Visualizes 3D/2D objects in a 3D space such as brain, DTI, MRI, sensor, helmet data.
*
* @brief Visualizes 3D data
*/
class DISP3DNEWSHARED_EXPORT View3D : public Window
{
    Q_OBJECT

public:
    typedef QSharedPointer<View3D> SPtr;             /**< Shared pointer type for View3D class. */
    typedef QSharedPointer<const View3D> ConstSPtr;  /**< Const shared pointer type for View3D class. */

    //=========================================================================================================
    /**
    * Default constructor
    *
    */
    explicit View3D(/*QWidget *parent = 0*/);

    //=========================================================================================================
    /**
    * Default destructor
    */
    ~View3D();

    //=========================================================================================================
    /**
    * Adds FreeSurfer brain data SET.
    *
    * @param[in] tSurfaceSet        FreeSurfer surface set.
    * @param[in] tAnnotationSet     FreeSurfer annotation set.
    *
    * @return                       Returns true if successful.
    */
    bool addBrainData(const SurfaceSet& tSurfaceSet, const AnnotationSet& tAnnotationSet = AnnotationSet());

    //=========================================================================================================
    /**
    * Adds FreeSurfer single brain data.
    *
    * @param[in] tSurface           FreeSurfer surface.
    * @param[in] tAnnotation        FreeSurfer annotation.
    *
    * @return                       Returns true if successful.
    */
    bool addBrainData(const Surface& tSurface, const Annotation& tAnnotation = Annotation());

    BrainTreeModel* getBrainTreeModel();

    void changeSceneColor(const QColor & colSceneColor);

protected:
    Qt3DCore::QAspectEngine             m_aspectEngine;                 /**< The aspect engine. */
    Qt3DCore::QEntity*                  m_pRootEntity;                  /**< The root/most top level entity buffer. */
    Qt3DInput::QInputAspect*            m_pInputAspect;                 /**< The input aspect. */
    Qt3DCore::QCamera*                  m_pCameraEntity;                /**< The camera entity. */
    Qt3DRender::QFrameGraph*            m_pFrameGraph;                  /**< The frame graph holding the render information. */
    Qt3DRender::QForwardRenderer*       m_pForwardRenderer;             /**< The renderer (here forward renderer). */

    QSharedPointer<Qt3DCore::QEntity>   m_XAxisEntity;                  /**< The entity representing a torus in x direction. */
    QSharedPointer<Qt3DCore::QEntity>   m_YAxisEntity;                  /**< The entity representing a torus in y direction. */
    QSharedPointer<Qt3DCore::QEntity>   m_ZAxisEntity;                  /**< The entity representing a torus in z direction. */

    Qt3DCore::QTransform*               m_pCameraTransform;             /**< The main camera transform. */
    Qt3DCore::QTransform*               m_pCameraScaleTransform;        /**< The camera scale transformation (added to m_pCameraTransform). */
    Qt3DCore::QTransform*               m_pCameraTranslateTransform;    /**< The camera translation transformation (added to m_pCameraTransform). */
    Qt3DCore::QTransform*               m_pCameraRotateTransformX;      /**< The camera x-axis rotation transformation (added to m_pCameraTransform). */
    Qt3DCore::QTransform*               m_pCameraRotateTransformY;      /**< The camera y-axis rotation transformation (added to m_pCameraTransform). */
    Qt3DCore::QTransform*               m_pCameraRotateTransformZ;      /**< The camera z-axis rotation transformation (added to m_pCameraTransform). */

    Brain::SPtr                         m_pBrain;                       /**< Pointer to the Brain class, which holds all BrainObjects. */

    bool            m_bCameraTransMode;         /**< Flag for activating/deactivating the translation camera mode. */
    bool            m_bModelRotationMode;       /**< Flag for activating/deactivating the rotation model mode. */
    bool            m_bCameraRotationMode;      /**< Flag for activating/deactivating the rotation camera mode. */

    QPoint          m_mousePressPositon;        /**< Position when the mouse was pressed. */

    float           m_fModelScale;              /**< The current camera scaling factor. */
    float           m_fCameraScale;             /**< The current camera scaling factor. */

    QVector3D       m_vecCameraTrans;           /**< The camera translation vector. */
    QVector3D       m_vecCameraTransOld;        /**< The camera old translation vector. */
    QVector3D       m_vecCameraRotation;        /**< The camera rotation vector. */
    QVector3D       m_vecCameraRotationOld;     /**< The camera old rotation vector. */
    QVector3D       m_vecModelRotation;         /**< The model rotation vector. */
    QVector3D       m_vecModelRotationOld;      /**< The model old rotation vector. */

    //=========================================================================================================
    /**
    * Init the 3D view
    *
    */
    void init();

    //=========================================================================================================
    /**
    * Init the 3D views transformation matrices
    *
    */
    void initTransformations();

    //=========================================================================================================
    /**
    * Virtual functions for mouse and keyboard control
    *
    */
    void keyPressEvent(QKeyEvent *e);
    void mousePressEvent(QMouseEvent *e);
    void wheelEvent(QWheelEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);
    void mouseMoveEvent(QMouseEvent *e);

    //=========================================================================================================
    /**
    * Creates a coordiante system (x/Green, y/Red, z/Blue).
    *
    * @param[in] parent         The parent identity which will "hold" the coordinate system.
    */
    void createCoordSystem(Qt3DCore::QEntity *parent);
};

} // NAMESPACE

#endif // VIEW3D_H
