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

#include "disp3d_global.h"
#include "3DObjects/brain/brain.h"
#include "helpers/window.h"
#include "helpers/types.h"

#include <mne/mne_sourceestimate.h>


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
// DEFINE NAMESPACE DISP3DLIB
//=============================================================================================================

namespace DISP3DLIB
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
    * @param[in] text               The text of the surface set tree item which this data should be added to. If no item with text exists it will be created.
    * @param[in] tSurfaceSet        FreeSurfer surface set.
    * @param[in] tAnnotationSet     FreeSurfer annotation set.
    *
    * @return                       Returns true if successful.
    */
    bool addBrainData(const QString& text, const SurfaceSet& tSurfaceSet, const AnnotationSet& tAnnotationSet = AnnotationSet());

    //=========================================================================================================
    /**
    * Adds FreeSurfer single brain data.
    *
    * @param[in] text               The text of the surface set tree item which this data should be added to. If no item with text exists it will be created.
    * @param[in] tSurface           FreeSurfer surface.
    * @param[in] tAnnotation        FreeSurfer annotation.
    *
    * @return                       Returns true if successful.
    */
    bool addBrainData(const QString& text, const Surface& tSurface, const Annotation& tAnnotation = Annotation());

    //=========================================================================================================
    /**
    * Adds source space data to the brain tree model.
    *
    * @param[in] text               The text of the surface set tree item which this data should be added to. If no item with text exists it will be created.
    * @param[in] tSourceSpace       The source space information.
    *
    * @return                       Returns true if successful.
    */
    bool addBrainData(const QString& text, const MNESourceSpace& tSourceSpace);

    //=========================================================================================================
    /**
    * Adds a forward solution data to the brain tree model. Convenient function to addBrainData(const QString& text, const MNESourceSpace& tSourceSpace).
    *
    * @param[in] text               The text of the surface set tree item which this data should be added to. If no item with text exists it will be created.
    * @param[in] tForwardSolution   The forward solution information.
    *
    * @return                       Returns true if successful.
    */
    bool addBrainData(const QString& text, const MNEForwardSolution& tForwardSolution);

    //=========================================================================================================
    /**
    * Adds source activity data to the brain tree model.
    *
    * @param[in] text                   The name of the hemisphere surface set to which this data should be added.
    * @param[in] tSourceEstimate        The MNESourceEstimate data.
    * @param[in] tForwardSolution       The MNEForwardSolution data.
    *
    * @return                           Returns a list of the BrainRTDataTreeItem where the data was appended to.
    */
    QList<BrainRTDataTreeItem*> addRtBrainData(const QString& text, const MNESourceEstimate& tSourceEstimate, const MNEForwardSolution& tForwardSolution = MNEForwardSolution());

    //=========================================================================================================
    /**
    * Return the tree model which holds the brain information.
    *
    * @return          The BrainTreeModel pointer.
    */
    BrainTreeModel* getBrainTreeModel();

    //=========================================================================================================
    /**
    * Set the background color of the scene.
    *
    * @param[in] colSceneColor          The new background color of the view.
    */
    void setSceneColor(const QColor& colSceneColor);

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

    Brain::SPtr                         m_pBrain;                       /**< Pointer to the Brain class, which holds all BrainObjects. */

    bool            m_bCameraTransMode;         /**< Flag for activating/deactivating the translation camera mode. */
    bool            m_bCameraRotationMode;      /**< Flag for activating/deactivating the rotation camera mode. */

    QPoint          m_mousePressPositon;        /**< Position when the mouse was pressed. */

    QVector3D       m_vecCameraTrans;           /**< The camera translation vector. */
    QVector3D       m_vecCameraTransOld;        /**< The camera old translation vector. */
    QVector3D       m_vecCameraRotation;        /**< The camera rotation vector. */
    QVector3D       m_vecCameraRotationOld;     /**< The camera old rotation vector. */

    //=========================================================================================================
    /**
    * Init the meta types
    */
    void initMetatypes();

    //=========================================================================================================
    /**
    * Init the 3D view
    */
    void init();

    //=========================================================================================================
    /**
    * Init the 3D views transformation matrices
    */
    void initTransformations();

    //=========================================================================================================
    /**
    * Window functions
    */
    void keyPressEvent(QKeyEvent* e);
    void mousePressEvent(QMouseEvent* e);
    void wheelEvent(QWheelEvent* e);
    void mouseReleaseEvent(QMouseEvent* e);
    void mouseMoveEvent(QMouseEvent* e);

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
