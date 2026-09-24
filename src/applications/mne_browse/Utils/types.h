//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2014-2026 MNE-CPP Authors
 *
 * @file     types.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @date     January, 2014
 * @version  2.1.0
 * @brief    Contains general application specific types
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

struct WhiteningSettings
{
    double regMag = 0.1;
    double regGrad = 0.1;
    double regEeg = 0.1;
    bool useProj = true;
    bool enableButterfly = false;
    bool enableLayout = false;
    bool enableRaw = false;
};

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
                  GetProjections = Qt::UserRole + 1008,
                  GetNumAverages = Qt::UserRole + 1009,
                  GetBaselineText = Qt::UserRole + 1010};
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
Q_DECLARE_METATYPE(MNEBROWSE::WhiteningSettings);

#endif // TYPES_H
