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

namespace BrainTreeItemModelRoles
{
    enum ItemRole{SurfName = Qt::UserRole + 1000,
                  SurfType = Qt::UserRole + 1001,
                  SurfHemi = Qt::UserRole + 1002,
                  SurfColorSulci = Qt::UserRole + 1003,
                  SurfColorGyri = Qt::UserRole + 1004,
                  SurfColorVert = Qt::UserRole + 1005,
                  SurfVert = Qt::UserRole + 1006,
                  SurfTris = Qt::UserRole + 1007,
                  SurfNorm = Qt::UserRole + 1008,
                  SurfCurv = Qt::UserRole + 1009,
                  SurfOffset = Qt::UserRole + 1010,
                  SurfFilePath = Qt::UserRole + 1011,
                  SurfAnnotName = Qt::UserRole + 1012,
                  SurfAnnotFilePath = Qt::UserRole + 1013,
                  SurfColorAnnot = Qt::UserRole + 1014,
                  RootItem = Qt::UserRole + 1015};
}

namespace BrainSurfaceTreeItemModelRoles
{
    enum ItemRole{SurfName = Qt::UserRole + 1100};
}

namespace BrainTreeSurfaceSetModelRoles
{
    enum ItemRole{SurfSetName = Qt::UserRole + 1200};
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
