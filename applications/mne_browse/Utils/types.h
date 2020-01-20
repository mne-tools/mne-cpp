//=============================================================================================================
/**
 * @file     types.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  1.0
 * @date     January, 2014
 *
 * @section  LICENSE
 *
 * Copyright (C) 2014, Christoph Dinh, Lorenz Esch. All rights reserved.
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

#include <Eigen/Core>
#include <Eigen/SparseCore>
#include <fiff/fiff.h>
#include "filteroperator.h"


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QPair>
#include <QList>
#include <QSharedPointer>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE MNEBROWSE
//=============================================================================================================

namespace MNEBROWSE
{

typedef Matrix<double,Dynamic,Dynamic,RowMajor> MatrixXdR;
typedef QPair<const double*,qint32> RowVectorPair;
typedef QPair<const float*,qint32> RowVectorPairF;
typedef QPair<int,int> QPairInts;

namespace RawModelRoles
{
    enum ItemRole{GetChannelMean = Qt::UserRole + 1000};
}

namespace AverageModelRoles
{
    enum ItemRole{GetAverageData = Qt::UserRole + 1001,
                  GetFiffInfo = Qt::UserRole + 1002,
                  GetAspectKind = Qt::UserRole + 1003,
                  GetFirstSample = Qt::UserRole + 1004,
                  GetLastSample = Qt::UserRole + 1005,
                  GetComment = Qt::UserRole + 1006,
                  GetTimeData = Qt::UserRole + 1007,
                  GetProjections = Qt::UserRole + 1008};
}

namespace ChannelInfoModelRoles
{
    enum ItemRole{GetOrigChName = Qt::UserRole + 1009,
                  GetMappedLayoutChName = Qt::UserRole + 1010,
                  GetChNumber = Qt::UserRole + 1011,
                  GetChKind = Qt::UserRole + 1012,
                  GetMEGType = Qt::UserRole + 1013,
                  GetChUnit = Qt::UserRole + 1014,
                  GetChAlias = Qt::UserRole + 1015,
                  GetChPosition = Qt::UserRole + 1016,
                  GetChDigitizer = Qt::UserRole + 1017,
                  GetChActiveFilter = Qt::UserRole + 1018,
                  GetChCoilType = Qt::UserRole + 1019};
}

namespace ProjectionModelRoles
{
    enum ItemRole{GetProjectionData = Qt::UserRole + 1019,
                  GetProjectionName = Qt::UserRole + 1020,
                  GetProjectionState = Qt::UserRole + 1021,
                  GetProjectionDimension = Qt::UserRole + 1022};
}

} //NAMESPACE

Q_DECLARE_METATYPE(FIFFLIB::fiff_int_t);
Q_DECLARE_METATYPE(MNEBROWSE::RowVectorPairF);
Q_DECLARE_METATYPE(const FIFFLIB::FiffInfo*);
Q_DECLARE_METATYPE(MNEBROWSE::MatrixXdR);
Q_DECLARE_METATYPE(MNEBROWSE::RowVectorPair);
Q_DECLARE_METATYPE(QList<MNEBROWSE::RowVectorPair>);
Q_DECLARE_METATYPE(QSharedPointer<DISPLIB::MNEOperator>);

#endif // TYPES_H
