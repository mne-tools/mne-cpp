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
    return 2;
}


//*************************************************************************************************************

QVariant EventModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(role != Qt::DisplayRole && role != Qt::TextAlignmentRole)
        return QVariant();

    if(orientation == Qt::Horizontal) {
        switch(section) {
        case 0: //chname column
            return QVariant("Sample");
        case 1: //data plot column
            return QVariant("Type");
        }
    }
    else if(orientation == Qt::Vertical) {
        return QString("Event %1").arg(section);
    }

    return QVariant();
}


//*************************************************************************************************************

QVariant EventModel::data(const QModelIndex &index, int role) const
{
    if(role != Qt::DisplayRole && role != Qt::BackgroundRole)
        return QVariant();

    if(index.column()>m_data.cols() || index.row()>m_data.rows())
        return QVariant();

    if (index.isValid()) {
        //******** first column (sample index) ********
        if(index.column()==0 && role == Qt::DisplayRole)
            return QVariant(m_data(index.row(), 0));

        //******** second column (event type plot) ********
        if(index.column()==1 && role == Qt::DisplayRole)
            return QVariant(m_data(index.row(), 2));

    } // end index.valid() check

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
        qDebug() << "Error while reading events.";
        return false;
    }

    //std::cout << events << endl;

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
