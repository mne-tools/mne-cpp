//=============================================================================================================
/**
* @file     brainobject.h
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
* @brief    BrainObject class declaration
*
*/

#ifndef BRAINOBJECT_H
#define BRAINOBJECT_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "disp3dnew_global.h"

#include "../helpers/renderable3Dentity.h"

#include <fs/surfaceset.h>
#include <fs/annotationset.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>

#include <Qt3DCore/QEntity>


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


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


//=============================================================================================================
/**
* Holds the data for brain visualization.
*
* @brief Holds the data for brain visualization.
*/
class DISP3DNEWSHARED_EXPORT BrainObject : public Renderable3DEntity
{
    Q_OBJECT

public:
    typedef QSharedPointer<BrainObject> SPtr;             /**< Shared pointer type for BrainObject class. */
    typedef QSharedPointer<const BrainObject> ConstSPtr;  /**< Const shared pointer type for BrainObject class. */

    //=========================================================================================================
    /**
    * Default constructor using freesurfer data as input
    *
    */
    BrainObject(const Surface &tSurface, Qt3DCore::QEntity *parent = 0);

    //=========================================================================================================
    /**
    * Default destructor
    */
    ~BrainObject();

protected:
    QString     m_sFilePath;        /**< Path to surf directory. */
    QString     m_sFileName;        /**< Surface file name. */
    qint32      m_iHemi;            /**< Hemisphere (lh = 0; rh = 1) */
    QString     m_sSurf;            /**< Loaded surface (eg. inflated, orig ...) */
    VectorXf    m_vecCurv;          /**< FreeSurfer curvature data */
    Vector3f    m_vecOffset;        /**< Surface offset */
};

} // NAMESPACE

#endif // BRAINOBJECT_H
