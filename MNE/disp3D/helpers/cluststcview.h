//=============================================================================================================
/**
* @file     cluststcview.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
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
* @brief    ClustStcView class declaration
*
*/

#ifndef CLUSTSTCVIEW_H
#define CLUSTSTCVIEW_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp3D_global.h"


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include "qglview.h"
#include <QGeometryData>
#include <QGLColorMaterial>
#include <QSharedPointer>
#include <QList>
#include <QVector>
#include <QMap>


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

class ClustStcModel;

//=============================================================================================================
/**
* 3D view to display cluster stc model data in a stereoscopic view
*
* @brief 3D Display
*/
class DISP3DSHARED_EXPORT ClustStcView : public QGLView
{
    Q_OBJECT
public:
    typedef QSharedPointer<ClustStcView> SPtr;            /**< Shared pointer type for ClustStcView class. */
    typedef QSharedPointer<const ClustStcView> ConstSPtr; /**< Const shared pointer type for ClustStcView class. */

    ClustStcView(bool showRegions = true, bool isStereo = true, QGLView::StereoType stereo = QGLView::RedCyanAnaglyph, QWindow *parent = 0);

    void dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles = QVector<int> ());

    void setModel(ClustStcModel* model);

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
    ClustStcModel* m_pModel;

    bool m_bIsInitialized;

    bool m_bShowRegions;

    bool m_bStereo;
    QGLView::StereoType m_stereoType;

    float m_fOffsetZ;                               /**< Z offset for pop-out effect. */
    float m_fOffsetZEye;                            /**< Z offset eye. */
    QGLSceneNode *m_pSceneNodeBrain;                /**< Scene node of the hemisphere models. */
    QGLSceneNode *m_pSceneNode;                     /**< Node of the scene. */

    QGLLightModel *m_pLightModel;                   /**< The selected light model. */
    QGLLightParameters *m_pLightParametersScene;    /**< The selected light parameters. */

    bool m_bColorize;

    QGLColorMaterial material;

    QMap<qint32, qint32> m_qMapLabelIdIndex;

};

} // NAMESPACE

#endif // CLUSTSTCVIEW_H
