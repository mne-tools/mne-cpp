//=============================================================================================================
/**
 * @file     dipolefitmodel.cpp
 * @author   Gabriel Motta <gbmotta@mgh.harvard.edu>
 * @since    0.1.7
 * @date     November, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020, Gabriel Motta. All rights reserved.
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
 * @brief    Definition of the DipoleFitModel Class.
 *
 */


//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "dipolefitmodel.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================


//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace ANSHAREDLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

DipoleFitModel::DipoleFitModel(INVERSELIB::ECDSet ECDSet,
                               const QString &sFilePath,
                               const QByteArray& byteLoadedData,
                               QObject* parent)
:AbstractModel(parent)
{
    Q_UNUSED(byteLoadedData);
    Q_UNUSED(sFilePath);

    m_ECD_Set = ECDSet;
}

//=============================================================================================================

DipoleFitModel::DipoleFitModel(const QString &sFilePath,
                               const QByteArray& byteLoadedData,
                               QObject* parent)
:AbstractModel(sFilePath, parent)
{
    Q_UNUSED(byteLoadedData);

}

//=============================================================================================================

int DipoleFitModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    return 1;
}

//=============================================================================================================

int DipoleFitModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    return 1;
}

//=============================================================================================================

QVariant DipoleFitModel::data(const QModelIndex &index,
                             int role) const
{
    Q_UNUSED(index);
    Q_UNUSED(role);

    return QVariant::fromValue(m_ECD_Set);
}

//=============================================================================================================

Qt::ItemFlags DipoleFitModel::flags(const QModelIndex &index) const
{
    return QAbstractItemModel::flags(index);
}
