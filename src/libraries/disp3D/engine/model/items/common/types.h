//=============================================================================================================
/**
 * @file     types.h
 * @author   Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Juan Garcia-Prieto <juangpc@gmail.com>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     December, 2015
 *
 * @section  LICENSE
 *
 * Copyright (C) 2015, Lars Debor, Juan Garcia-Prieto, Lorenz Esch. All rights reserved.
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

#ifndef DISP3DLIB_TYPES_H
#define DISP3DLIB_TYPES_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fs/label.h>
#include <inverse/dipoleFit/ecd_set.h>
#include <fiff/fiff_info.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QStandardItem>
#include <QByteArray>
#include <QSharedPointer>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/Sparse>

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
                    MNEDataItem = QStandardItem::UserType + 5,
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
                    AbstractMeshItem = QStandardItem::UserType + 18,
                    SensorDataItem = QStandardItem::UserType + 19,
                    GpuInterpolationItem = QStandardItem::UserType + 21};
}

namespace MetaTreeItemTypes
{
    enum ItemType{FileName = QStandardItem::UserType + 100,
                    FilePath = QStandardItem::UserType + 101,
                    SurfaceColorGyri = QStandardItem::UserType + 102,
                    SurfaceColorSulci = QStandardItem::UserType + 103,
                    SurfaceColorVert = QStandardItem::UserType + 104,
                    AlphaValue = QStandardItem::UserType + 105,
                    StreamStatus = QStandardItem::UserType + 106,
                    SourceSpaceType = QStandardItem::UserType + 107,
                    ColormapType = QStandardItem::UserType + 108,
                    StreamingTimeInterval = QStandardItem::UserType + 109,
                    LoopedStreaming = QStandardItem::UserType + 110,
                    NumberAverages = QStandardItem::UserType + 111,
                    DataThreshold = QStandardItem::UserType + 112,
                    VisualizationType = QStandardItem::UserType + 113,
                    Color = QStandardItem::UserType + 114,
                    UnknownItem = QStandardItem::UserType + 115,
                    TranslateX = QStandardItem::UserType + 116,
                    TranslateY = QStandardItem::UserType + 117,
                    TranslateZ = QStandardItem::UserType + 118,
                    InterpolationFunction = QStandardItem::UserType + 119,
                    NetworkMatrix = QStandardItem::UserType + 120,
                    SurfaceTessInner = QStandardItem::UserType + 121,
                    SurfaceTessOuter = QStandardItem::UserType + 122,
                    SurfaceTriangleScale = QStandardItem::UserType + 123,
                    NumberDipoles = QStandardItem::UserType + 124,
                    MaterialType = QStandardItem::UserType + 125,
                    ShowNormals = QStandardItem::UserType + 126,
                    Scale = QStandardItem::UserType + 127,
                    CancelDistance = QStandardItem::UserType + 128};
}

// Model item roles
namespace MetaTreeItemRoles
{
    enum ItemRole{SurfaceFileName = Qt::UserRole,
                    SurfaceType = Qt::UserRole + 1,
                    SurfaceColorSulci = Qt::UserRole + 2,
                    SurfaceColorGyri = Qt::UserRole + 3,
                    SurfaceFilePath = Qt::UserRole + 4,
                    AnnotName = Qt::UserRole + 5,
                    AnnotFilePath = Qt::UserRole + 6,
                    AlphaValue = Qt::UserRole + 7,
                    StreamStatus = Qt::UserRole + 8,
                    SourceSpaceType = Qt::UserRole + 9,
                    ColormapType = Qt::UserRole + 10,
                    StreamingTimeInterval = Qt::UserRole + 11,
                    LoopedStreaming = Qt::UserRole + 12,
                    NumberAverages = Qt::UserRole + 13,
                    DataThreshold = Qt::UserRole + 14,
                    VisualizationType = Qt::UserRole + 15,
                    Color = Qt::UserRole + 16,
                    TranslateX = Qt::UserRole + 17,
                    TranslateY = Qt::UserRole + 18,
                    TranslateZ = Qt::UserRole + 19,
                    InterpolationFunction = Qt::UserRole + 20,
                    SurfaceTessInner = Qt::UserRole + 21,
                    SurfaceTessOuter = Qt::UserRole + 22,
                    SurfaceTriangleScale = Qt::UserRole + 23,
                    SurfaceMaterial = Qt::UserRole + 24,
                    Scale = Qt::UserRole + 25,
                    CancelDistance = Qt::UserRole + 26};
}

namespace Data3DTreeModelItemRoles
{
    enum ItemRole{SurfaceCurrentColorVert = Qt::UserRole + 100,
                    NumberVertices = Qt::UserRole + 101,
                    SurfaceCurv = Qt::UserRole + 102,
                    NetworkData = Qt::UserRole + 103,
                    SurfaceHemi = Qt::UserRole + 104,
                    AnnotColors = Qt::UserRole + 105,
                    FileName = Qt::UserRole + 106,
                    FilePath = Qt::UserRole + 107,
                    Data = Qt::UserRole + 108,
                    VertexBased = Qt::UserRole + 109,
                    InterpolationBased = Qt::UserRole + 110,
                    AnnotationBased = Qt::UserRole + 111};
}
} //NAMESPACE DISP3DLIB

// Metatype declaration for correct QVariant usage
// DO NOT FORGET TO REGISTER THESE TYPES IF YOU WANT TO USE THEM IN SIGNAL SLOT/SLOT SYSTEM (SEE Data3DTreeModel initMetatypes())
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

#ifndef metatype_matrixx4f
#define metatype_matrixx4f
Q_DECLARE_METATYPE(Eigen::MatrixX4f);
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

#ifndef metatype_fiffinfo
#define metatype_fiffinfo
Q_DECLARE_METATYPE(FIFFLIB::FiffInfo);
#endif

#ifndef metatype_qvectorvector3f
#define metatype_qvectorvector3f
Q_DECLARE_METATYPE(QVector<Eigen::Vector3f>);
#endif

#ifndef metatype_qvectorvectorint
#define metatype_qvectorvectorint
Q_DECLARE_METATYPE(QVector<QVector<int> >);
#endif

#ifndef metatype_qvectorint
#define metatype_qvectorint
Q_DECLARE_METATYPE(QVector<int>);
#endif

#ifndef metatype_sparsematf
#define metatype_sparsematf
Q_DECLARE_METATYPE(Eigen::SparseMatrix<float>);
#endif

#ifndef metatype_sharedptrsparsematf
#define metatype_sharedptrsparsematf
Q_DECLARE_METATYPE(QSharedPointer<Eigen::SparseMatrix<float> >);
#endif

#ifndef metatype_listlabel
#define metatype_listlabel
Q_DECLARE_METATYPE(QList<FSLIB::Label>);
#endif

#endif // DISP3DLIB_TYPES_H
