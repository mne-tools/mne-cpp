//=============================================================================================================
/**
 * @file     averagingdatamodel.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.6
 * @date     September, 2020
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
 * @brief    Definition of the AveragingDataModel Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "averagingdatamodel.h"

#include <fiff/fiff_evoked_set.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QFile>

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

AveragingDataModel::AveragingDataModel(QSharedPointer<FIFFLIB::FiffEvokedSet> pEvokedSet,
                                       QObject* parent)
: AbstractModel(parent)
, m_pFiffEvokedSet(pEvokedSet)
, m_bFromFile(false)
{

}

//=============================================================================================================

AveragingDataModel::AveragingDataModel(const QString &sFilePath,
                                       const QByteArray& byteLoadedData,
                                       QObject* parent)
: AbstractModel(sFilePath, parent)
, m_bFromFile(true)
{
    Q_UNUSED(byteLoadedData);

    QFile file(sFilePath);
    m_pFiffEvokedSet = QSharedPointer<FIFFLIB::FiffEvokedSet>::create(file);
}

//=============================================================================================================

AveragingDataModel::~AveragingDataModel()
{

}

//=============================================================================================================

QVariant AveragingDataModel::data(const QModelIndex &index,
                                  int role) const
{
    Q_UNUSED(index);
    Q_UNUSED(role);

    return QVariant::fromValue(m_pFiffEvokedSet);
}

//=============================================================================================================

Qt::ItemFlags AveragingDataModel::flags(const QModelIndex &index) const
{
    return QAbstractItemModel::flags(index);
}

//=============================================================================================================

int AveragingDataModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 1;
}

//=============================================================================================================

int AveragingDataModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 1;
}

//=============================================================================================================

void AveragingDataModel::setEvokedSet(QSharedPointer<FIFFLIB::FiffEvokedSet> pEvokedSet)
{
    m_pFiffEvokedSet = pEvokedSet;
}

//=============================================================================================================

bool AveragingDataModel::isFromFile()
{
    return m_bFromFile;
}

//=============================================================================================================

QSharedPointer<FIFFLIB::FiffInfo> AveragingDataModel::getFiffInfo()
{
    QSharedPointer<FIFFLIB::FiffInfo> pInfo = QSharedPointer<FIFFLIB::FiffInfo>(new FIFFLIB::FiffInfo(m_pFiffEvokedSet->info));
    return pInfo;
}

//=============================================================================================================

QSharedPointer<FIFFLIB::FiffEvokedSet> AveragingDataModel::getEvokedSet()
{
    return m_pFiffEvokedSet;
}
