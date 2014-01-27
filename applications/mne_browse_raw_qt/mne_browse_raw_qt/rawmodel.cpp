//=============================================================================================================
/**
* @file     rawmodel.cpp
* @author   Florian Schlembach <florian.schlembach@tu-ilmenau.de>;
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
*           Jens Haueisen <jens.haueisen@tu-ilmenau.de>
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

RawModel::RawModel(QObject *parent)
: QAbstractTableModel(parent)
, m_iWindowSize(6000) //samples
, n_reloadPos(2000) //samples
, m_reloaded(false)
{
}

//*************************************************************************************************************

RawModel::RawModel(QIODevice &p_IODevice, QObject *parent)
: QAbstractTableModel(parent)
, m_iWindowSize(6000) //samples
, n_reloadPos(2000) //samples
, m_reloaded(false)
{
    //read fiff data
    loadFiffData(p_IODevice);
}

//=============================================================================================================
//virtual functions

int RawModel::rowCount(const QModelIndex & /*parent*/) const
{
    if(!m_chinfolist.empty())
        return m_chinfolist.size();
    else return 0;
}

int RawModel::columnCount(const QModelIndex & /*parent*/) const
{
    return 2;
}

//*************************************************************************************************************

QVariant RawModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::DisplayRole && index.isValid()) {
        if(index.column()==0)
            return QVariant(m_chinfolist[index.row()].ch_name);
        if(index.column()==1) {
            QVariant v;

            //form RowVectorPair of pointer and length of RowVector
            QPair<const double*,qint32> rowVectorPair;

            //pack all adjacent (after reload) RowVectorPairs into a QList
            QList<RowVectorPair> listRowVectorPair;

            for(qint16 i=0; i < m_data.size(); ++i) {
                rowVectorPair.first = m_data[i].data() + index.row()*m_data[i].cols();
                rowVectorPair.second = m_data[i].cols();

                listRowVectorPair.append(rowVectorPair);
            }

            v.setValue(listRowVectorPair);
            return v;
        }
    }

    return QVariant();
}

//*************************************************************************************************************

QVariant RawModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(role != Qt::DisplayRole && role != Qt::TextAlignmentRole)
        return QVariant();

    if(orientation == Qt::Horizontal) {
        switch(section) {
        case 0: //chname column
            return QVariant();
        case 1: //data plot column
            return QVariant("data plot");
            switch(role) {
            case Qt::DisplayRole:
                return QVariant("data plot");
            case Qt::TextAlignmentRole:
                return QVariant(Qt::AlignLeft);
            }
        }
    }
    else if(orientation == Qt::Vertical) {
        QModelIndex chname = createIndex(section,0);
        switch(role) {
        case Qt::DisplayRole:
            return QVariant(data(chname).toString());
//        case Qt::TextAlignmentRole:
//            return QVariant();
        }
    }

    return QVariant();
}


//=============================================================================================================
//non-virtual functions

bool RawModel::loadFiffData(QIODevice& p_IODevice)
{
    beginResetModel();
    clearModel();

    MatrixXd t_data,t_times;

    m_pfiffIO = QSharedPointer<FiffIO>(new FiffIO(p_IODevice));
    if(!m_pfiffIO->m_qlistRaw.empty()) {
        m_iFiffCursor = m_pfiffIO->m_qlistRaw[0]->first_samp+12000; //Set cursor somewhere into fiff file [in samples]
        if(!m_pfiffIO->m_qlistRaw[0]->read_raw_segment(t_data, t_times, m_iFiffCursor, m_iFiffCursor+m_iWindowSize))
            return false;
    }
    else {
        qDebug("RawModel: ERROR! Data set does not contain any fiff data!");
        endResetModel();
        return false;
    }

    //set loaded fiff data
    m_data.append(t_data);
    m_times.append(t_times);
    m_iCurBlockPosition = 0; //set BlockPosition in m_data QList to first block

    loadChInfos();

    endResetModel();
    return true;
}

//*************************************************************************************************************

void RawModel::clearModel() {
    m_pfiffIO.clear();
    m_data.clear();
    m_times.clear();
    m_iCurBlockPosition = 0;
    m_iFiffCursor = 0;
    m_chinfolist.clear();


    qDebug("RawModel cleared.");
}

//*************************************************************************************************************

void RawModel::reloadFiffData(bool before) {
    MatrixXd t_reloaddata,t_reloadtimes;

    //update scroll position
    if(before) {
        m_iCurBlockPosition += 1;
        m_iFiffCursor -= m_iWindowSize;
    }
    else {
//        m_iCurBlockPosition += 1;
        m_iFiffCursor += m_iWindowSize;
    }

    if(!m_pfiffIO->m_qlistRaw[0]->read_raw_segment(t_reloaddata, t_reloadtimes, m_iFiffCursor-m_iWindowSize, m_iFiffCursor))
        printf("Error when reading raw data!");

    //extend m_data with reloaded data
    m_data.prepend(t_reloaddata);
    m_times.prepend(t_reloadtimes);

    qDebug("Fiff data REloaded.");

    QModelIndex topLeft = createIndex(0,1);
    QModelIndex bottomRight = createIndex(m_data[m_iCurBlockPosition].rows(),1);

    emit dataChanged(topLeft,bottomRight);
    emit layoutChanged();
}

//*************************************************************************************************************

void RawModel::loadChInfos()
{
    for(qint32 i=0; i < m_pfiffIO->m_qlistRaw[0]->info.nchan; ++i)
        m_chinfolist.append(m_pfiffIO->m_qlistRaw[0]->info.chs[i]);
}

//*************************************************************************************************************

double RawModel::maxDataValue(qint16 chan) const {
    double dMax;

    double max = m_data[0].row(0).maxCoeff();
    double min = m_data[0].row(0).minCoeff();

    dMax = (double) (m_chinfolist[chan].range*m_chinfolist[chan].cal)/2;

    return dMax;
}

//=============================================================================================================
//slots

void RawModel::reloadData(int value) {
    m_iCurScrollPos = m_iFiffCursor+value;
    qDebug() << "Current ScrollBar value" << value << "Current Fiff Scroll Position value " << m_iCurScrollPos;

    if(value < n_reloadPos && !m_reloaded) {
//        qDebug() << "Current ScrollBar value" << value << "(reload data at FRONT)";
        reloadFiffData(true);
        m_reloaded = true;
    }
    else if(value > m_iWindowSize-n_reloadPos) {
//        qDebug() << "Current ScrollBar value" << value << "(reload data at END), absolute Fiff file position:" << m_iCurScrollPos;
    }
}
