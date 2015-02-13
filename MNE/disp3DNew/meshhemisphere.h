//=============================================================================================================
/**
* @file     meshhemisphere.h
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
* @brief    Declaration of MeshHemisphere which holds the data of one hemisphere in form of a mesh.
*
*/

#ifndef MESHHEMISPHERE_H
#define MESHHEMISPHERE_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "disp3DNew_global.h"
#include <mne/mne_sourcespace.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <Qt3DRenderer/qt3drenderer_global.h>
#include <Qt3DRenderer/qabstractmesh.h>


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

class MeshHemispherePrivate;


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Qt3D;
using namespace MNELIB;


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


//=============================================================================================================
/**
* Holds the data of one hemisphere in form of a mesh.
*
* @brief Holds the data of one hemisphere in form of a mesh.
*/
class DISP3DNEWSHARED_EXPORT MeshHemisphere : public QAbstractMesh
{
    Q_OBJECT
public:
    //=========================================================================================================
    /**
    * Default constructor
    *
    * @param[in] parent         The parent node
    */
    explicit MeshHemisphere(QNode *parent = 0);

    //=========================================================================================================
    /**
    * Default constructor
    *
    * @param[in] sourceSpace  Source space which contains the geometry information
    * @param[in] parent         The parent node
    */
    explicit MeshHemisphere(const MNESourceSpace &sourceSpace, QNode *parent = 0);

    QAbstractMeshFunctorPtr meshFunctor() const;

protected:
    void copy(const QNode *ref);

    MNESourceSpace m_sourceSpace;

private:
    Q_DECLARE_PRIVATE(MeshHemisphere)
    QT3D_CLONEABLE(MeshHemisphere)
};


class DISP3DNEWSHARED_EXPORT MeshHemisphereFunctor : public QAbstractMeshFunctor
{

public:
    MeshHemisphereFunctor(const MNESourceSpace &sourceSpace);
    QMeshDataPtr operator ()();
    bool operator ==(const QAbstractMeshFunctor &other) const;

private:
    MNESourceSpace m_sourceSpace;
};


} // NAMESPACE

#endif // MESHHEMISPHERE_H
