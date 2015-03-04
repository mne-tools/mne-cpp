//=============================================================================================================
/**
* @file     brainsurface.h
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
* @brief    Declaration of BrainSurface which holds the basic functionality of a freesurfer brain surface.
*
*/

#ifndef BRAINSURFACE_H
#define BRAINSURFACE_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "disp3DNew_global.h"

#include "brainhemisphere.h"

#include "../helpers/renderableentity.h"
#include "../models/stcdatamodel.h"

#include <fs/surfaceset.h>
#include <fs/annotationset.h>

#include <disp/colormap.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <Qt3DRenderer/qt3drenderer_global.h>
#include <Qt3DCore/QAbstractBuffer>
#include <QByteArray>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE DISP3DLIB
//=============================================================================================================

namespace DISP3DNEWLIB
{

//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Qt3D;
using namespace FSLIB;
using namespace DISPLIB;


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


//=============================================================================================================
/**
* Basic functionalities for freesurfer surface.
*
* @brief Holds the data of one hemisphere in form of a mesh.
*/
class DISP3DNEWSHARED_EXPORT BrainSurface : public RenderableEntity
{
    Q_OBJECT
public:
    typedef QSharedPointer<BrainSurface> SPtr;             /**< Shared pointer type for BrainSurface class. */
    typedef QSharedPointer<const BrainSurface> ConstSPtr;  /**< Const shared pointer type for BrainSurface class. */

    //=========================================================================================================
    /**
    * Default constructor
    *
    * @param[in] parent         The parent node
    */
    explicit BrainSurface(QEntity *parent = 0);

    //=========================================================================================================
    /**
    * Construts the BrainSurface set by reading it of the given surface.
    *
    * @param[in] subject_id         Name of subject
    * @param[in] hemi               Which hemisphere to load {0 -> lh, 1 -> rh, 2 -> both}
    * @param[in] surf               Name of the surface to load (eg. inflated, orig ...)
    * @param[in] subjects_dir       Subjects directory
    * @param[in] parent             The parent node
    */
    explicit BrainSurface(const QString &subject_id, qint32 hemi, const QString &surf, const QString &subjects_dir, QEntity *parent = 0);

    //=========================================================================================================
    /**
    * Construts the BrainSurface set by reading it of the given surface.
    *
    * @param[in] subject_id         Name of subject
    * @param[in] hemi               Which hemisphere to load {0 -> lh, 1 -> rh, 2 -> both}
    * @param[in] surf               Name of the surface to load (eg. inflated, orig ...)
    * @param[in] atlas              Name of the atlas to load (eg. aparc.a2009s, aparc, aparc.DKTatlas40, BA, BA.thresh, ...)
    * @param[in] subjects_dir       Subjects directory
    * @param[in] parent             The parent node
    */
    explicit BrainSurface(const QString &subject_id, qint32 hemi, const QString &surf, const QString &atlas, const QString &subjects_dir, QEntity *parent = 0);

    //=========================================================================================================
    /**
    * Construts the BrainSurface by reading a given surface.
    *
    * @param[in] p_sFile            Surface file name with path
    * @param[in] parent             The parent node
    */
    explicit BrainSurface(const QString& p_sFile, QEntity *parent = 0);

    //=========================================================================================================
    /**
    * Default destructor
    */
    ~BrainSurface();

    //=========================================================================================================
    /**
    * Updates the current activation.
    *
    */
    void updateActivation();

    //=========================================================================================================
    /**
    * Informs the view about the changed data from the set model
    *
    * @param[in] topLeft
    * @param[in] bottomRight
    * @param[in] roles
    */
    void dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles = QVector<int> ());

    //=========================================================================================================
    /**
    * Sets the data model
    *
    * @param[in] model
    */
    void setModel(StcDataModel::SPtr model);

protected:
    //=========================================================================================================
    /**
    * Initializes the BrainSurface.
    */
    void init();

    //=========================================================================================================
    /**
    * Calculates the bounding box and stores it to m_vecBoundingBoxMin, m_vecBoundingBoxMax and m_vecBoundingBoxCenter.
    */
    void calcBoundingBox();

    BrainHemisphere* m_pLeftHemisphere;
    BrainHemisphere* m_pRightHemisphere;

    SurfaceSet m_SurfaceSet;                        /**< Surface set */
    AnnotationSet m_AnnotationSet;                  /**< Annotation set */
    QList<QColor> m_qlColors;                       /**< current colors which also reflect the activation */

    QVector3D m_vecBoundingBoxMin;                  /**< X, Y, Z minima. */
    QVector3D m_vecBoundingBoxMax;                  /**< X, Y, Z maxima. */
    QVector3D m_vecBoundingBoxCenter;               /**< X, Y, Z center. */

    StcDataModel::SPtr m_pStcDataModel;

    QMap<int, QColor> m_qmDefaultVertexColorLH;
    QMap<int, QColor> m_qmDefaultVertexColorRH;
    QMap<int, QColor> m_qmVertexActivationColorLH;
    QMap<int, QColor> m_qmVertexActivationColorRH;

    QColor m_ColorSulci;
    QColor m_ColorGyri;

private:
};

} // NAMESPACE

#endif // BRAINSURFACE_H
