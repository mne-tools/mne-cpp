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

#include "disp3dnew_global.h"

#include "3DObjects/brain.h"

#include "helpers/window.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>

#include <QWindow>

#include <QDebug>

#include <Qt3DCore/QAspectEngine>
#include <Qt3DCore/QCamera>
#include <Qt3DCore/QTransform>
#include <Qt3DCore/QScaleTransform>

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
    explicit View3D();

    //=========================================================================================================
    /**
    * Default destructor
    */
    ~View3D();

    //=========================================================================================================
    /**
    * Adds FreeSurfer brain data.
    *
    * @param[in] subject_id         Name of subject
    * @param[in] hemi               Which hemisphere to load {0 -> lh, 1 -> rh, 2 -> both}
    * @param[in] surf               Name of the surface to load (eg. inflated, orig ...)
    * @param[in] subjects_dir       Subjects directory
    */
    bool addFsBrainData(const QString &subject_id, qint32 hemi, const QString &surf, const QString &subjects_dir);

protected:
    Qt3DCore::QAspectEngine         m_aspectEngine;
    Qt3DCore::QEntity*              m_pRootEntity;
    Qt3DInput::QInputAspect*        m_pInputAspect;
    Qt3DCore::QCamera*              m_pCameraEntity;
    Qt3DRender::QFrameGraph*        m_pFrameGraph;
    Qt3DRender::QForwardRenderer*   m_pForwardRenderer;
    Qt3DCore::QTransform*           m_pTransform;
    Qt3DCore::QScaleTransform*      m_pScaleTransform;
    Qt3DCore::QTranslateTransform*  m_pTranslateTransform;
    Qt3DCore::QRotateTransform*     m_pRotateTransformX;
    Qt3DCore::QRotateTransform*     m_pRotateTransformY;

    QSharedPointer<Qt3DCore::QEntity> m_XAxisEntity;
    QSharedPointer<Qt3DCore::QEntity> m_YAxisEntity;
    QSharedPointer<Qt3DCore::QEntity> m_ZAxisEntity;

    Brain::SPtr          m_pBrain;

    bool    m_bDragMode;                /**< Flag which defines drag mode by pressing the middle mouse button.*/
    bool    m_bZoomMode;                /**< Flag which defines zoom mode by pressing the right mouse button.*/
    bool    m_bRotationMode;            /**< Flag which defines rotation mode by pressing the left mouse button.*/

    QPoint  m_mousePressPositon;
    float m_rotationXOld;				/**< Saves data from the mouse x rotation.*/
    float m_rotationYOld;				/**< Saves data from the mouse y rotation.*/
    float m_dragXOld;					/**< Saves data from the mouse x movement.*/
    float m_dragYOld;					/**< Saves data from the mouse y movement.*/
    float m_zoomOld;					/**< Saves data from the mouse zoom position.*/
    float m_rotationX;					/**< Holds data from the mouse x rotation.*/
    float m_rotationY;					/**< Holds data from the mouse y rotation.*/
    float m_scalefactor;				/**< Holds the scaling factor used when rendering the 3D model.*/
    float m_X;
    float m_Y;
    float m_Z;

    //=========================================================================================================
    /**
    * Init the 3D view
    */
    void init();

    void keyPressEvent(QKeyEvent *e);
    void mousePressEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);
    void mouseMoveEvent(QMouseEvent *e);

    void createCoordSystem(Qt3DCore::QEntity *rootEntity);

private:

};

} // NAMESPACE

#endif // VIEW3D_H
