//=============================================================================================================
/**
* @file     eventmodel.cpp
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
*           Jens Haueisen <jens.haueisen@tu-ilmenau.de>
* @version  1.0
* @date     July, 2014
*
* @section  LICENSE
*
* Copyright (C) 2014, Lorenz Esch, Christoph Dinh, Matti Hamalainen and Jens Haueisen. All rights reserved.
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
* @brief    This class represents the model of the model/view framework of mne_browse_raw_qt application.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "eventmodel.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEBrowseRawQt;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

EventModel::EventModel(QObject *parent)
: QAbstractTableModel(parent)
{
//    m_iWindowSize = m_qSettings.value("RawModel/window_size").toInt();
//    m_reloadPos = m_qSettings.value("RawModel/reload_pos").toInt();
//    m_maxWindows = m_qSettings.value("RawModel/max_windows").toInt();
}

//*************************************************************************************************************

EventModel::EventModel(QFile &qFile, QObject *parent)
: QAbstractTableModel(parent)
//, m_bFileloaded(false)
//, m_qSettings()
//, m_bStartReached(false)
//, m_bEndReached(false)
//, m_bReloading(false)
//, m_bProcessing(false)
{
//    m_iWindowSize = m_qSettings.value("RawModel/window_size").toInt();
//    m_reloadPos = m_qSettings.value("RawModel/reload_pos").toInt();
//    m_maxWindows = m_qSettings.value("RawModel/max_windows").toInt();
//    m_iFilterTaps = m_qSettings.value("RawModel/num_filter_taps").toInt();

//    //connect signal and slots
//    connect(&m_reloadFutureWatcher,&QFutureWatcher<QPair<MatrixXd,MatrixXd> >::finished,[this](){
//        insertReloadedData(m_reloadFutureWatcher.future().result());
//    });

//    connect(this,&RawModel::dataReloaded,[this](){
//        if(!m_assignedOperators.empty()) updateOperatorsConcurrently();
//    });

////    connect(&m_operatorFutureWatcher,&QFutureWatcher<QPair<int,RowVectorXd> >::resultReadyAt,[this](int index){
////        insertProcessedData(index);
////    });
//    connect(&m_operatorFutureWatcher,&QFutureWatcher<void>::finished,[this](){
//        insertProcessedData();
//    });

//    connect(&m_operatorFutureWatcher,&QFutureWatcher<QPair<int,RowVectorXd> >::progressValueChanged,[this](int progressValue){
//        qDebug() << "RawModel: ProgressValue m_operatorFutureWatcher, " << progressValue << " items processed out of" << m_listTmpChData.size();
//    });
}


//*************************************************************************************************************
//virtual functions
int EventModel::rowCount(const QModelIndex & /*parent*/) const
{
    if(!m_data.rows()==0)
        return m_data.rows();
    else return 0;
}


//*************************************************************************************************************

int EventModel::columnCount(const QModelIndex & /*parent*/) const
{
    return 3;
}


//*************************************************************************************************************

QVariant EventModel::data(const QModelIndex &index, int role) const
{
//    if(role != Qt::DisplayRole && role != Qt::BackgroundRole)
//        return QVariant();

//    if (index.isValid()) {
//        //******** first column (sample value) ********
//        if(index.column()==0 && role == Qt::DisplayRole)
//            return QVariant(m_data[index.row()].ch_name);

//        //******** second column (data plot) ********
//        if(index.column()==1) {
//            QVariant v;

//            switch(role) {
//            case Qt::DisplayRole: {
//                //form RowVectorPair of pointer and length of RowVector
//                QPair<const double*,qint32> rowVectorPair;

//                //pack all adjacent (after reload) RowVectorPairs into a QList
//                QList<RowVectorPair> listRowVectorPair;

//                for(qint16 i=0; i < m_data.size(); ++i) {
//                    //if channel is not filtered or background Processing pending...
//                    if(!m_assignedOperators.contains(index.row()) || (m_bProcessing && m_bReloadBefore && i==0) || (m_bProcessing && !m_bReloadBefore && i==m_data.size()-1)) {
//                        rowVectorPair.first = m_data[i].data() + index.row()*m_data[i].cols();
//                        rowVectorPair.second  = m_data[i].cols();
//                    }
//                    else { //if channel IS filtered
//                        rowVectorPair.first = m_procData[i].data() + index.row()*m_procData[i].cols();
//                        rowVectorPair.second  = m_procData[i].cols();
//                    }

//                    listRowVectorPair.append(rowVectorPair);
//                }

//                v.setValue(listRowVectorPair);
//                return v;
//                break;
//            }
//            case Qt::BackgroundRole: {
//                if(m_fiffInfo.bads.contains(m_chInfolist[index.row()].ch_name)) {
//                    QBrush brush;
//                    brush.setStyle(Qt::SolidPattern);
////                    qDebug() << m_chInfolist[index.row()].ch_name << "is marked as bad, index:" << index.row();
//                    brush.setColor(Qt::red);
//                    return QVariant(brush);
//                }
//                else
//                    return QVariant();

//                break;
//            }
//        } // end role switch
//    } // end column check

//    } // end index.valid() check

    return QVariant();
}


//*************************************************************************************************************

bool EventModel::loadEventData(QFile& qFile)
{
    beginResetModel();
    clearModel();

    // Read events
    MatrixXi events;

    if(!MNE::read_events(qFile, events))
    {
        qDebug() << "Error while read events.";
        return false;
    }

    std::cout << events << endl;

    qDebug() << QString("Events read from %1").arg(qFile.fileName());

    //set loaded fiff event data
    m_data = events;

    endResetModel();
    return true;
}


//*************************************************************************************************************

void EventModel::clearModel()
{
    //data model structure
//    m_dataclear();

//    //View parameters
//    m_iAbsFiffCursor = 0;
//    m_iCurAbsScrollPos = 0;
//    m_bStartReached = false;
//    m_bEndReached = false;

    qDebug("EventModel cleared.");
}
