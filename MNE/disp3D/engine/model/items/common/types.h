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

#include <fs/label.h>
#include <inverse/dipoleFit/ecd_set.h>


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QStandardItem>
#include <QByteArray>


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
// DEFINE NAMESPACE DISP3DLIB
//=============================================================================================================

namespace DISP3DLIB
{
// Typedefs

// Model item types
namespace Data3DTreeModelItemTypes
{
    enum ItemType{UnknownItem = QStandardItem::UserType,
                    MeasurementItem = QStandardItem::UserType + 1,
                    HemisphereItem = QStandardItem::UserType + 2,
                    SurfaceItem = QStandardItem::UserType + 3,
                    AnnotationItem = QStandardItem::UserType + 4,
                    MNEEstimateItem = QStandardItem::UserType + 5,
                    SourceSpaceItem = QStandardItem::UserType + 6,
                    NetworkItem = QStandardItem::UserType + 7,
                    SubjectItem = QStandardItem::UserType + 8,
                    BemItem = QStandardItem::UserType + 9,
                    BemSurfaceItem = QStandardItem::UserType + 10,
                    DigitizerSetItem = QStandardItem::UserType + 11,
                    DigitizerItem = QStandardItem::UserType + 12,
                    ECDDataItem = QStandardItem::UserType + 13,
                    MriItem = QStandardItem::UserType + 14,
                    SensorSetItem = QStandardItem::UserType + 15,
                    SensorSurfaceItem = QStandardItem::UserType + 16,
                    SensorPositionItem = QStandardItem::UserType + 17,
                    AbstractSurfaceItem = QStandardItem::UserType + 18};
}

namespace MetaTreeItemTypes
{
    enum ItemType{FileName = QStandardItem::UserType + 100,
                    FilePath = QStandardItem::UserType + 101,
                    SurfaceColorGyri = QStandardItem::UserType + 103,
                    SurfaceColorSulci = QStandardItem::UserType + 104,
                    SurfaceColorVert = QStandardItem::UserType + 105,
                    AlphaValue = QStandardItem::UserType + 106,
                    StreamStatus = QStandardItem::UserType + 107,
                    SourceSpaceType = QStandardItem::UserType + 108,
                    ColormapType = QStandardItem::UserType + 109,
                    StreamingTimeInterval = QStandardItem::UserType + 110,
                    LoopedStreaming = QStandardItem::UserType + 111,
                    NumberAverages = QStandardItem::UserType + 112,
                    DistributedSourceLocThreshold = QStandardItem::UserType + 113,
                    VisualizationType = QStandardItem::UserType + 114,
                    Color = QStandardItem::UserType + 115,
                    UnknownItem = QStandardItem::UserType + 116,
                    SurfaceTranslateX = QStandardItem::UserType + 117,
                    SurfaceTranslateY = QStandardItem::UserType + 118,
                    SurfaceTranslateZ = QStandardItem::UserType + 119,
                    NetworkThreshold = QStandardItem::UserType + 120,
                    NetworkMatrix = QStandardItem::UserType + 121,
                    SurfaceTessInner = QStandardItem::UserType + 122,
                    SurfaceTessOuter = QStandardItem::UserType + 123,
                    SurfaceTriangleScale = QStandardItem::UserType + 124,
                    NumberDipoles = QStandardItem::UserType + 125,
                    MaterialType = QStandardItem::UserType + 126,
                    ShowNormals = QStandardItem::UserType + 127  };
}

// Model item roles
namespace Data3DTreeModelItemRoles
{
    enum ItemRole{SurfaceCurrentColorVert = Qt::UserRole + 100,
                    SurfaceVert = Qt::UserRole + 101,
                    SurfaceTris = Qt::UserRole + 102,
                    SurfaceNorm = Qt::UserRole + 103,
                    SurfaceCurv = Qt::UserRole + 104,
                    SurfaceOffset = Qt::UserRole + 105,
                    SurfaceCurvatureColorVert = Qt::UserRole + 107,
                    NetworkDataMatrix = Qt::UserRole + 108,
                    SurfaceAnnotationColorVert = Qt::UserRole + 109,
                    SurfaceSetName = Qt::UserRole + 111,
                    SurfaceHemi = Qt::UserRole + 112,
                    AnnotColors = Qt::UserRole + 113,
                    FileName = Qt::UserRole + 114,
                    FilePath = Qt::UserRole + 115,
                    LabeList = Qt::UserRole + 116,
                    LabeIds = Qt::UserRole + 117,
                    RTData = Qt::UserRole + 118,
                    RTVertNoLeftHemi = Qt::UserRole + 119,
                    RTTimes = Qt::UserRole + 120,
                    RTHemi = Qt::UserRole + 121,
                    RTStartIdxLeftHemi = Qt::UserRole + 122,
                    RTEndIdxLeftHemi = Qt::UserRole + 123,
                    VertexBased = Qt::UserRole + 124,
                    SmoothingBased = Qt::UserRole + 125,
                    AnnotationBased = Qt::UserRole + 126,
                    BemName = Qt::UserRole + 127,
                    RTStartIdxRightHemi = Qt::UserRole + 128,
                    RTEndIdxRightHemi = Qt::UserRole + 129,
                    RTVertNoRightHemi = Qt::UserRole + 130,
                    SourceVertices = Qt::UserRole + 131,
                    NetworkData = Qt::UserRole + 132,
                    ECDSetData = Qt::UserRole + 133};
}

namespace MetaTreeItemRoles
{
    enum ItemRole{SurfaceFileName = Qt::UserRole,
                    SurfaceType = Qt::UserRole + 1,
                    SurfaceColorSulci = Qt::UserRole + 3,
                    SurfaceColorGyri = Qt::UserRole + 4,
                    SurfaceFilePath = Qt::UserRole + 5,
                    AnnotName = Qt::UserRole + 6,
                    AnnotFilePath = Qt::UserRole + 7,
                    AlphaValue = Qt::UserRole + 8,
                    StreamStatus = Qt::UserRole + 9,
                    SourceSpaceType = Qt::UserRole + 10,
                    ColormapType = Qt::UserRole + 11,
                    StreamingTimeInterval = Qt::UserRole + 12,
                    LoopedStreaming = Qt::UserRole + 13,
                    NumberAverages = Qt::UserRole + 14,
                    DistributedSourceLocThreshold = Qt::UserRole + 15,
                    VisualizationType = Qt::UserRole + 16,
                    Color = Qt::UserRole + 17,
                    SurfaceTranslateX = Qt::UserRole + 18,
                    SurfaceTranslateY = Qt::UserRole + 19,
                    SurfaceTranslateZ = Qt::UserRole + 20,
                    PointColor = Qt::UserRole + 21,
                    NetworkThreshold = Qt::UserRole + 22,
                    SurfaceTessInner = Qt::UserRole + 23,
                    SurfaceTessOuter = Qt::UserRole + 24,
                    SurfaceTriangleScale = Qt::UserRole + 25,
                    SurfaceMaterial = Qt::UserRole + 26};
}

} //NAMESPACE DISP3DLIB

// Metatype declaration for correct QVariant usage
// DO NOT FORGET TO REGISTER THESE TYPES IF YOU WANT TO USE THEM IN SIGNAL SLOT/SLOT SYSTEM (SEE VIEW3D initMetatypes())
#ifndef metatype_matrixx3i
#define metatype_matrixx3i
Q_DECLARE_METATYPE(Eigen::MatrixX3i);
#endif

#ifndef metatype_matrixXd
#define metatype_matrixXd
Q_DECLARE_METATYPE(Eigen::MatrixXd);
#endif

#ifndef metatype_matrixx3f
#define metatype_matrixx3f
Q_DECLARE_METATYPE(Eigen::MatrixX3f);
#endif

#ifndef metatype_vectorxf
#define metatype_vectorxf
Q_DECLARE_METATYPE(Eigen::VectorXf);
#endif

#ifndef metatype_vectorxi
#define metatype_vectorxi
Q_DECLARE_METATYPE(Eigen::VectorXi);
#endif

#ifndef metatype_vectorxd
#define metatype_vectorxd
Q_DECLARE_METATYPE(Eigen::VectorXd);
#endif

#ifndef metatype_rowvectorxf
#define metatype_rowvectorxf
Q_DECLARE_METATYPE(Eigen::RowVectorXf);
#endif

#ifndef metatype_vector3f
#define metatype_vector3f
Q_DECLARE_METATYPE(Eigen::Vector3f);
#endif

#endif // TYPES_H
