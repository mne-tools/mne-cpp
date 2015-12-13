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


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE DISP3DNEWLIB
//=============================================================================================================

namespace DISP3DNEWLIB
{

namespace BrainTreeItemTypes
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
                    SurfaceFileType = QStandardItem::UserType + 9,
                    SurfaceColorGyri = QStandardItem::UserType + 10,
                    SurfaceColorSulci = QStandardItem::UserType + 11,
                    SurfaceColorVert = QStandardItem::UserType + 12};
}

namespace BrainTreeModelRoles
{
    enum ItemRole{SurfaceName = Qt::UserRole,
                      SurfaceType = Qt::UserRole + 1,
                      SurfaceHemi = Qt::UserRole + 2,
                      SurfaceColorSulci = Qt::UserRole + 3,
                      SurfaceColorGyri = Qt::UserRole + 4,
                      SurfaceColorVert = Qt::UserRole + 5,
                      SurfaceVert = Qt::UserRole + 6,
                      SurfaceTris = Qt::UserRole + 7,
                      SurfaceNorm = Qt::UserRole + 8,
                      SurfaceCurv = Qt::UserRole + 9,
                      SurfaceOffset = Qt::UserRole + 10,
                      SurfaceFilePath = Qt::UserRole + 11,
                      AnnotName = Qt::UserRole + 12,
                      AnnotFilePath = Qt::UserRole + 13,
                      AnnotColor = Qt::UserRole + 14,
                      Renderable3DEntity = Qt::UserRole + 15,
                      RootItem = Qt::UserRole + 16,
                      SurfaceSetName = Qt::UserRole + 17};
}

} //NAMESPACE DISP3DNEWLIB

#ifndef metatype_renderable3Dentity
#define metatype_renderable3Dentity
Q_DECLARE_METATYPE(DISP3DNEWLIB::Renderable3DEntity*)
#endif

//Q_DECLARE_METATYPE(FIFFLIB::fiff_int_t);
//Q_DECLARE_METATYPE(MNEBrowseRawQt::RowVectorPairF);
//Q_DECLARE_METATYPE(const FIFFLIB::FiffInfo*);
//Q_DECLARE_METATYPE(MNEBrowseRawQt::MatrixXdR);
//Q_DECLARE_METATYPE(MNEBrowseRawQt::RowVectorPair);
//Q_DECLARE_METATYPE(QList<MNEBrowseRawQt::RowVectorPair>);
//Q_DECLARE_METATYPE(QSharedPointer<DISPLIB::MNEOperator>);

#endif // TYPES_H
