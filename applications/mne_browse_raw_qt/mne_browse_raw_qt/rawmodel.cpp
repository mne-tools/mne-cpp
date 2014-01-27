//=============================================================================================================
/**
* @file     rawmodel.cpp
* @author   Florian Schlembach <florian.schlembach@tu-ilmenau.de>;
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     January, 2014
*
* @section  LICENSE
*
* Copyright (C) 2014, Christoph Dinh and Matti Hamalainen. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that
* the following conditions are met:
*     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
*       following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
*       the following disclaimer in the documentation and/or other materials provided with the distribution.
*     * Neither the name of the Massachusetts General Hospital nor the names of its contributors may be used
*       to endorse or promote products derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MASSACHUSETTS GENERAL HOSPITAL BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*
* @brief    Implementation of data model of mne_browse_raw_qt
*
*/

#include "rawmodel.h"

//*************************************************************************************************************

RawModel::RawModel(QObject *parent) :
    QAbstractTableModel(parent)
{
    qRegisterMetaType<MatrixXd>("MatrixXd");
}

//*************************************************************************************************************

RawModel::RawModel(QObject *parent, QIODevice& p_IODevice) :
    QAbstractTableModel(parent)
{
    qRegisterMetaType<MatrixXd>("MatrixXd");

    //read fiff data
    m_pfiffIO = QSharedPointer<FiffIO>(new FiffIO(p_IODevice));

    m_windowlength = 2; //just for now
    m_position = 100;

    if(!m_pfiffIO->m_qlistRaw[0]->read_raw_segment_times(m_data, m_times, m_position, m_position+m_windowlength)) {
        printf("Error when reading raw data!");
    }

    if(!m_pfiffIO->m_qlistRaw.empty()) {
        RawModel::loadChNames();
        RawModel::loadChInfos();
    }
    else
        printf("ERROR! Data set does not contain any fiff data!");

}

//*************************************************************************************************************

int RawModel::rowCount(const QModelIndex & /*parent*/) const
{
    return m_data.rows();
}

//*************************************************************************************************************

int RawModel::columnCount(const QModelIndex & /*parent*/) const
{
    return 1; //2
}

//*************************************************************************************************************

QVariant RawModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::DisplayRole && index.isValid()) {
        if(index.column()==0)
            return QVariant(m_chnames[index.row()]);
        if(index.column()==1) {
            QVariant v;
            v.setValue((MatrixXd)m_data.row(index.row()));
            return v;
        }
    }

    return QVariant();
}

Qt::ItemFlags RawModel::flags(const QModelIndex & index) const
{
    return Qt::NoItemFlags;
}

QVariant RawModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(role != Qt::DisplayRole)
        return QVariant();

    if(orientation == Qt::Horizontal) {
        switch(section) {
        case 0: return QVariant("chname");
        case 1: return QVariant("data plot");
        }
    }
    else if(orientation == Qt::Vertical) {
        return QVariant(section);
    }

    return QVariant();
}

//*************************************************************************************************************



//*************************************************************************************************************

void RawModel::loadChNames() {
   for(qint32 i=0; i < m_pfiffIO->m_qlistRaw[0]->info.nchan; ++i)
            m_chnames.append(m_pfiffIO->m_qlistRaw[0]->info.chs[i].ch_name);
}

//*************************************************************************************************************

void RawModel::loadChInfos() {
    for(qint32 i=0; i < m_pfiffIO->m_qlistRaw[0]->info.nchan; ++i)
        m_chinfolist.append(m_pfiffIO->m_qlistRaw[0]->info.chs[i]);
}
