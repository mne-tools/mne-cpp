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


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QStandardItem>


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

enum BrainTreeItemTypes {
    SurfaceItem = QStandardItem::UserType,
    SurfaceFileName = QStandardItem::UserType + 1,
    SurfaceFilePath = QStandardItem::UserType + 2,
    AnnotFileName = QStandardItem::UserType + 3,
    AnnotFilePath = QStandardItem::UserType + 4,
};

namespace BrainTreeModelRoles
{
    enum ItemRole{GetSurfName = Qt::UserRole,
                  GetSurfType = Qt::UserRole + 1,
                  GetSurfHemi = Qt::UserRole + 2,
                  GetSurfColorSulci = Qt::UserRole + 3,
                  GetSurfColorGyri = Qt::UserRole + 4,
                  GetSurfColorVert = Qt::UserRole + 5,
                  GetSurfVert = Qt::UserRole + 6,
                  GetSurfTris = Qt::UserRole + 7,
                  GetSurfNorm = Qt::UserRole + 8,
                  GetSurfCurv = Qt::UserRole + 9,
                  GetSurfOffset = Qt::UserRole + 10,
                  GetSurfFilePath = Qt::UserRole + 11,
                  GetAnnotName = Qt::UserRole + 12,
                  GetAnnotFilePath = Qt::UserRole + 13,
                  GetAnnotColor = Qt::UserRole + 14,
                  GetRenderable3DEntity = Qt::UserRole + 15,
                  GetRootItem = Qt::UserRole + 16};
}



} //NAMESPACE

//Q_DECLARE_METATYPE(FIFFLIB::fiff_int_t);
//Q_DECLARE_METATYPE(MNEBrowseRawQt::RowVectorPairF);
//Q_DECLARE_METATYPE(const FIFFLIB::FiffInfo*);
//Q_DECLARE_METATYPE(MNEBrowseRawQt::MatrixXdR);
//Q_DECLARE_METATYPE(MNEBrowseRawQt::RowVectorPair);
//Q_DECLARE_METATYPE(QList<MNEBrowseRawQt::RowVectorPair>);
//Q_DECLARE_METATYPE(QSharedPointer<DISPLIB::MNEOperator>);

#endif // TYPES_H
