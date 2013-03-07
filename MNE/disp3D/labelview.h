//=============================================================================================================
/**
* @file     labelview.h
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
* @brief    LabelView class declaration
*
*/

#ifndef LABELVIEW_H
#define LABELVIEW_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "disp3D_global.h"

#include <mne/mne.h>
#include <fs/surface.h>
#include <inverse/sourceestimate.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include "qglview.h"
#include <QGeometryData>
#include <QGLColorMaterial>
#include <QSharedPointer>
#include <QList>


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
// DEFINE NAMESPACE FSLIB
//=============================================================================================================

namespace DISP3DLIB
{

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNELIB;
using namespace FSLIB;
using namespace INVERSELIB;


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


//=============================================================================================================
/**
* Visualize labels using a stereoscopic view. Coloring is done per label.
*
* @brief 3D stereoscopic labels
*/
class DISP3DSHARED_EXPORT LabelView : public QGLView
{
    Q_OBJECT
public:
    typedef QSharedPointer<LabelView> SPtr;            /**< Shared pointer type for LabelView class. */
    typedef QSharedPointer<const LabelView> ConstSPtr; /**< Const shared pointer type for LabelView class. */
    
    //=========================================================================================================
    /**
    * Default constructor
    *
    * @param[in] parent     Parent QObject (optional)
    */
    LabelView(Surface &p_surf, QList<Label> &p_qListLabels, QList<RowVector4i> &p_qListRGBAs, QWindow *parent = 0);
    
    //=========================================================================================================
    /**
    * Destroys the LabelView class.
    */
    ~LabelView();


    void pushSourceEstimate(SourceEstimate &p_sourceEstimate);


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

private:

    //Data Stuff
    Surface m_surf;                                 /**< The surface which should be displayed. */
    QList<Label> m_qListLabels;                     /**< The labels. */
    QList<RowVector4i> m_qListRGBAs;                /**< The label colors encoded in RGBA. */

    //GL Stuff
    bool m_bStereo;

    QGLLightModel *m_pLightModel;                   /**< The selected light model. */
    QGLLightParameters *m_pLightParametersScene;    /**< The selected light parameters. */

    QGLColorMaterial material;

    QGLSceneNode *m_pSceneNodeBrain;               /**< Scene node of the hemisphere models. */
    QGLSceneNode *m_pSceneNode;                    /**< Node of the scene. */

    QGLCamera *m_pCameraFrontal;     /**< frontal camera. */




    SourceEstimate m_curSourceEstimate;
    RowVectorXd m_vecFirstLabelSourceEstimate;
    double m_dMaxSourceEstimate;

    qint32 simCount;
    qint32 m_nTSteps;
    QTimer *m_timer;
    void updateData();





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

#endif // LABELVIEW_H

