//=============================================================================================================
/**
* @file     types.h
* @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     December, 2015
*
* @section  LICENSE
*
* Copyright (C) 2015, Lorenz Esch, Matti Hamalainen. All rights reserved.
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
* @brief    Contains general application specific types
*
*/
#ifndef TYPES_H
#define TYPES_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "renderable3Dentity.h"


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QStandardItem>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE DISP3DNEWLIB
//=============================================================================================================

namespace DISP3DNEWLIB
{
// Typedefs

// Model item types
namespace BrainTreeModelItemTypes
{
    enum ItemType{UnknownItem = QStandardItem::UserType,
                    SurfaceSetItem = QStandardItem::UserType + 1,
                    HemisphereItem = QStandardItem::UserType + 2,
                    SurfaceItem = QStandardItem::UserType + 3,
                    AnnotationItem = QStandardItem::UserType + 4,
                    SurfaceFileName = QStandardItem::UserType + 5,
                    SurfaceFilePath = QStandardItem::UserType + 6,
                    AnnotFileName = QStandardItem::UserType + 7,
                    AnnotFilePath = QStandardItem::UserType + 8,
                    SurfaceType = QStandardItem::UserType + 9,
                    SurfaceColorGyri = QStandardItem::UserType + 10,
                    SurfaceColorSulci = QStandardItem::UserType + 11,
                    SurfaceColorVert = QStandardItem::UserType + 12,
                    SurfaceColorInfoOrigin = QStandardItem::UserType + 13,
                    RTDataItem = QStandardItem::UserType + 14,
                    RTDataStreamStatus = QStandardItem::UserType + 15, //RT data streaming on or off
                    RTDataSourceSpaceType = QStandardItem::UserType + 16, //Clustered or full sourcespace
                    RTDataColormapType = QStandardItem::UserType + 17}; //Type of the colormap used
}

// Model item roles
namespace BrainTreeItemRoles
{
    enum ItemRole{SurfaceFileName = Qt::UserRole,
                    SurfaceType = Qt::UserRole + 1,
                    SurfaceColorSulci = Qt::UserRole + 3,
                    SurfaceColorGyri = Qt::UserRole + 4,
                    SurfaceFilePath = Qt::UserRole + 5,
                    AnnotName = Qt::UserRole + 6,
                    AnnotFilePath = Qt::UserRole + 7,
                    SurfaceColorInfoOrigin = Qt::UserRole + 8,
                    RTDataStreamStatus = Qt::UserRole + 9,
                    RTDataSourceSpaceType = Qt::UserRole + 10,
                    RTDataColormapType = Qt::UserRole + 11};
}

namespace BrainSurfaceTreeItemRoles
{
    enum ItemRole{SurfaceCurrentColorVert = Qt::UserRole + 100,
                    SurfaceVert = Qt::UserRole + 101,
                    SurfaceTris = Qt::UserRole + 102,
                    SurfaceNorm = Qt::UserRole + 103,
                    SurfaceCurv = Qt::UserRole + 104,
                    SurfaceOffset = Qt::UserRole + 105,
                    SurfaceRenderable3DEntity = Qt::UserRole + 106,
                    SurfaceCurvatureColorVert = Qt::UserRole + 107,
                    SurfaceRTSourceLocColor = Qt::UserRole + 108};
}

namespace BrainSurfaceSetTreeItemRoles
{
    enum ItemRole{SurfaceSetName = Qt::UserRole + 200};
}

namespace BrainHemisphereTreeItemRoles
{
    enum ItemRole{SurfaceHemi = Qt::UserRole + 300};
}

namespace BrainAnnotationTreeItemRoles
{
    enum ItemRole{AnnotColors = Qt::UserRole + 400,
                    AnnotFileName = Qt::UserRole + 401,
                    AnnotFilePath = Qt::UserRole + 402};
}

namespace BrainRTDataTreeItemRoles
{
    enum ItemRole{RTData = Qt::UserRole + 500,
                    RTVertices = Qt::UserRole + 501,
                    RTTimes = Qt::UserRole + 502};
}

} //NAMESPACE DISP3DNEWLIB

// Metatype declaration for correct QVariant usage
#ifndef metatype_renderable3Dentity
#define metatype_renderable3Dentity
Q_DECLARE_METATYPE(DISP3DNEWLIB::Renderable3DEntity*)
#endif

#ifndef metatype_matrixx3i
#define metatype_matrixx3i
Q_DECLARE_METATYPE(Eigen::MatrixX3i)
#endif

#ifndef metatype_matrixXd
#define metatype_matrixXd
Q_DECLARE_METATYPE(Eigen::MatrixXd)
#endif

#ifndef metatype_matrixx3f
#define metatype_matrixx3f
Q_DECLARE_METATYPE(Eigen::MatrixX3f)
#endif

#ifndef metatype_vectorxf
#define metatype_vectorxf
Q_DECLARE_METATYPE(Eigen::VectorXf)
#endif

#ifndef metatype_vectorxi
#define metatype_vectorxi
Q_DECLARE_METATYPE(Eigen::VectorXi)
#endif

#ifndef metatype_rowvectorxf
#define metatype_rowvectorxf
Q_DECLARE_METATYPE(Eigen::RowVectorXf)
#endif

#ifndef metatype_vector3f
#define metatype_vector3f
Q_DECLARE_METATYPE(Eigen::Vector3f)
#endif

#endif // TYPES_H
