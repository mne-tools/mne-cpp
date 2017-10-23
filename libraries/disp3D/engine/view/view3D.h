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
* @brief    View3D class declaration.
*
*/

#ifndef VIEW3D_H
#define VIEW3D_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../disp3D_global.h"

#include <fs/annotationset.h>
#include <fs/annotation.h>
#include <mne/mne_forwardsolution.h>
#include <connectivity/network/network.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <Qt3DExtras/Qt3DWindow>
#include <QVector3D>
#include <QPointer>


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class QPropertyAnimation;

namespace Qt3DCore {
    class QTransform;
}

namespace Qt3DRender {
    class QDirectionalLight;
    class QPointLight;
}

namespace Qt3DExtras {
    class QPhongMaterial;
}


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE DISP3DLIB
//=============================================================================================================

namespace DISP3DLIB
{


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class Data3DTreeModel;
class CustomFrameGraph;


//=============================================================================================================
/**
* Visualizes 3D/2D objects in a 3D space such as brain, DTI, MRI, sensor, helmet data.
*
* @brief Visualizes 3D data
*/
class DISP3DSHARED_EXPORT View3D : public Qt3DExtras::Qt3DWindow
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
    * Return the tree model which holds the subject information.
    *
    * @param[in] pModel     The tree model holding the 3d models.
    */
    void setModel(QSharedPointer<DISP3DLIB::Data3DTreeModel> pModel);

    //=========================================================================================================
    /**
    * Set the background color of the scene.
    *
    * @param[in] colSceneColor          The new background color of the view.
    */
    void setSceneColor(const QColor& colSceneColor);

    void startModelRotationRecursive(QObject* pObject);

    //=========================================================================================================
    /**
    * Starts or stops to rotate all loaded 3D models.
    */
    void startStopModelRotation(bool checked);

    //=========================================================================================================
    /**
    * Toggle the coord axis visibility.
    */
    void toggleCoordAxis(bool checked);

    //=========================================================================================================
    /**
    * Show fullscreen.
    */
    void showFullScreen(bool checked);

    //=========================================================================================================
    /**
    * Change light color.
    */
    void setLightColor(QColor color);

    //=========================================================================================================
    /**
    * Set light intensity.
    */
    void setLightIntensity(double value);

protected:
    //=========================================================================================================
    /**
    * Init the 3D view
    */
    void init();

    //=========================================================================================================
    /**
    * Init the light for the 3D view
    */
    void initLight();

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
    void keyReleaseEvent(QKeyEvent* e);
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

    //=========================================================================================================
    /**
    * Sets the rotation for all 3D models being children.
    *
    * @param[in] obj         The parent of the children to be rotated.
    */
    void setRotationRecursive(QObject* obj);

    QPointer<Qt3DCore::QEntity>         m_pRootEntity;                  /**< The root/most top level entity buffer. */
    QPointer<Qt3DCore::QEntity>         m_p3DObjectsEntity;             /**< The root/most top level entity buffer. */
    QPointer<Qt3DCore::QEntity>         m_pLightEntity;                 /**< The root/most top level entity buffer. */
    QPointer<Qt3DRender::QCamera>       m_pCameraEntity;                /**< The camera entity. */
    QPointer<CustomFrameGraph>          m_pFrameGraph;                   /**< The frameGraph entity. */

    QSharedPointer<Qt3DCore::QEntity>   m_XAxisEntity;                  /**< The entity representing a torus in x direction. */
    QSharedPointer<Qt3DCore::QEntity>   m_YAxisEntity;                  /**< The entity representing a torus in y direction. */
    QSharedPointer<Qt3DCore::QEntity>   m_ZAxisEntity;                  /**< The entity representing a torus in z direction. */

    QPointer<Qt3DCore::QTransform>      m_pCameraTransform;             /**< The main camera transform. */

    bool                                m_bCameraTransMode;             /**< Flag for activating/deactivating the translation camera mode. */
    bool                                m_bRotationMode;                /**< Flag for activating/deactivating the rotation mode. */
    bool                                m_bModelRotationMode;           /**< Flag for activating/deactivating the rotation model mode (camera is default). */

    QPoint                              m_mousePressPositon;            /**< Position when the mouse was pressed. */

    QVector3D                           m_vecViewTrans;               /**< The camera translation vector. */
    QVector3D                           m_vecViewTransOld;            /**< The camera old translation vector. */
    QVector3D                           m_vecViewRotation;            /**< The camera rotation vector. */
    QVector3D                           m_vecViewRotationOld;         /**< The camera old rotation vector. */
    QVector3D                           m_vecModelRotation;           /**< The model rotation vector. */
    QVector3D                           m_vecModelRotationOld;        /**< The model old rotation vector. */

    QList<QPointer<QPropertyAnimation> >  m_lPropertyAnimations;        /**< The animations for each 3D object. */
    QList<QPair<QPointer<Qt3DRender::QPointLight> , QPointer<Qt3DExtras::QPhongMaterial> > >  m_lLightSources;        /**< The light sources. */
};

} // NAMESPACE

#endif // VIEW3D_H
