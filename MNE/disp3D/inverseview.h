//=============================================================================================================
/**
* @file     inverseview.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
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
* @brief    InverseView class declaration
*
*/

#ifndef INVERSEVIEW_H
#define INVERSEVIEW_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "disp3D_global.h"

#include "inverseviewproducer.h"

#include <mne/mne_sourcespace.h>
#include <fs/surfaceset.h>
#include <mne/mne_sourceestimate.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include "qglview.h"
#include <QGeometryData>
#include <QGLColorMaterial>
#include <QSharedPointer>
#include <QList>
#include <QMap>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class QTimer;

namespace FSLIB
{
class Label;
}


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

using namespace MNELIB;
using namespace FSLIB;
using namespace Eigen;


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


//=============================================================================================================
/**
* ToDo: derive this from geometryview!
* Visualize labels using a stereoscopic view. Coloring is done per label.
*
* @brief 3D stereoscopic labels
*/
class DISP3DSHARED_EXPORT InverseView : public QGLView
{
    Q_OBJECT
public:
    typedef QSharedPointer<InverseView> SPtr;            /**< Shared pointer type for InverseView class. */
    typedef QSharedPointer<const InverseView> ConstSPtr; /**< Const shared pointer type for InverseView class. */
    
    //=========================================================================================================
    /**
    * Default constructor
    *
    * @param[in] p_sourceSpace  Source space which contains the geometry information
    * @param[in] p_qListLabels  region of interest labels
    * @param[in] p_qListRGBAs   color information for given region of interest
    * @param[in] p_iFps         Frames per second
    * @param[in] p_bLoop        if current source estimate should be repeated
    * @param[in] p_bStereo      if stereo view should be turned on
    * @param[in] parent         Parent QObject (optional)
    */
    InverseView(const MNESourceSpace &p_sourceSpace, QList<Label> &p_qListLabels, QList<RowVector4i> &p_qListRGBAs, qint32 p_iFps = 24, bool p_bLoop = true, bool p_bStereo = false, QWindow *parent = 0);
    
    //=========================================================================================================
    /**
    * Destroys the InverseView class.
    */
    ~InverseView();

    //=========================================================================================================
    /**
    * Appends a new source estimate to the internal inverse producer
    *
    * @param[in] p_sourceEstimate   Source estimate to push
    */
    void pushSourceEstimate(MNESourceEstimate &p_sourceEstimate);

protected:
    //=========================================================================================================
    /**
    * Initializes the current GL context represented by painter.
    *
    * @param[in] painter    GL painter which should be initialized
    */
    void initializeGL(QGLPainter *painter);

    //=========================================================================================================
    /**
    * Paints the scene onto painter. The color and depth buffers will have already been cleared, and the camera() position set.
    *
    * @param[in] painter    GL painter which is updated
    */
    void paintGL(QGLPainter *painter);

    //=========================================================================================================
    /**
    * Processes the key press event e.
    *
    * @param[in] e      the key press event.
    */
    void keyPressEvent(QKeyEvent *e);

    //=========================================================================================================
    /**
    * Processes the mouse move event e.
    *
    * @param[in] e      the mouse move event.
    */
    void mouseMoveEvent(QMouseEvent *e);

    //=========================================================================================================
    /**
    * Processes the mouse press event e.
    *
    * @param[in] e      the mouse press event.
    */
    void mousePressEvent(QMouseEvent *e);

private:

    InverseViewProducer::SPtr m_pInverseViewProducer;   /**< Inverse view producer. */

    //Data Stuff
    MNESourceSpace m_sourceSpace;                   /**< The used source space. */
    QList<Label> m_qListLabels;                     /**< The labels. */
    QList<RowVector4i> m_qListRGBAs;                /**< The label colors encoded in RGBA. */

    qint32 m_iColorMode;                            /**< used colorization mode. */

    //GL Stuff
    bool m_bStereo;

    QGLLightModel *m_pLightModel;                   /**< The selected light model. */
    QGLLightParameters *m_pLightParametersScene;    /**< The selected light parameters. */

    QGLColorMaterial material;

    QGLSceneNode *m_pSceneNodeBrain;                /**< Scene node of the hemisphere models. */
    QVector3D m_vecBoundingBoxMin;                  /**< X, Y, Z minima. */
    QVector3D m_vecBoundingBoxMax;                  /**< X, Y, Z maxima. */
    QVector3D m_vecBoundingBoxCenter;               /**< X, Y, Z center. */

    float m_fOffsetZ;                               /**< Z offset for pop-out effect. */
    float m_fOffsetZEye;                            /**< Z offset eye. */


    QGLSceneNode *m_pSceneNode;                     /**< Node of the scene. */

//    QGLCamera *m_pCameraFrontal;     /**< frontal camera. */


    QList< QMap<qint32, qint32> > m_qListMapLabelIdIndex;

    //=========================================================================================================
    /**
    * update source activation
    *
    * @param[in] p_pVecActivation   new activation vector.
    */
    void updateActivation(QSharedPointer<Eigen::VectorXd> p_pVecActivation);

    //=========================================================================================================
    /**
    * Creates the scene
    *
    * @return the root scene noode
    */
//    QGLSceneNode *createScene();

};

//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================


} // NAMESPACE

#endif // INVERSEVIEW_H

