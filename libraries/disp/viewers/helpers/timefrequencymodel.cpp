//=============================================================================================================
/**
 * @file     timefrequencymodel.cpp
 * @author   Gabriel Motta <gbmotta@mgh.harvard.edu>
 * @since    0.1.9
 * @date     April, 2021
 *
 * @section  LICENSE
 *
 * Copyright (C) 2021, Gabriel Motta. All rights reserved.
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
 * @brief    Definition of the TimeFrequencyModel Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "timefrequencymodel.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================


TimeFrequencyModel::TimeFrequencyModel()
{

}

//=============================================================================================================

TimeFrequencyModel::TimeFrequencyModel(std::vector<Eigen::MatrixXd>& spectr)
: m_vSpectr(std::move(spectr))
{

}

//=============================================================================================================

void TimeFrequencyModel::setSpectr(std::vector<Eigen::MatrixXd>& spectr)
{
    m_vSpectr.clear();
    m_vSpectr = std::move(spectr);
}

//=============================================================================================================

int TimeFrequencyModel::rowCount(const QModelIndex & /*parent*/) const
{
    return m_vSpectr.size();
}

//=============================================================================================================

int TimeFrequencyModel::columnCount(const QModelIndex & /*parent*/) const
{
    return 3;
}

//=============================================================================================================

QVariant TimeFrequencyModel::data(const QModelIndex &index,
                                  int role) const
{
    if(role != Qt::DisplayRole && role != Qt::BackgroundRole) {
        return QVariant();
    }

    int row = index.row();

    if(!index.isValid() && row >= m_vSpectr.size()) {
        return QVariant();
    }

    if(index.column() == 0 && role == Qt::DisplayRole) {
        //            return QVariant(m_pEvokedSet->info.ch_names);

    }

    if(index.column() == 1) { //timefrequencyview
        switch(role){
        case Qt::BackgroundRole:{
            return QVariant();
        }
        case Qt::DisplayRole:{
            QVariant variant;
            variant.setValue(m_vSpectr[0]);
            return variant;
        }
        }
    }
    if(index.column() == 2) { //timefrequencyview
        switch(role){
        case Qt::BackgroundRole:{
            return QVariant();
        }
        case Qt::DisplayRole:{
            QVariant variant;
            variant.setValue(m_vSpectr[row]);
            return variant;
        }
        }
    }

}

//=============================================================================================================

QVariant TimeFrequencyModel::data(int row, int column, int role) const
{
    return data(index(row, column), role);
}

//=============================================================================================================

QVariant TimeFrequencyModel::headerData(int section,
                                        Qt::Orientation orientation,
                                        int role) const
{
    return QVariant();
}

//=============================================================================================================

void TimeFrequencyModel::setFiffInfo(const FIFFLIB::FiffInfo &info)
{
    m_Info = info;
}

//=============================================================================================================

float TimeFrequencyModel::getSamplingFrequency()
{
    return m_Info.sfreq;
}
