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

RawModel::RawModel(QObject *parent)
: QAbstractTableModel(parent)
, m_dWindowLength(10) //secs
, n_reloadPos(2000) //samples
, m_reloaded(false)
{
//    qRegisterMetaType<MatrixXd>("MatrixXd");
}

//*************************************************************************************************************

RawModel::RawModel(QIODevice &p_IODevice, QObject *parent)
: QAbstractTableModel(parent)
, m_dWindowLength(10) //secs
, n_reloadPos(2000) //samples
, m_reloaded(false)
{
//    qRegisterMetaType<MatrixXd>("MatrixXd");

    //read fiff data
    loadFiffData(p_IODevice);
}

//*************************************************************************************************************
//virtual functions

int RawModel::rowCount(const QModelIndex & /*parent*/) const
{
    return m_data[m_iBlockPosition].rows();
}

int RawModel::columnCount(const QModelIndex & /*parent*/) const
{
    return 2;
}

QVariant RawModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::DisplayRole && index.isValid()) {
        if(index.column()==0)
            return QVariant(m_chnames[index.row()]);
        if(index.column()==1) {
            QVariant v;

            MatrixXd mat = m_data[m_iBlockPosition];
            MapRowVectorXd mapVector(mat.row(index.row()).data(),mat.rows());

            v.setValue((MapRowVectorXd) mapVector);
//            v.setValue((QList<Matrix<double,Dynamic,Dynamic,RowMajor> >) m_data);
            return v;
        }
    }

    return QVariant();
}

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


//*************************************************************************************************************
//non-virtual functions

void RawModel::loadFiffData(QIODevice& p_IODevice)
{
    beginResetModel();

    MatrixXd t_data,t_times;

    m_pfiffIO = QSharedPointer<FiffIO>(new FiffIO(p_IODevice));
    m_dFiffPosition = m_pfiffIO->m_qlistRaw[0]->first_samp/m_pfiffIO->m_qlistRaw[0]->info.sfreq+40; //take beginning of raw data as position + 40 secs [in secs]

    if(!m_pfiffIO->m_qlistRaw[0]->read_raw_segment_times(t_data, t_times, m_dFiffPosition-m_dWindowLength/2, m_dFiffPosition+m_dWindowLength/2))
        qFatal("Error when reading raw data!");

    //set loaded fiff data
    m_data.append(t_data);
    m_times.append(t_times);
    m_iBlockPosition = 0; //set BlockPosition in m_data QList to first block

    if(!m_pfiffIO->m_qlistRaw.empty()) {
        RawModel::loadChNames();
        RawModel::loadChInfos();

        qDebug("Fiff data loaded.");
    }
    else
        qFatal("ERROR! Data set does not contain any fiff data!");

    endResetModel();
}

void RawModel::reloadFiffData(bool before) {
    MatrixXd t_reloaddata,t_reloadtimes;

    if(before)
        if(!m_pfiffIO->m_qlistRaw[0]->read_raw_segment_times(t_reloaddata, t_reloadtimes, m_dFiffPosition-3/2*m_dWindowLength, m_dFiffPosition-m_dWindowLength/2))
            printf("Error when reading raw data!");
    else
        if(!m_pfiffIO->m_qlistRaw[0]->read_raw_segment_times(t_reloaddata, t_reloadtimes, m_dFiffPosition+m_dWindowLength/2, m_dFiffPosition+3/2*m_dWindowLength))
            printf("Error when reading raw data!");

    // extend m_data with reloaded data
    m_data.prepend(t_reloaddata);
    m_times.prepend(t_reloadtimes);

    qDebug("Fiff data REloaded.");

    QModelIndex topLeft = createIndex(0,1);
    QModelIndex bottomRight = createIndex(m_data[m_iBlockPosition].rows(),1);

    emit dataChanged(topLeft,bottomRight);
}

void RawModel::loadChNames()
{
    for(qint32 i=0; i < m_pfiffIO->m_qlistRaw[0]->info.nchan; ++i)
            m_chnames.append(m_pfiffIO->m_qlistRaw[0]->info.chs[i].ch_name);
}

void RawModel::loadChInfos()
{
    for(qint32 i=0; i < m_pfiffIO->m_qlistRaw[0]->info.nchan; ++i)
        m_chinfolist.append(m_pfiffIO->m_qlistRaw[0]->info.chs[i]);
}

qint32 RawModel::sizeOfData()
{
    return m_data[m_iBlockPosition].cols();
}


double RawModel::maxDataValue(qint32 type) const {

//    QString kind = m_chinfolist[0].channel_type(type);

    MatrixXd picks(m_data[m_iBlockPosition].rows(),m_data[m_iBlockPosition].rows());
    picks.setZero();
    qint32 j = 0;

    for(qint32 i=0; i < m_chinfolist.size(); ++i) {
        if(m_chinfolist[i].kind == type) {
            picks(i,i) = 1;
            ++j;
        }
    }

    MatrixXd picks_data = picks*m_data[m_iBlockPosition];

    double max = picks_data.maxCoeff();
    return max;
}

//*************************************************************************************************************
//slots

void RawModel::reloadData(int value) {
    if(value < n_reloadPos && !m_reloaded) { //ToDo: 1200 is an estimation for the window size -> make it dynamic
        qDebug("reload data at FRONT, value: %i", value);
//        reloadFiffData(true);
        m_reloaded = true;
    }
    else if(value > m_data[m_iBlockPosition].cols()-(n_reloadPos+1200)) {
        qDebug("reload data at AFTER, value: %i", value);
    }

}
