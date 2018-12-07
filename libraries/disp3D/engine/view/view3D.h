//=============================================================================================================
/**
* @file     view3D.h
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Lars Debor <lars.debor@tu-ilmenau.de>
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     November, 2015
*
* @section  LICENSE
*
* Copyright (C) 2015, Lorenz Esch, Lars Debor and Matti Hamalainen. All rights reserved.
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

#ifndef DISP3DLIB_VIEW3D_H
#define DISP3DLIB_VIEW3D_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../disp3D_global.h"


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

namespace Qt3DRender {
    class QPointLight;
    class QRenderCapture;
    class QRenderCaptureReply;
}


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE DISP3DLIB
//=============================================================================================================

namespace DISP3DLIB
{


//*************************************************************************************************************
//=============================================================================================================
// DISP3DLIB FORWARD DECLARATIONS
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
    */
    explicit View3D(/*QWidget *parent = 0*/);

    //=========================================================================================================
    /**
    * Default destructor
    */
    ~View3D() = default;

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
    void setLightColor(const QColor &color);

    //=========================================================================================================
    /**
    * Set light intensity.
    */
    void setLightIntensity(double value);

    //=========================================================================================================
    /**
    * Renders a screenshot of the view and saves it to the passed path. SVG and PNG supported.
    *
    * @param [in] fileName     The file name and path where to store the screenshot.
    */
    void takeScreenshot();

protected:

    void saveScreenshot();

    //=========================================================================================================
    /**
    * Init the light for the 3D view
    */
    void initLight();

    //=========================================================================================================
    /**
    * Window function
    */
    void keyPressEvent(QKeyEvent* e) override;

    //=========================================================================================================
    /**
    * Creates a coordiante system (x/Green, y/Red, z/Blue).
    *
    * @param[in] parent         The parent identity which will "hold" the coordinate system.
    */
    void createCoordSystem(Qt3DCore::QEntity* parent);

    //=========================================================================================================
    /**
    * Starts the automated rotation animation for all 3D models being childern.
    *
    * @param[in] pObject         The parent of the children to be rotated.
    */
    void startModelRotationRecursive(QObject* pObject);


    QPointer<Qt3DCore::QEntity>                 m_pRootEntity;                  /**< The root/most top level entity buffer. */
    QPointer<Qt3DCore::QEntity>                 m_p3DObjectsEntity;             /**< The root/most top level entity buffer. */
    QPointer<Qt3DCore::QEntity>                 m_pLightEntity;                 /**< The root/most top level entity buffer. */
    QSharedPointer<Qt3DCore::QEntity>           m_pCoordSysEntity;              /**< The entity representing the x/y/z coord system. */

    QPointer<CustomFrameGraph>                  m_pFrameGraph;                  /**< The frameGraph entity. */
    QPointer<Qt3DRender::QCamera>               m_pCamera;                      /**< The camera entity. */
    QPointer<Qt3DRender::QRenderCapture>        m_pCapture;                     /**< The capture object to save screenshots. */
    QPointer<Qt3DRender::QRenderCaptureReply>   m_pReply;                       /**< The capture reply object to save screenshots. */

    QList<QPointer<QPropertyAnimation> >  m_lPropertyAnimations;         /**< The animations for each 3D object. */
    QList<QPointer<Qt3DRender::QPointLight> >  m_lLightSources;          /**< The light sources. */

};

} // NAMESPACE

#endif // DISP3DLIB_VIEW3D_H
