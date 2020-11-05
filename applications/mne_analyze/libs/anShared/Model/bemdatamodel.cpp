//=============================================================================================================
/**
 * @file     bemdatamodel.cpp
 * @author   Ruben Dörfel <doerfelruben@aol.com>
 * @since    0.1.6
 * @date     August, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020, Ruben Dörfel. All rights reserved.
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
 * @brief    BemDataModel class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "bemdatamodel.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace ANSHAREDLIB;
using namespace MNELIB;

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

BemDataModel::BemDataModel(QObject *pParent)
: AbstractModel(pParent)
{
    qInfo() << "[BemDataModel::BemDataModel] Default constructor called !";
}

//=============================================================================================================

BemDataModel::BemDataModel(const QString &sFilePath,
                           const QByteArray& byteLoadedData,
                           QObject *pParent)
: AbstractModel(sFilePath, pParent)
, m_pBem(MNEBem::SPtr::create())
{
    if(byteLoadedData.isEmpty()) {
        m_file.setFileName(sFilePath);
        initBemData(m_file);
    } else {
        m_byteLoadedData = byteLoadedData;
        m_buffer.setData(m_byteLoadedData);
        initBemData(m_buffer);
    }
}

//=============================================================================================================

BemDataModel::~BemDataModel()
{

}

//=============================================================================================================

void BemDataModel::initBemData(QIODevice& qIODevice)
{
    // build Bem
    m_pBem = MNEBem::SPtr::create(qIODevice);

    if(m_pBem->isEmpty()) {
        qWarning() << "[BemDataModel::initBemData] File does not contain any Bem data";
        return;
    }

    emit newBemAvailable(m_pBem);

    // need to close the file manually
    qIODevice.close();
    m_bIsInit = true;
}

//=============================================================================================================

QVariant BemDataModel::data(const QModelIndex &index,
                            int role) const
{
    return QVariant();
}

//=============================================================================================================

Qt::ItemFlags BemDataModel::flags(const QModelIndex &index) const
{
    return QAbstractItemModel::flags(index);
}

//=============================================================================================================

QModelIndex BemDataModel::index(int row,
                                int column,
                                const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return createIndex(row, column);
}

//=============================================================================================================

QModelIndex BemDataModel::parent(const QModelIndex &index) const
{
    Q_UNUSED(index);
    return QModelIndex();
}

//=============================================================================================================

int BemDataModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 0;
}

//=============================================================================================================

int BemDataModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 0;
}
