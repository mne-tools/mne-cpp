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
* Copyright (C) 2014, Florian Schlembach, Christoph Dinh, Matti Hamalainen and Jens Haueisen. All rights reserved.
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
* @brief    This class represents the model of the model/view framework of mne_browse_raw_qt application.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rawmodel.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEBrowseRawQt;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RawModel::RawModel(QObject *parent)
: QAbstractTableModel(parent)
, m_bFileloaded(false)
, m_bStartReached(false)
, m_bEndReached(false)
, m_bReloading(false)
, m_bProcessing(false)
, m_fiffInfo(FiffInfo())
, m_pfiffIO(QSharedPointer<FiffIO>(new FiffIO()))
, m_filterChType("All")
{
    m_iWindowSize = MODEL_WINDOW_SIZE;
    m_reloadPos = MODEL_RELOAD_POS;
    m_maxWindows = MODEL_MAX_WINDOWS;
    m_iFilterTaps = MODEL_NUM_FILTER_TAPS;

    //Set default sampling freq to 1024
    m_fiffInfo.sfreq = 1024;

    // Generate default filter operator - This needs to be done here so that the filter design tool works without loading a file
    genStdFilterOps();

    //connect data reloading - this is done concurrently
    connect(&m_reloadFutureWatcher,&QFutureWatcher<QPair<MatrixXd,MatrixXd> >::finished,[this](){
        insertReloadedData(m_reloadFutureWatcher.future().result());
    });

    //connect filtering reloading - this is done after a new block has been loaded
    connect(this,&RawModel::dataReloaded,[this](){
        if(!m_assignedOperators.empty())
            updateOperatorsConcurrently();
    });

//    connect(&m_operatorFutureWatcher,&QFutureWatcher<QPair<int,RowVectorXd> >::resultReadyAt,[this](int index){
//        insertProcessedData(index);
//    });
    connect(&m_operatorFutureWatcher,&QFutureWatcher<void>::finished,[this](){
        insertProcessedDataAll();
    });
//    connect(&m_operatorFutureWatcher,&QFutureWatcher<QPair<int,RowVectorXd> >::progressValueChanged,[this](int progressValue){
//        qDebug() << "RawModel: ProgressValue m_operatorFutureWatcher, " << progressValue << " items processed out of" << m_listTmpChData.size();
//    });
}


//*************************************************************************************************************

RawModel::RawModel(QFile &qFile, QObject *parent)
: QAbstractTableModel(parent)
, m_bFileloaded(false)
, m_bStartReached(false)
, m_bEndReached(false)
, m_bReloading(false)
, m_bProcessing(false)
, m_fiffInfo(FiffInfo())
, m_pfiffIO(QSharedPointer<FiffIO>(new FiffIO()))
, m_filterChType("All")
{
    m_iWindowSize = MODEL_WINDOW_SIZE;
    m_reloadPos = MODEL_RELOAD_POS;
    m_maxWindows = MODEL_MAX_WINDOWS;
    m_iFilterTaps = MODEL_NUM_FILTER_TAPS;

    //read fiff data
    loadFiffData(qFile);

    //generator FilterOperator objects
    genStdFilterOps();

    //connect signal and slots
    connect(&m_reloadFutureWatcher,&QFutureWatcher<QPair<MatrixXd,MatrixXd> >::finished,[this](){
        insertReloadedData(m_reloadFutureWatcher.future().result());
    });

    connect(this,&RawModel::dataReloaded,[this](){
        if(!m_assignedOperators.empty())
            updateOperatorsConcurrently();
    });

//    connect(&m_operatorFutureWatcher,&QFutureWatcher<QPair<int,RowVectorXd> >::resultReadyAt,[this](int index){
//        insertProcessedData(index);
//    });
    connect(&m_operatorFutureWatcher,&QFutureWatcher<void>::finished,[this](){
        insertProcessedDataAll();
    });
//    connect(&m_operatorFutureWatcher,&QFutureWatcher<QPair<int,RowVectorXd> >::progressValueChanged,[this](int progressValue){
//        qDebug() << "RawModel: ProgressValue m_operatorFutureWatcher, " << progressValue << " items processed out of" << m_listTmpChData.size();
//    });
}


//*************************************************************************************************************
//virtual functions
int RawModel::rowCount(const QModelIndex & /*parent*/) const
{
    if(!m_chInfolist.empty())
        return m_chInfolist.size();
    else return 0;
}


//*************************************************************************************************************

int RawModel::columnCount(const QModelIndex & /*parent*/) const
{
    return 3;
}


//*************************************************************************************************************

QVariant RawModel::data(const QModelIndex &index, int role) const
{
    if(role != Qt::DisplayRole && role != Qt::BackgroundRole && role != RawModelRoles::GetChannelMean)
        return QVariant();

    if (index.isValid()) {
        //******** first column (chname) ********
        if(index.column()==0 && role == Qt::DisplayRole)
            return QVariant(m_chInfolist[index.row()].ch_name);

        //******** second column (data plot) ********
        if(index.column()==1) {
            QVariant v;

            switch(role) {
            case Qt::DisplayRole: {
                //form RowVectorPair of pointer and length of RowVector
                QPair<const double*,qint32> rowVectorPair;

                //pack all adjacent (after reload) RowVectorPairs into a QList
                QList<RowVectorPair> listRowVectorPair;

                for(qint16 i=0; i < m_data.size(); ++i) {
                    //if channel is not filtered or background Processing pending...
                    if(!m_assignedOperators.contains(index.row()) || (m_bProcessing && m_bReloadBefore && i==0) || (m_bProcessing && !m_bReloadBefore && i==m_data.size()-1)) {
                        rowVectorPair.first = m_data[i]->dataRaw().data() + index.row()*m_data[i]->dataRaw().cols();
                        rowVectorPair.second  = m_data[i]->dataRaw().cols();
                    }
                    else { //if channel IS filtered
                        rowVectorPair.first = m_data[i]->dataProc().data() + index.row()*m_data[i]->dataProc().cols();
                        rowVectorPair.second  = m_data[i]->dataProc().cols();
                    }

                    listRowVectorPair.append(rowVectorPair);
                }

                v.setValue(listRowVectorPair);
                return v;
                break;
            }

            case Qt::BackgroundRole: { //plot channel red if marked as red
                if(m_fiffInfo.bads.contains(m_chInfolist[index.row()].ch_name)) {
                    QBrush brush;
                    brush.setStyle(Qt::SolidPattern);
//                    qDebug() << m_chInfolist[index.row()].ch_name << "is marked as bad, index:" << index.row();
                    brush.setColor(Qt::darkRed);
                    return QVariant(brush);
                }
                else
                    return QVariant();

                break;
            }
            }
        }

        //******** third column (mean data of the channels plot) ********
        if(index.column()==2) {
            switch(role) {
            case RawModelRoles::GetChannelMean: {
                QVariant v;

                //if channel is not filtered or background Processing pending...
                if(!m_assignedOperators.contains(index.row()) || (m_bProcessing && m_bReloadBefore) || (m_bProcessing && !m_bReloadBefore)) {
                    //Calculate the global mean of all loaded data in m_dataMean
                    double sum = 0;
                    for(int i = 0; i<m_data.size(); i++)
                        sum += m_data[i]->dataRawMean(index.row());

                    v.setValue(sum/m_data.size());
                }
                else { //if channel IS filtered
                    //Calculate the global mean of all loaded data in m_dataMean
                    double sum = 0;
                    for(int i = 0; i<m_data.size(); i++)
                        sum += m_data[i]->dataProcMean(index.row());

                    v.setValue(sum/m_data.size());
                }

                return v;
                break;
            }
            } // end role switch
        } // end column check
    } // end index.valid() check

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
        }
    }

    return QVariant();
}


//*********************+****************************************************************************************

void RawModel::genStdFilterOps()
{
    //clear operators
    //m_Operators.clear();

    //regenerate them with the correct sampling frequency (only required for naming of filter)
    double sfreq = (m_fiffInfo.sfreq>=0) ? m_fiffInfo.sfreq : 600.0;
    double nyquist_freq = sfreq/2;

    int filterTaps = m_iFilterTaps;
    int fftLength = m_iWindowSize+2*filterTaps; //2*filterTaps because we need to add data at the front and back
    int exp = ceil(log2(fftLength));
    fftLength = pow(2, exp);

    //HPF
    double cutoffFreqHz = 50; //in Hz
    QString name = QString("HPF_%1").arg(cutoffFreqHz);
    m_Operators.insert(name,QSharedPointer<MNEOperator>(new FilterOperator(name,FilterOperator::HPF,m_iFilterTaps,cutoffFreqHz/nyquist_freq,0.2,0.1,fftLength)));

    //LPF
    cutoffFreqHz = 30; //in Hz
    name = QString("LPF_%1").arg(cutoffFreqHz);
    m_Operators.insert(name,QSharedPointer<MNEOperator>(new FilterOperator(name,FilterOperator::LPF,m_iFilterTaps,cutoffFreqHz/nyquist_freq,0.2,0.1,fftLength)));
    cutoffFreqHz = 10; //in Hz
    name = QString("LPF_%1").arg(cutoffFreqHz);
    m_Operators.insert(name,QSharedPointer<MNEOperator>(new FilterOperator(name,FilterOperator::LPF,m_iFilterTaps,cutoffFreqHz/nyquist_freq,0.2,0.1,fftLength)));

    //BPF
    double from_freqHz = 30;
    double to_freqHz = 40;
    double trans_width = 15;
    double center = (to_freqHz - from_freqHz)/2; //double center = from_freqHz+bw/2;
    double bw = to_freqHz-from_freqHz; //double bw = to_freqHz/from_freqHz;

    name = QString("BPF_%1-%2").arg(from_freqHz).arg(to_freqHz);
    m_Operators.insert(name,QSharedPointer<MNEOperator>(new FilterOperator(name,FilterOperator::BPF,80,(double)center/nyquist_freq,(double)bw/nyquist_freq,(double)trans_width/nyquist_freq,fftLength)));

    //Own/manual set filter - only an entry i nthe operator list generated which is called when the filterwindow is used
    cutoffFreqHz = 40;
    //only create if filter does not exist yet
    if(!m_Operators.contains(QString("User defined (See 'Adjust/Filter')"))) {
        name = QString("User defined (See 'Adjust/Filter')");
        m_Operators.insert(name,QSharedPointer<MNEOperator>(new FilterOperator(name,FilterOperator::LPF,m_iFilterTaps,cutoffFreqHz/nyquist_freq,0.2,0.1,(m_iWindowSize+m_iFilterTaps))));
    }

    //**********
    //filter debugging -> store filter coefficients to plain text file
//    QFile file("C:/Users/Flo/Desktop/matlab/filter.txt");
//    file.open(QFile::WriteOnly | QFile::Text);
//    QTextStream out(&file);

//    QSharedPointer<FilterOperator> filtPtr = m_Operators[name].staticCast<FilterOperator>();
//    for(int i=0; i < filtPtr->m_dCoeffA.cols(); ++i)
//        out << filtPtr->m_dCoeffA(0,i) << endl;

//    file.close();
    //**********

    qDebug() << "RawModel: Standard FilterOperators generated and loaded.";
}


//*************************************************************************************************************

bool RawModel::loadFiffData(QFile& qFile)
{
    beginResetModel();
    clearModel();

    MatrixXd t_data,t_times; //type is later on (when append to m_data) casted into MatrixXdR (Row-Major)
    QSharedPointer<DataPackage> newDataPackage;

    m_pfiffIO = QSharedPointer<FiffIO>(new FiffIO(qFile));
    if(!m_pfiffIO->m_qlistRaw.empty()) {
        m_iAbsFiffCursor = m_pfiffIO->m_qlistRaw[0]->first_samp; //Set cursor somewhere into fiff file [in samples]
        m_bStartReached = true;

        int start = m_iAbsFiffCursor - MODEL_MAX_NUM_FILTER_TAPS/2;
        int end = start + m_iWindowSize - 1 + MODEL_MAX_NUM_FILTER_TAPS;

        if(!m_pfiffIO->m_qlistRaw[0]->read_raw_segment(t_data, t_times, start, end))
            return false;

        //This is the first data laoded, hence no data to the left possible, but needed for correct filtering -> mirror data to the front
        MatrixXdR tempMat(t_data.rows(), t_data.cols() + MODEL_MAX_NUM_FILTER_TAPS/2);
        tempMat.block(0,0,t_data.rows(),MODEL_MAX_NUM_FILTER_TAPS/2) = t_data.block(0,0,t_data.rows(),MODEL_MAX_NUM_FILTER_TAPS/2);
        tempMat.block(0,MODEL_MAX_NUM_FILTER_TAPS/2,t_data.rows(),t_data.cols()) = t_data;

        newDataPackage = QSharedPointer<DataPackage>(new DataPackage(tempMat, (MatrixXdR)t_times, MODEL_MAX_NUM_FILTER_TAPS/2, MODEL_MAX_NUM_FILTER_TAPS/2));

        m_bFileloaded = true;
    }
    else {
        qDebug("RawModel: ERROR! Data set does not contain any fiff data!");
        endResetModel();
        m_bFileloaded = false;
        return false;
    }

    //set loaded fiff data
    m_data.append(newDataPackage);

    loadFiffInfos();
    genStdFilterOps();

    endResetModel();

    emit fileLoaded(m_fiffInfo);
    emit assignedOperatorsChanged(m_assignedOperators);

    return true;
}


//*************************************************************************************************************

bool RawModel::writeFiffData(QFile &qFile)
{
    //replace m_fiffInfo with the one contained in the m_pfiffIO object (for replacing bad channels)
    m_pfiffIO->m_qlistRaw[0]->info = m_fiffInfo;

    if (m_pfiffIO->write_raw(qFile,0)) { ; //ToDo: by now, fiff data file is completely written new -> substitute only FiffInfo in old file?
        qDebug() << "Done saving as fiff file" << qFile.fileName() << "...";
        return true;
    }
    else
        return false;
}


//*************************************************************************************************************
//non-virtual functions
//private
void RawModel::loadFiffInfos()
{
    //loads chinfos
    for(qint32 i=0; i < m_pfiffIO->m_qlistRaw[0]->info.nchan; ++i)
        m_chInfolist.append(m_pfiffIO->m_qlistRaw[0]->info.chs[i]);

    //loads fiffInfo
    m_fiffInfo = m_pfiffIO->m_qlistRaw[0]->info;
}


//*************************************************************************************************************

void RawModel::clearModel()
{
    //FiffIO object
    m_pfiffIO.clear();
    m_fiffInfo.clear();
    m_chInfolist.clear();

    //data model structure
    m_data.clear();

    //MNEOperators
    m_assignedOperators.clear();

    //View parameters
    m_iAbsFiffCursor = 0;
    m_iCurAbsScrollPos = 0;
    m_bStartReached = false;
    m_bEndReached = false;

    qDebug("RawModel cleared.");
}


//*************************************************************************************************************

void RawModel::resetPosition(qint32 position)
{
    beginResetModel();

    //reset members
    m_data.clear();

    m_bStartReached = false;
    m_bEndReached = false;
    m_bReloading = false;
    m_bProcessing = false;

    //calculate multiple integer of m_iWindowSize from beginning of Fiff file (rounded down)
    qint32 distance = position - firstSample();
    qint32 mult = floor(distance / m_iWindowSize);

    m_iAbsFiffCursor = firstSample() + mult*m_iWindowSize;

    MatrixXd t_data,t_times; //type is later on (when append to m_data) casted into MatrixXdR (Row-Major)

    int start = m_iAbsFiffCursor - MODEL_MAX_NUM_FILTER_TAPS/2;
    int end = start + m_iWindowSize - 1 + MODEL_MAX_NUM_FILTER_TAPS;

    m_Mutex.lock();
    if(!m_pfiffIO->m_qlistRaw[0]->read_raw_segment(t_data, t_times, start, end))
        qDebug() << "RawModel: Error resetting position of Fiff file!";
    m_Mutex.unlock();

    //Possible position of the reset can cause no data to the left or right available, but needed for correct filtering -> mirror data to the front or back
    MatrixXdR tempMat(t_data.rows(), t_data.cols() + MODEL_MAX_NUM_FILTER_TAPS/2);
    QSharedPointer<DataPackage> newDataPackage;

    if(start<firstSample()) { //no data to the left - mirror data to the left
        tempMat.block(0,0,t_data.rows(),MODEL_MAX_NUM_FILTER_TAPS/2) = t_data.block(0,0,t_data.rows(),MODEL_MAX_NUM_FILTER_TAPS/2);
        tempMat.block(0,MODEL_MAX_NUM_FILTER_TAPS/2,t_data.rows(),t_data.cols()) = t_data;
        newDataPackage = QSharedPointer<DataPackage>(new DataPackage(DataPackage(tempMat, (MatrixXdR)t_times, MODEL_MAX_NUM_FILTER_TAPS/2, MODEL_MAX_NUM_FILTER_TAPS/2)));
    }
    else if(end>lastSample()) { //no data to the right - mirror data to the right
        tempMat.block(0,0,t_data.rows(),t_data.cols()) = t_data;
        tempMat.block(0,t_data.cols(),t_data.rows(),MODEL_MAX_NUM_FILTER_TAPS/2) = t_data.block(0,t_data.cols()-MODEL_MAX_NUM_FILTER_TAPS/2,t_data.rows(),MODEL_MAX_NUM_FILTER_TAPS/2);
        newDataPackage = QSharedPointer<DataPackage>(new DataPackage(DataPackage(tempMat, (MatrixXdR)t_times, MODEL_MAX_NUM_FILTER_TAPS/2, MODEL_MAX_NUM_FILTER_TAPS/2)));
    }
    else
        newDataPackage = QSharedPointer<DataPackage>(new DataPackage((MatrixXdR)t_data, (MatrixXdR)t_times, MODEL_MAX_NUM_FILTER_TAPS/2, MODEL_MAX_NUM_FILTER_TAPS/2));

    //append loaded block
    m_data.append(newDataPackage);

    updateOperators();

    endResetModel();

//    if(!(m_iAbsFiffCursor<=firstSample()))
//        updateScrollPos(m_iCurAbsScrollPos-firstSample()); //little hack: if the m_iCurAbsScrollPos is now close to the edge -> force reloading w/o scrolling

    qDebug() << "RawModel: Model Position RESET, samples from " << m_iAbsFiffCursor << "to" << m_iAbsFiffCursor+m_iWindowSize-1 << "reloaded.";

    emit dataChanged(createIndex(0,1),createIndex(m_chInfolist.size(),1));
}


//*************************************************************************************************************

void RawModel::reloadFiffData(bool before)
{
    m_bReloadBefore = before;

    //update scroll position
    fiff_int_t start,end;
    if(before) {
        m_iAbsFiffCursor -= m_iWindowSize;
        start = m_iAbsFiffCursor - MODEL_MAX_NUM_FILTER_TAPS/2;
        end = start + m_iWindowSize - 1 + MODEL_MAX_NUM_FILTER_TAPS;

        //check if start of fiff file is reached
        if(start < firstSample()) {
            m_bStartReached = true;
            qDebug() << "RawModel: Start of fiff file reached.";

            m_iAbsFiffCursor = firstSample();
            //resetPosition(m_iAbsFiffCursor);
            return;
        }
    }
    else {
        start = m_iAbsFiffCursor + sizeOfPreloadedData() - MODEL_MAX_NUM_FILTER_TAPS/2;
        end = start + m_iWindowSize - 1 + MODEL_MAX_NUM_FILTER_TAPS;

        //check if end of fiff file is reached
        if(end > lastSample()) {
            //Reload one more time
            if(m_bEndReached)
                return;
            else
                m_bEndReached = true;

            end = lastSample();
            qDebug() << "RawModel: End of fiff file reached.";
        }
    }

    m_bReloading = true;

    //read data with respect to start and end point
    QFuture<QPair<MatrixXd,MatrixXd> > future = QtConcurrent::run(this,&RawModel::readSegment,start,end);

    //Wait for thread reloading is finished, then insert reloaded data
//    future.waitForFinished();
//    insertReloadedData(future.result());

    m_reloadFutureWatcher.setFuture(future);
}


//*************************************************************************************************************

QPair<MatrixXd,MatrixXd> RawModel::readSegment(fiff_int_t from, fiff_int_t to)
{
    QPair<MatrixXd,MatrixXd> datatime;

    m_Mutex.lock();
    if(!m_pfiffIO->m_qlistRaw[0]->read_raw_segment(datatime.first, datatime.second, from, to)) {
        printf("RawModel: Error when reading raw data!");
        return datatime;
    }
    m_Mutex.unlock();

    return datatime;
}


//*************************************************************************************************************
//public SLOTS
void RawModel::updateScrollPos(int value)
{
    m_iCurAbsScrollPos = firstSample() + value;
    qDebug() << "RawModel: absolute Fiff Scroll Cursor" << m_iCurAbsScrollPos << "(m_iAbsFiffCursor" << m_iAbsFiffCursor << ", sizeOfPreloadedData" << sizeOfPreloadedData() << ", firstSample()" << firstSample() << ")";

    //if a scroll position is selected, which is not within the loaded data range -> reset position of model
    if(m_iCurAbsScrollPos > (m_iAbsFiffCursor+sizeOfPreloadedData()+m_iWindowSize) || m_iCurAbsScrollPos < m_iAbsFiffCursor) {
        qDebug() << "RawModel: Reset position requested, m_iAbsFiffCursor:" << m_iAbsFiffCursor << "m_iCurAbsScrollPos:" << m_iCurAbsScrollPos;
        resetPosition(m_iCurAbsScrollPos);
        return;
    }

    //reload data if end of loaded range is reached and no relaoding is currently active
    //front
    if(!m_bReloading && (m_iCurAbsScrollPos-m_iAbsFiffCursor < m_reloadPos) && !m_bStartReached) {
        qDebug() << "RawModel: Reload requested at FRONT of loaded fiff data, m_iAbsFiffCursor:" << m_iAbsFiffCursor << "m_iCurAbsScrollPos:" << m_iCurAbsScrollPos;
        reloadFiffData(1);
    }
    //end
    else if(!m_bReloading && m_iCurAbsScrollPos > m_iAbsFiffCursor+sizeOfPreloadedData()-m_reloadPos && !m_bEndReached) {
        qDebug() << "RawModel: Reload requested at END of loaded fiff data, m_iAbsFiffCursor:" << m_iAbsFiffCursor << "m_iCurAbsScrollPos:" << m_iCurAbsScrollPos;
        reloadFiffData(0);
    }
}


//*************************************************************************************************************

void RawModel::markChBad(QModelIndexList chlist, bool status)
{
    for(qint8 i=0; i < chlist.size(); ++i) {
        if(status) {
            if(!m_fiffInfo.bads.contains(m_chInfolist[chlist[i].row()].ch_name))
                m_fiffInfo.bads.append(m_chInfolist[chlist[i].row()].ch_name);
            qDebug() << "RawModel:" << m_chInfolist[chlist[i].row()].ch_name << "marked as bad.";
        }
        else {
            if(m_fiffInfo.bads.contains(m_chInfolist[chlist[i].row()].ch_name)) {
                int index = m_fiffInfo.bads.indexOf(m_chInfolist[chlist[i].row()].ch_name);
                m_fiffInfo.bads.removeAt(index);
                qDebug() << "RawModel:" << m_chInfolist[chlist[i].row()].ch_name << "marked as good.";
            }
        }

        emit dataChanged(chlist[i],chlist[i]);
    }
}


//*************************************************************************************************************

void RawModel::applyOperator(QModelIndexList chlist, const QSharedPointer<MNEOperator>& operatorPtr, const QString &chType)
{
    m_filterChType = chType;

    //clear channel list becuase we will generate a new one
    chlist.clear();

    //filter only channels which include chType in their names
    if(chType == "All") {
        for(qint32 i=0; i < m_chInfolist.size(); ++i) {
            if(!m_chInfolist.at(i).ch_name.contains("STI") && !m_chInfolist.at(i).ch_name.contains("MISC") && !m_chInfolist.at(i).ch_name.contains("TRG"))
                if(!m_assignedOperators.values(i).contains(operatorPtr))
                    m_assignedOperators.insertMulti(i,operatorPtr);
        }
    }
    else {
        for(qint32 i=0; i < m_chInfolist.size(); ++i) {
            if(!m_chInfolist.at(i).ch_name.contains("STI") && !m_chInfolist.at(i).ch_name.contains("MISC") && !m_chInfolist.at(i).ch_name.contains("TRG") && m_chInfolist.at(i).ch_name.contains(chType))
                if(!m_assignedOperators.values(i).contains(operatorPtr))
                    m_assignedOperators.insertMulti(i,operatorPtr);
        }
    }

    for(int i=0; i<m_data.size(); i++)
        updateOperatorsConcurrently(i);

    emit assignedOperatorsChanged(m_assignedOperators);

    qDebug() << "RawModel: using FilterType" << operatorPtr->m_sName;
}


//*************************************************************************************************************

void RawModel::applyOperator(QModelIndexList chlist, const QSharedPointer<MNEOperator>& operatorPtr)
{
    //filter all when chlist is empty
    if(chlist.empty()) {
        for(qint32 i=0; i < m_chInfolist.size(); ++i) {
            if(chlist.at(i).column()==1)
                if(!m_chInfolist.at(i).ch_name.contains("STI") && !m_chInfolist.at(i).ch_name.contains("MISC") && !m_chInfolist.at(i).ch_name.contains("TRG"))
                    if(!m_assignedOperators.values(i).contains(operatorPtr))
                        m_assignedOperators.insertMulti(i,operatorPtr);
        }
    }

    for(qint32 i=0; i < chlist.size(); ++i) {  //iterate through selected channels to filter
        if(chlist.at(i).column()==1) {
            QString chName = data(index(i,0)).toString();
            if(!chName.contains("STI") && !chName.contains("MISC") && !chName.contains("TRG"))
                if(!m_assignedOperators.values(chlist.at(i).row()).contains(operatorPtr))
                    m_assignedOperators.insertMulti(chlist.at(i).row(),operatorPtr);
        }
    }

    for(int i=0; i<m_data.size(); i++)
        updateOperatorsConcurrently(i);

    emit assignedOperatorsChanged(m_assignedOperators);

    qDebug() << "RawModel: using FilterType" << operatorPtr->m_sName;
}


//*************************************************************************************************************

void RawModel::applyOperatorsConcurrently(QPair<int,RowVectorXd>& chdata) const
{
    QSharedPointer<FilterOperator> filter;

    //QList<int> listFilteredChs = m_assignedOperators.keys();

    QList<QSharedPointer<MNEOperator> > ops = m_assignedOperators.values(chdata.first);
    for(qint32 i=0; i < ops.size(); ++i) {
        switch(ops[i]->m_OperatorType) {
        case MNEOperator::FILTER: {
            filter = ops[i].staticCast<FilterOperator>();
            RowVectorXd tmp = filter->applyFFTFilter(chdata.second);
            chdata.second = tmp;
        }
        case MNEOperator::PCA: {
            //do something
        }
        }
    }
//    return chdata;
}


//*************************************************************************************************************

void RawModel::updateOperators(QModelIndex chan)
{
    for(qint32 i=0; i < m_assignedOperators.values(chan.row()).size(); ++i) {
        for(qint32 j=0; j < m_assignedOperators.values(chan.row()).size(); ++j) {
            if(!m_assignedOperators.values(chan.row()).contains(m_assignedOperators.values(chan.row())[j]))
                m_assignedOperators.insertMulti(chan.row(), m_assignedOperators.values(chan.row())[j]);
        }
    }

    for(int i=0; i<m_data.size(); i++)
        updateOperatorsConcurrently(i);

    emit assignedOperatorsChanged(m_assignedOperators);
}


//*************************************************************************************************************

void RawModel::updateOperators(QModelIndexList chlist)
{
    if(chlist.empty())
        for(qint32 i=0; i < m_chInfolist.size(); ++i)
            chlist.append(createIndex(i,1));

    for(qint32 i=0; i < chlist.size(); ++i) {
        for(qint32 j=0; j < m_assignedOperators.values(chlist[i].row()).size(); ++j) {
            if(!m_assignedOperators.values(chlist[i].row()).contains(m_assignedOperators.values(chlist[i].row())[j]))
                m_assignedOperators.insertMulti(chlist[i].row(), m_assignedOperators.values(chlist[i].row())[j]);
        }
    }

    for(int i=0; i<m_data.size(); i++)
        updateOperatorsConcurrently(i);

    emit assignedOperatorsChanged(m_assignedOperators);
}


//*************************************************************************************************************

void RawModel::updateOperators()
{
    updateOperators(QModelIndexList());
}


//*************************************************************************************************************

void RawModel::undoFilter(QModelIndexList chlist, const QSharedPointer<MNEOperator> &filterPtr)
{
    for(qint32 i=0; i < chlist.size(); ++i) {
        if(m_assignedOperators.contains(chlist[i].row()) && m_assignedOperators.values(chlist[i].row()).contains(filterPtr)) {
            QMutableMapIterator<int,QSharedPointer<MNEOperator> > it(m_assignedOperators);
            while(it.hasNext()) {
                it.next();
                if(it.key()==chlist[i].row() && it.value()==filterPtr) {
                    it.remove();
                    qDebug() << "RawModel: Filter operator removed of type" << filterPtr->m_sName << "for channel" << chlist[i].row();
                    updateOperators(chlist[i]);
                    continue;
                }
            }

        }
        else {
            qDebug() << "RawModel: No filter of type" << filterPtr->m_sName << "applied to channel" << chlist[i].row();
            continue;
        }
    }

    emit assignedOperatorsChanged(m_assignedOperators);
}


//*************************************************************************************************************

void RawModel::undoFilter(QModelIndexList chlist)
{
    for(qint32 i=0; i < chlist.size(); ++i) {
        m_assignedOperators.remove(chlist[i].row());
        qDebug() << "RawModel: All filter operator removed of type for channel" << chlist[i].row();
    }

    emit assignedOperatorsChanged(m_assignedOperators);
}


//*************************************************************************************************************

void RawModel::undoFilter(const QString &chType)
{
    if(chType == "All")
        undoFilter();
    else {
        //filter only channels which include chType in their names
        for(qint32 i=0; i < m_chInfolist.size(); ++i)
            if(m_chInfolist.at(i).ch_name.contains(chType))
                m_assignedOperators.remove(i);
    }

    emit assignedOperatorsChanged(m_assignedOperators);
}



//*************************************************************************************************************

void RawModel::undoFilter()
{
    m_assignedOperators.clear();

    emit assignedOperatorsChanged(m_assignedOperators);
}


//*************************************************************************************************************
//private SLOTS
void RawModel::insertReloadedData(QPair<MatrixXd,MatrixXd> dataTimesPair)
{
    QSharedPointer<DataPackage> newDataPackage = QSharedPointer<DataPackage>(new DataPackage((MatrixXdR)dataTimesPair.first, (MatrixXdR)dataTimesPair.second, MODEL_MAX_NUM_FILTER_TAPS/2, MODEL_MAX_NUM_FILTER_TAPS/2));

    //extend m_data with reloaded data
    if(m_bReloadBefore) {
        m_data.prepend(newDataPackage);

        //maintain at maximum m_maxWindows data windows and drop the rest
        if(m_data.size() > m_maxWindows) {
            m_data.removeLast();
        }
    }
    else {
        m_data.append(newDataPackage);

        //maintain at maximum m_maxWindows data windows and drop the rest
        if(m_data.size() > m_maxWindows) {
            m_data.removeFirst();
            m_iAbsFiffCursor += m_iWindowSize;
        }
    }

    m_bReloading = false;

    emit dataChanged(createIndex(0,1),createIndex(m_chInfolist.size()-1,1));
    emit dataReloaded();

    //delete newDataPackage;

    qDebug() << "RawModel: Fiff data Reloaded from " << dataTimesPair.second.coeff(0) << "secs to" << dataTimesPair.second.coeff(dataTimesPair.second.cols()-1) << "secs";
}


//*************************************************************************************************************

void RawModel::updateOperatorsConcurrently()
{
    m_bProcessing = true;

    QList<int> listFilteredChs = m_assignedOperators.keys();
    m_listTmpChData.clear();

    //get the rows which are to be filtered out of the m_data matrix. Note that this is done windows wise, hence jumps in the filtered signal might be visible
    for(qint32 i=0; i < listFilteredChs.size(); ++i) {
        if(m_bReloadBefore)
            m_listTmpChData.append(QPair<int,RowVectorXd>(listFilteredChs[i],m_data.first()->dataRawOrig().row(listFilteredChs[i])));
        else
            m_listTmpChData.append(QPair<int,RowVectorXd>(listFilteredChs[i],m_data.last()->dataRawOrig().row(listFilteredChs[i])));
    }

    qDebug() << "RawModel: Starting of concurrent PROCESSING operation of" << listFilteredChs.size() << "items";

    //************* here it could be also performed QtConcurrent::mapped() *************
    // advantage: QFutureWatcher would give partial results -> signal resultsReadyAt(int idx)
    // disadvantage: data needs to be copied twice (also to QFuture<QPair<int,RowVectorXd> > object) instead of once (to m_listTmpChData)

    //generate lambda function
//    std::function<QPair<int,RowVectorXd> (QPair<int,RowVectorXd>&)> applyOps = [this](QPair<int,RowVectorXd>& chdata) -> QPair<int,RowVectorXd> {
//        return applyOperatorsConcurrently(chdata);
//    };

//    QFuture<QPair<int,RowVectorXd> > future = QtConcurrent::mapped(m_listTmpChData.begin(),m_listTmpChData.end(),applyOps);
    //**************************************************************************************************************************************************************************

    QFuture<void > future = QtConcurrent::map(m_listTmpChData,[this](QPair<int,RowVectorXd>& chdata) {
        return applyOperatorsConcurrently(chdata);
    });

    m_operatorFutureWatcher.setFuture(future);

    //Wait for thread to be finished processig the data, then insert data
    //future.waitForFinished();

    qDebug() << "RawModel: finished concurrent PROCESSING operation of" << listFilteredChs.size() << "items";

    //insertProcessedDataAll();
}


//*************************************************************************************************************

void RawModel::updateOperatorsConcurrently(int windowIndex)
{
    if(windowIndex >= m_data.size() || windowIndex < 0)
        windowIndex = 0;

    m_bProcessing = true;

    QList<int> listFilteredChs = m_assignedOperators.keys();
    m_listTmpChData.clear();

    //get the rows which are to be filtered out of the m_data matrix. Note that this is done windows wise, hence jumps in the filtered signal might be visible
    for(qint32 i=0; i < listFilteredChs.size(); ++i) {
        if(m_bReloadBefore)
            m_listTmpChData.append(QPair<int,RowVectorXd>(listFilteredChs[i],m_data[windowIndex]->dataRawOrig().row(listFilteredChs[i])));
        else
            m_listTmpChData.append(QPair<int,RowVectorXd>(listFilteredChs[i],m_data[windowIndex]->dataRawOrig().row(listFilteredChs[i])));
    }

    qDebug() << "RawModel: Starting of concurrent PROCESSING operation of" << listFilteredChs.size() << "items in m_data block"<<windowIndex;

    //************* here it could be also performed QtConcurrent::mapped() *************
    // advantage: QFutureWatcher would give partial results -> signal resultsReadyAt(int idx)
    // disadvantage: data needs to be copied twice (also to QFuture<QPair<int,RowVectorXd> > object) instead of once (to m_listTmpChData)

    //generate lambda function
//    std::function<QPair<int,RowVectorXd> (QPair<int,RowVectorXd>&)> applyOps = [this](QPair<int,RowVectorXd>& chdata) -> QPair<int,RowVectorXd> {
//        return applyOperatorsConcurrently(chdata);
//    };

//    QFuture<QPair<int,RowVectorXd> > future = QtConcurrent::mapped(m_listTmpChData.begin(),m_listTmpChData.end(),applyOps);
    //**************************************************************************************************************************************************************************

    QFuture<void > future = QtConcurrent::map(m_listTmpChData,[this](QPair<int,RowVectorXd>& chdata) {
        return applyOperatorsConcurrently(chdata);
    });

    //Wait for thread to be finished processig the data, then insert data
    future.waitForFinished();

    qDebug() << "RawModel: finished concurrent PROCESSING operation of" << listFilteredChs.size() << "items";

    insertProcessedDataAll(windowIndex);
}


//*************************************************************************************************************

void RawModel::insertProcessedDataRow(int rowIndex)
{
    QList<int> listFilteredChs = m_assignedOperators.keys();

    if(m_bReloadBefore) {
        m_data.first()->setOrigProcData(m_listTmpChData[rowIndex].second, m_listTmpChData[rowIndex].first);
        m_data.first()->cutOrigProcData(MODEL_MAX_NUM_FILTER_TAPS/2, MODEL_MAX_NUM_FILTER_TAPS/2);
    }
    else {
        m_data.last()->setOrigProcData(m_listTmpChData[rowIndex].second, m_listTmpChData[rowIndex].first);
        m_data.last()->cutOrigProcData(MODEL_MAX_NUM_FILTER_TAPS/2, MODEL_MAX_NUM_FILTER_TAPS/2);
    }

    emit dataChanged(createIndex(listFilteredChs[rowIndex],1),createIndex(listFilteredChs[rowIndex],1));

    if(rowIndex == listFilteredChs.last())
        m_bProcessing = false;
}


//*************************************************************************************************************

void RawModel::insertProcessedDataAll(int windowIndex)
{
    if(windowIndex >= m_data.size() || windowIndex < 0)
        windowIndex = 0;

    QList<int> listFilteredChs = m_assignedOperators.keys();

    for(qint32 i=0; i < listFilteredChs.size(); ++i) {
        if(m_bReloadBefore)
            m_data[windowIndex]->setOrigProcData(m_listTmpChData[i].second, listFilteredChs[i]);
        else
            m_data[windowIndex]->setOrigProcData(m_listTmpChData[i].second, listFilteredChs[i]);
    }

    //Cut original data to window size and calculate mean for filtered data
    if(m_bReloadBefore)
        m_data[windowIndex]->cutOrigProcData(MODEL_MAX_NUM_FILTER_TAPS/2, MODEL_MAX_NUM_FILTER_TAPS/2);
    else
        m_data[windowIndex]->cutOrigProcData(MODEL_MAX_NUM_FILTER_TAPS/2, MODEL_MAX_NUM_FILTER_TAPS/2);

    emit dataChanged(createIndex(0,1),createIndex(m_chInfolist.size(),1));

    qDebug() << "RawModel: Finished inserting" << listFilteredChs.size() << "channels.";
    m_bProcessing = false;
}


//*************************************************************************************************************

void RawModel::insertProcessedDataAll()
{
    QList<int> listFilteredChs = m_assignedOperators.keys();

    for(qint32 i=0; i < listFilteredChs.size(); ++i) {
        if(m_bReloadBefore)
            m_data.first()->setOrigProcData(m_listTmpChData[i].second, listFilteredChs[i]);
        else
            m_data.last()->setOrigProcData(m_listTmpChData[i].second, listFilteredChs[i]);
    }

    //Cut original data to window size and calculate mean for filtered data
    if(m_bReloadBefore)
        m_data.first()->cutOrigProcData(MODEL_MAX_NUM_FILTER_TAPS/2, MODEL_MAX_NUM_FILTER_TAPS/2);
    else
        m_data.last()->cutOrigProcData(MODEL_MAX_NUM_FILTER_TAPS/2, MODEL_MAX_NUM_FILTER_TAPS/2);

    emit dataChanged(createIndex(0,1),createIndex(m_chInfolist.size(),1));

    qDebug() << "RawModel: Finished inserting" << listFilteredChs.size() << "channels.";
    m_bProcessing = false;
}
