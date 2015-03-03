//=============================================================================================================
/**
* @file     brainview.h
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     February, 2015
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
* @brief    BrainView class declaration with qt3d 2.0 support
*
*/

#ifndef BRAINVIEW_H
#define BRAINVIEW_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "disp3DNew_global.h"

#include "brainSurface/brainsurface.h"
#include "models/stcdatamodel.h"

#include <fs/surfaceset.h>
#include <fs/annotationset.h>

#include "MNE/mne_forwardsolution.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QString>
#include <QSharedPointer>

#include <Qt3DCore/window.h>
#include <Qt3DCore/qcamera.h>
#include <Qt3DCore/qentity.h>
#include <Qt3DCore/qcameralens.h>
#include <Qt3DCore/qaspectengine.h>

#include <Qt3DInput/QInputAspect>

#include <Qt3DRenderer/qcameraselector.h>
#include <Qt3DRenderer/qrenderpassfilter.h>
#include <Qt3DRenderer/qforwardrenderer.h>
#include <Qt3DRenderer/qviewport.h>
#include <Qt3DRenderer/qrenderaspect.h>
#include <Qt3DRenderer/qframegraph.h>
#include <Qt3DRenderer/qclearbuffer.h>

#include <Qt3DRenderer/qcylindermesh.h>

#include <Qt3DRenderer/QPointLight>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>


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

using namespace FSLIB;
using namespace Eigen;
using namespace Qt3D;


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


//=============================================================================================================
/**
* ToDo: derive this from geometryview!
* Visualizes FreeSurfer surfaces.
*
* @brief FreeSurfer surface visualisation
*/
class DISP3DNEWSHARED_EXPORT BrainView : public Qt3D::Window
{
    Q_OBJECT
public:
    typedef QSharedPointer<BrainView> SPtr;             /**< Shared pointer type for BrainView class. */
    typedef QSharedPointer<const BrainView> ConstSPtr;  /**< Const shared pointer type for BrainView class. */

    //=========================================================================================================
    /**
    * Default constructor
    *
    */
    explicit BrainView();

    //=========================================================================================================
    /**
    * Constructs the BrainView set by reading it of the given surface.
    *
    * @param[in] subject_id         Name of subject
    * @param[in] hemi               Which hemisphere to load {0 -> lh, 1 -> rh, 2 -> both}
    * @param[in] surf               Name of the surface to load (eg. inflated, orig ...)
    * @param[in] subjects_dir       Subjects directory
    */
    explicit BrainView(const QString &subject_id, qint32 hemi, const QString &surf, const QString &subjects_dir);

    //=========================================================================================================
    /**
    * Constructs the BrainView set by reading it of the given surface.
    *
    * @param[in] subject_id         Name of subject
    * @param[in] hemi               Which hemisphere to load {0 -> lh, 1 -> rh, 2 -> both}
    * @param[in] surf               Name of the surface to load (eg. inflated, orig ...)
    * @param[in] atlas              Name of the atlas to load (eg. aparc.a2009s, aparc, aparc.DKTatlas40, BA, BA.thresh, ...)
    * @param[in] subjects_dir       Subjects directory
    */
    explicit BrainView(const QString &subject_id, qint32 hemi, const QString &surf, const QString &atlas, const QString &subjects_dir);

    //=========================================================================================================
    /**
    * Constructs the BrainView by reading a given surface.
    *
    * @param[in] p_sFile            Surface file name with path
    */
    explicit BrainView(const QString& p_sFile);

    //=========================================================================================================
    /**
    * Default destructor
    */
    ~BrainView();

    //=========================================================================================================
    /**
    * Appends a new source estimate to the internal inverse producer
    *
    * @param[in] p_sourceEstimate   Source estimate to push
    */
    void addSourceEstimate(MNESourceEstimate &p_sourceEstimate);


    void initStcDataModel(const QString &subject_id, qint32 hemi, const QString &surf, const QString &subjects_dir, const QString &atlas, const MNEForwardSolution &forwardSolution);

protected:
    //=========================================================================================================
    /**
    * Initializes the Brain View.
    *
    * @param[in] p_sFile            Surface file name with path
    * @param[in] subject_id         Name of subject
    * @param[in] hemi               Which hemisphere to load {0 -> lh, 1 -> rh, 2 -> both}
    * @param[in] surf               Name of the surface to load (eg. inflated, orig ...)
    * @param[in] atlas              Name of the atlas to load (eg. aparc.a2009s, aparc, aparc.DKTatlas40, BA, BA.thresh, ...)
    * @param[in] subjects_dir       Subjects directory
    */
    void init(const QString& p_sFile, const QString &subject_id, qint32 hemi, const QString &surf, const QString &atlas, const QString &subjects_dir);

    void createCoordSystem(QEntity *rootEntity);

    //=========================================================================================================
    /**
    * Processes the mouse press event e.
    *
    * @param[in] e      the mouse press event.
    */
    void mousePressEvent(QMouseEvent *e);

    QAspectEngine m_Engine;
    QInputAspect *m_pAspectInput;
    QVariantMap m_data;

    Qt3D::QEntity * m_pRootEntity;
    QSharedPointer<Qt3D::QEntity> m_XAxisEntity;
    QSharedPointer<Qt3D::QEntity> m_YAxisEntity;
    QSharedPointer<Qt3D::QEntity> m_ZAxisEntity;

    BrainSurface::SPtr m_pBrainSurfaceEntity;

    StcDataModel::SPtr m_pStcDataModel;

private:

};

} // NAMESPACE

#endif // BRAINVIEW_H
