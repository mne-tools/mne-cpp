//=============================================================================================================
/**
* @file     babymeg.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Limin Sun <liminsun@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     February, 2013
*
* @section  LICENSE
*
* Copyright (C) 2013, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Contains the implementation of the BabyMEG class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "babymeg.h"

#include "FormFiles/babymegsetupwidget.h"
#include "FormFiles/babymegprojectdialog.h"


#include <utils/ioutils.h>

#include <iostream>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore/QtPlugin>
#include <QtCore/QTextStream>
#include <QtCore/QFile>

#include <QList>
#include <QDebug>
#include <QDir>
#include <QDateTime>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace BabyMEGPlugin;
using namespace UTILSLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

BabyMEG::BabyMEG()
: m_iBlinkStatus(0)
, m_iBufferSize(-1)
, m_bWriteToFile(false)
, m_sCurrentParadigm("")
, m_bIsRunning(false)
, m_pRawMatrixBuffer(0)
{
    m_pActionSetupProject = new QAction(QIcon(":/images/database.png"), tr("Setup Project"),this);
//    m_pActionSetupProject->setShortcut(tr("F12"));
    m_pActionSetupProject->setStatusTip(tr("Setup Project"));
    connect(m_pActionSetupProject, &QAction::triggered, this, &BabyMEG::showProjectDialog);
    addPluginAction(m_pActionSetupProject);

    m_pActionUpdateFiffInfo = new QAction(QIcon(":/images/latestFiffInfo.png"), tr("Update Fiff Info"),this);
    m_pActionUpdateFiffInfo->setStatusTip(tr("Update Fiff Info"));
    connect(m_pActionUpdateFiffInfo, &QAction::triggered, this, &BabyMEG::UpdateFiffInfo);
    addPluginAction(m_pActionUpdateFiffInfo);

    m_pActionRecordFile = new QAction(QIcon(":/images/record.png"), tr("Start Recording"),this);
//    m_pActionSetupProject->setShortcut(tr("F12"));
    m_pActionRecordFile->setStatusTip(tr("Start Recording"));
    connect(m_pActionRecordFile, &QAction::triggered, this, &BabyMEG::toggleRecordingFile);
    addPluginAction(m_pActionRecordFile);

    //m_pActionRecordFile->setEnabled(false);

    m_pActionSqdCtrl = new QAction(QIcon(":/images/sqdctrl.png"), tr("Squid Control"),this);
//    m_pActionSetupProject->setShortcut(tr("F12"));
    m_pActionSqdCtrl->setStatusTip(tr("Squid Control"));
    connect(m_pActionSqdCtrl, &QAction::triggered, this, &BabyMEG::showSqdCtrlDialog);
    addPluginAction(m_pActionSqdCtrl);

}


//*************************************************************************************************************

BabyMEG::~BabyMEG()
{
    if(this->isRunning())
        stop();

    if(myClient && myClient->isConnected())
            myClient->DisConnectBabyMEG();

}


//*************************************************************************************************************

QSharedPointer<IPlugin> BabyMEG::clone() const
{
    QSharedPointer<BabyMEG> pBabyMEGClone(new BabyMEG());
    return pBabyMEGClone;
}


//*************************************************************************************************************

QString BabyMEG::getFilePath(bool currentTime) const
{
    QString sFilePath = m_sBabyMEGDataPath + "/" + m_sCurrentProject + "/" + m_sCurrentSubject;

    QString sTimeStamp;

    if(currentTime)
        sTimeStamp = QDateTime::currentDateTime().toString("yyMMdd_hhmmss");
    else
        sTimeStamp = "<YYMMDD_HMS>";

    if(m_sCurrentParadigm.isEmpty())
        sFilePath.append("/"+ sTimeStamp + "_" + m_sCurrentSubject + "_raw.fif");
    else
        sFilePath.append("/"+ sTimeStamp + "_" + m_sCurrentSubject + "_" + m_sCurrentParadigm + "_raw.fif");

    return sFilePath;
}


//*************************************************************************************************************

void BabyMEG::init()
{
    //BabyMEGData Path
    m_sBabyMEGDataPath = QDir::homePath() + "/BabyMEGData";
    if(!QDir(m_sBabyMEGDataPath).exists())
        QDir().mkdir(m_sBabyMEGDataPath);
    //Test Project
    if(!QDir(m_sBabyMEGDataPath+"/TestProject").exists())
        QDir().mkdir(m_sBabyMEGDataPath+"/TestProject");
    QSettings settings;
    m_sCurrentProject = settings.value(QString("Plugin/%1/currentProject").arg(getName()), "TestProject").toString();
    //Test Subject
    if(!QDir(m_sBabyMEGDataPath+"/TestProject/TestSubject").exists())
        QDir().mkdir(m_sBabyMEGDataPath+"/TestProject/TestSubject");
    m_sCurrentSubject = settings.value(QString("Plugin/%1/currentSubject").arg(getName()), "TestSubject").toString();

    //BabyMEG Inits
    pInfo = QSharedPointer<BabyMEGInfo>(new BabyMEGInfo());
    connect(pInfo.data(), &BabyMEGInfo::fiffInfoAvailable, this, &BabyMEG::setFiffInfo);
    connect(pInfo.data(), &BabyMEGInfo::SendDataPackage, this, &BabyMEG::setFiffData);
    connect(pInfo.data(), &BabyMEGInfo::SendCMDPackage, this, &BabyMEG::setCMDData);
    connect(pInfo.data(), &BabyMEGInfo::GainInfoUpdate, this, &BabyMEG::setFiffGainInfo);

    myClient = QSharedPointer<BabyMEGClient>(new BabyMEGClient(6340,this));
    myClient->SetInfo(pInfo);
    myClient->start();
    myClientComm = QSharedPointer<BabyMEGClient>(new BabyMEGClient(6341,this));
    myClientComm->SetInfo(pInfo);
    myClientComm->start();

    myClientComm->SendCommandToBabyMEGShortConnection("INFO");

    myClient->ConnectToBabyMEG();

    //init channels when fiff info is available
    connect(this, &BabyMEG::fiffInfoAvailable, this, &BabyMEG::initConnector);

}


//*************************************************************************************************************

void BabyMEG::unload()
{

}


//*************************************************************************************************************
//=============================================================================================================
// Create measurement instances and config them
//=============================================================================================================

void BabyMEG::initConnector()
{
    if(m_pFiffInfo)
    {
        m_pRTMSABabyMEG = PluginOutputData<NewRealTimeMultiSampleArray>::create(this, "BabyMEG Output", "BabyMEG");
        m_pRTMSABabyMEG->data()->setName(this->getName());//Provide name to auto store widget settings

        m_pRTMSABabyMEG->data()->initFromFiffInfo(m_pFiffInfo);
        m_pRTMSABabyMEG->data()->setMultiArraySize(2);

        m_pRTMSABabyMEG->data()->setSamplingRate(m_pFiffInfo->sfreq);

        m_pRTMSABabyMEG->data()->setVisibility(true);

        m_outputConnectors.append(m_pRTMSABabyMEG);
    }
}


//*************************************************************************************************************

void BabyMEG::clear()
{
    m_pFiffInfo.reset();
    m_iBufferSize = -1;
}


//*************************************************************************************************************

void BabyMEG::showProjectDialog()
{
    BabyMEGProjectDialog projectDialog(this);
    projectDialog.exec();
}


//*************************************************************************************************************

void BabyMEG::showSqdCtrlDialog()
{
    //BabyMEGSQUIDControlDgl SQUIDCtrlDlg(this);
    //SQUIDCtrlDlg.exec();
    // added by Limin for nonmodal dialog
    if (SQUIDCtrlDlg == NULL)
        SQUIDCtrlDlg = QSharedPointer<BabyMEGSQUIDControlDgl>(new BabyMEGSQUIDControlDgl(this));

    if (!SQUIDCtrlDlg->isVisible())
    {
        SQUIDCtrlDlg->show();
        SQUIDCtrlDlg->raise();
        SQUIDCtrlDlg->Init();
    }
}

//*************************************************************************************************************

void BabyMEG::UpdateFiffInfo()
{

    // read gain info and save them to the m_pFiffInfo.range
    myClientComm->SendCommandToBabyMEGShortConnection("INFG");

    //sleep(0.5);

    //m_pActionRecordFile->setEnabled(true);

}


//*************************************************************************************************************

void BabyMEG::splitRecordingFile()
{
    qDebug() << "Split recording file";
    ++m_iSplitCount;
    QString nextFileName = m_sRecordFile.remove("_raw.fif");
    nextFileName += QString("-%1_raw.fif").arg(m_iSplitCount);

    /*
    * Write the link to the next file
    */
    qint32 data;
    m_pOutfid->start_block(FIFFB_REF);
    data = FIFFV_ROLE_NEXT_FILE;
    m_pOutfid->write_int(FIFF_REF_ROLE,&data);
    m_pOutfid->write_string(FIFF_REF_FILE_NAME, nextFileName);
    m_pOutfid->write_id(FIFF_REF_FILE_ID);//ToDo meas_id
    data = m_iSplitCount - 1;
    m_pOutfid->write_int(FIFF_REF_FILE_NUM, &data);
    m_pOutfid->end_block(FIFFB_REF);

    //finish file
    m_pOutfid->finish_writing_raw();

    //start next file
    m_qFileOut.setFileName(nextFileName);
    m_pOutfid = Fiff::start_writing_raw(m_qFileOut, *m_pFiffInfo, m_cals);
    fiff_int_t first = 0;
    m_pOutfid->write_int(FIFF_FIRST_SAMPLE, &first);
}


//*************************************************************************************************************

void BabyMEG::toggleRecordingFile()
{
    //Setup writing to file
    if(m_bWriteToFile)
    {
        m_pOutfid->finish_writing_raw();
        m_bWriteToFile = false;
        m_pTimerRecordingChange->stop();
        m_pActionRecordFile->setIcon(QIcon(":/images/record.png"));
        m_iSplitCount = 0;
    }
    else
    {
        m_iSplitCount = 0;

        if(!m_pFiffInfo)
        {
            QMessageBox msgBox;
            msgBox.setText("FiffInfo missing!");
            msgBox.exec();
            return;
        }


        //Initiate the stream for writing to the fif file
        m_sRecordFile = getFilePath(true);
        m_qFileOut.setFileName(m_sRecordFile);
        if(m_qFileOut.exists())
        {
            QMessageBox msgBox;
            msgBox.setText("The file you want to write already exists.");
            msgBox.setInformativeText("Do you want to overwrite this file?");
            msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
            int ret = msgBox.exec();
            if(ret == QMessageBox::No)
                return;
        }

        m_pOutfid = Fiff::start_writing_raw(m_qFileOut, *m_pFiffInfo, m_cals);
        fiff_int_t first = 0;
        m_pOutfid->write_int(FIFF_FIRST_SAMPLE, &first);

        m_bWriteToFile = true;

        m_pTimerRecordingChange = QSharedPointer<QTimer>(new QTimer);
        connect(m_pTimerRecordingChange.data(), &QTimer::timeout, this, &BabyMEG::changeRecordingButton);
        m_pTimerRecordingChange->start(500);
    }
}


//*************************************************************************************************************

void BabyMEG::setFiffData(QByteArray DATA)
{
    //get the first byte -- the data format
    int dformat = DATA.left(1).toInt();

    DATA.remove(0,1);
    qint32 rows = m_pFiffInfo->nchan;
    qint32 cols = (DATA.size()/dformat)/rows;

    qDebug() << "[BabyMEG] Matrix " << rows << "x" << cols << " [Data bytes:" << dformat << "]";

    MatrixXf rawData(Map<MatrixXf>( (float*)DATA.data(),rows, cols ));

    for(qint32 i = 0; i < rows*cols; ++i)
        IOUtils::swap_floatp(rawData.data()+i);


    if(m_bIsRunning)
    {
        if(!m_pRawMatrixBuffer)
            m_pRawMatrixBuffer = CircularMatrixBuffer<float>::SPtr(new CircularMatrixBuffer<float>(40, rows, cols));

        m_pRawMatrixBuffer->push(&rawData);
    }
//    else
//    {
////        std::cout << "Data coming" << std::endl; //"first ten elements \n" << rawData.block(0,0,1,10) << std::endl;

//        emit DataToSquidCtrlGUI(rawData);
//    }

    emit DataToSquidCtrlGUI(rawData);
}


//*************************************************************************************************************

void BabyMEG::setFiffInfo(FiffInfo p_FiffInfo)
{
    m_pFiffInfo = QSharedPointer<FiffInfo>(new FiffInfo(p_FiffInfo));

    m_iBufferSize = pInfo->dataLength;
    sfreq = pInfo->sfreq;

    emit fiffInfoAvailable();
}

//*************************************************************************************************************

void BabyMEG::setFiffGainInfo(QStringList GainInfo)
{
    if(!m_pFiffInfo)
    {
        QMessageBox msgBox;
        msgBox.setText("FiffInfo missing!");
        msgBox.exec();
        return;
    }
    else
    {
        //set up the gain info
        qDebug()<<"Set Gain Info";
        for(qint32 i = 0; i < m_pFiffInfo->nchan; i++)
        {
            m_pFiffInfo->chs[i].range = 1.0f/GainInfo.at(i).toFloat();//1; // set gain
            //qDebug()<<i<<"="<<m_pFiffInfo->chs[i].ch_name<<","<<m_pFiffInfo->chs[i].range;
        }
    }

}
//*************************************************************************************************************

void BabyMEG::setCMDData(QByteArray DATA)
{
    qDebug()<<"------"<<DATA;
//    m_commandManager["FLL"].reply(DATA);
    emit SendCMDDataToSQUIDControl(DATA);
    qDebug()<<"Data has been received.";
}


//*************************************************************************************************************
void BabyMEG::comFLL(QString t_sFLLControlCommand)
{
    qDebug()<<"FLL commands";

    qDebug() << "BabyMeg Received" << t_sFLLControlCommand;
    int strlen = t_sFLLControlCommand.size();
    QByteArray Scmd = myClientComm->MGH_LM_Int2Byte(strlen);
    QByteArray SC = QByteArray("COMS")+Scmd;
    SC.append(t_sFLLControlCommand);
    myClientComm->SendCommandToBabyMEGShortConnection(SC);
}


//*************************************************************************************************************

bool BabyMEG::start()
{
    //Check if the thread is already or still running. This can happen if the start button is pressed immediately after the stop button was pressed. In this case the stopping process is not finished yet but the start process is initiated.
    if(this->isRunning())
        QThread::wait();

    if(!m_pRTMSABabyMEG)
        initConnector();

    // Start threads
    m_bIsRunning = true;

    if(!myClient->isConnected())
        myClient->ConnectToBabyMEG();
    // Start threads
    QThread::start();

    return true;
}


//*************************************************************************************************************

bool BabyMEG::stop()
{
    if(myClient->isConnected())
        myClient->DisConnectBabyMEG();

    m_bIsRunning = false;

    //In case the semaphore blocks the thread -> Release the QSemaphore and let it exit from the pop function (acquire statement)
    m_pRawMatrixBuffer->releaseFromPop();

    //Clear Buffers
    m_pRawMatrixBuffer->clear();

    return true;
}


//*************************************************************************************************************

IPlugin::PluginType BabyMEG::getType() const
{
    return _ISensor;
}


//*************************************************************************************************************

QString BabyMEG::getName() const
{
    return "BabyMEG";
}


//*************************************************************************************************************

QWidget* BabyMEG::setupWidget()
{
    if(!myClient->isConnected())
        myClient->ConnectToBabyMEG();

    BabyMEGSetupWidget* widget = new BabyMEGSetupWidget(this);//widget is later distroyed by CentralWidget - so it has to be created everytime new

    //init dialog

    return widget;
}


//*************************************************************************************************************

MatrixXf BabyMEG::calibrate(const MatrixXf& data)
{
//    if(m_pFiffInfo)
//    {
//        bool projAvailable = true;

//        if (this->proj.size() == 0)
//            projAvailable = false;

//        //
//        //  Initialize the data and calibration vector
//        //
//        qint32 nchan = this->info.nchan;
//        qint32 dest  = 0;//1;
//        qint32 i, k, r;

//        typedef Eigen::Triplet<double> T;
//        std::vector<T> tripletList;
//        tripletList.reserve(nchan);
//        for(i = 0; i < nchan; ++i)
//            tripletList.push_back(T(i, i, this->cals[i]));

//        SparseMatrix<double> cal(nchan, nchan);
//        cal.setFromTriplets(tripletList.begin(), tripletList.end());
//    //    cal.makeCompressed();

//        MatrixXd mult_full;
//        //
//        if (sel.size() == 0)
//        {
//            data = MatrixXd(nchan, to-from+1);
//    //            data->setZero();
//            if (projAvailable || this->comp.kind != -1)
//            {
//                if (!projAvailable)
//                    mult_full = this->comp.data->data*cal;
//                else if (this->comp.kind == -1)
//                    mult_full = this->proj*cal;
//                else
//                    mult_full = this->proj*this->comp.data->data*cal;
//            }
//        }
//        else
//        {
//            data = MatrixXd(sel.size(),to-from+1);
//    //            data->setZero();

//            MatrixXd selVect(sel.size(), nchan);

//            selVect.setZero();

//            if (!projAvailable && this->comp.kind == -1)
//            {
//                tripletList.clear();
//                tripletList.reserve(sel.size());
//                for(i = 0; i < sel.size(); ++i)
//                    tripletList.push_back(T(i, i, this->cals[sel[i]]));
//                cal = SparseMatrix<double>(sel.size(), sel.size());
//                cal.setFromTriplets(tripletList.begin(), tripletList.end());
//            }
//            else
//            {
//                if (!projAvailable)
//                {
//                    qDebug() << "This has to be debugged! #1";
//                    for( i = 0; i  < sel.size(); ++i)
//                        selVect.row(i) = this->comp.data->data.block(sel[i],0,1,nchan);
//                    mult_full = selVect*cal;
//                }
//                else if (this->comp.kind == -1)
//                {
//                    for( i = 0; i  < sel.size(); ++i)
//                        selVect.row(i) = this->proj.block(sel[i],0,1,nchan);

//                    mult_full = selVect*cal;
//                }
//                else
//                {
//                    qDebug() << "This has to be debugged! #3";
//                    for( i = 0; i  < sel.size(); ++i)
//                        selVect.row(i) = this->proj.block(sel[i],0,1,nchan);

//                    mult_full = selVect*this->comp.data->data*cal;
//                }
//            }
//        }

//        bool do_debug = false;
//        //
//        // Make mult sparse
//        //
//        tripletList.clear();
//        tripletList.reserve(mult_full.rows()*mult_full.cols());
//        for(i = 0; i < mult_full.rows(); ++i)
//            for(k = 0; k < mult_full.cols(); ++k)
//                if(mult_full(i,k) != 0)
//                    tripletList.push_back(T(i, k, mult_full(i,k)));

//        SparseMatrix<double> mult(mult_full.rows(),mult_full.cols());
//        if(tripletList.size() > 0)
//            mult.setFromTriplets(tripletList.begin(), tripletList.end());
//    //    mult.makeCompressed();

//        //

//        FiffStream::SPtr fid;
//        if (!this->file->device()->isOpen())
//        {
//            if (!this->file->device()->open(QIODevice::ReadOnly))
//            {
//                printf("Cannot open file %s",this->info.filename.toUtf8().constData());
//            }
//            fid = this->file;
//        }
//        else
//        {
//            fid = this->file;
//        }

//        MatrixXd one;
//        fiff_int_t first_pick, last_pick, picksamp;
//        for(k = 0; k < this->rawdir.size(); ++k)
//        {
//            FiffRawDir thisRawDir = this->rawdir[k];
//            //
//            //  Do we need this buffer
//            //
//            if (thisRawDir.last > from)
//            {
//                if (thisRawDir.ent.kind == -1)
//                {
//                    //
//                    //  Take the easy route: skip is translated to zeros
//                    //
//                    if(do_debug)
//                        printf("S");
//                    if (sel.cols() <= 0)
//                        one.resize(nchan,thisRawDir.nsamp);
//                    else
//                        one.resize(sel.cols(),thisRawDir.nsamp);

//                    one.setZero();
//                }
//                else
//                {
//                    FiffTag::SPtr t_pTag;
//                    FiffTag::read_tag(fid.data(), t_pTag, thisRawDir.ent.pos);
//                    //
//                    //   Depending on the state of the projection and selection
//                    //   we proceed a little bit differently
//                    //
//                    if (mult.cols() == 0)
//                    {
//                        if (sel.cols() == 0)
//                        {
//                            if (t_pTag->type == FIFFT_DAU_PACK16)
//                                one = cal*(Map< MatrixDau16 >( t_pTag->toDauPack16(),nchan, thisRawDir.nsamp)).cast<double>();
//                            else if(t_pTag->type == FIFFT_INT)
//                                one = cal*(Map< MatrixXi >( t_pTag->toInt(),nchan, thisRawDir.nsamp)).cast<double>();
//                            else if(t_pTag->type == FIFFT_FLOAT)
//                                one = cal*(Map< MatrixXf >( t_pTag->toFloat(),nchan, thisRawDir.nsamp)).cast<double>();
//                            else
//                                printf("Data Storage Format not known jet [1]!! Type: %d\n", t_pTag->type);
//                        }
//                        else
//                        {

//                            //ToDo find a faster solution for this!! --> make cal and mul sparse like in MATLAB
//                            MatrixXd newData(sel.cols(), thisRawDir.nsamp); //ToDo this can be done much faster, without newData

//                            if (t_pTag->type == FIFFT_DAU_PACK16)
//                            {
//                                MatrixXd tmp_data = (Map< MatrixDau16 > ( t_pTag->toDauPack16(),nchan, thisRawDir.nsamp)).cast<double>();

//                                for(r = 0; r < sel.size(); ++r)
//                                    newData.block(r,0,1,thisRawDir.nsamp) = tmp_data.block(sel[r],0,1,thisRawDir.nsamp);
//                            }
//                            else if(t_pTag->type == FIFFT_INT)
//                            {
//                                MatrixXd tmp_data = (Map< MatrixXi >( t_pTag->toInt(),nchan, thisRawDir.nsamp)).cast<double>();

//                                for(r = 0; r < sel.size(); ++r)
//                                    newData.block(r,0,1,thisRawDir.nsamp) = tmp_data.block(sel[r],0,1,thisRawDir.nsamp);
//                            }
//                            else if(t_pTag->type == FIFFT_FLOAT)
//                            {
//                                MatrixXd tmp_data = (Map< MatrixXf > ( t_pTag->toFloat(),nchan, thisRawDir.nsamp)).cast<double>();

//                                for(r = 0; r < sel.size(); ++r)
//                                    newData.block(r,0,1,thisRawDir.nsamp) = tmp_data.block(sel[r],0,1,thisRawDir.nsamp);
//                            }
//                            else
//                            {
//                                printf("Data Storage Format not known jet [2]!! Type: %d\n", t_pTag->type);
//                            }

//                            one = cal*newData;
//                        }
//                    }
//                    else
//                    {
//                        if (t_pTag->type == FIFFT_DAU_PACK16)
//                            one = mult*(Map< MatrixDau16 >( t_pTag->toDauPack16(),nchan, thisRawDir.nsamp)).cast<double>();
//                        else if(t_pTag->type == FIFFT_INT)
//                            one = mult*(Map< MatrixXi >( t_pTag->toInt(),nchan, thisRawDir.nsamp)).cast<double>();
//                        else if(t_pTag->type == FIFFT_FLOAT)
//                            one = mult*(Map< MatrixXf >( t_pTag->toFloat(),nchan, thisRawDir.nsamp)).cast<double>();
//                        else
//                            printf("Data Storage Format not known jet [3]!! Type: %d\n", t_pTag->type);
//                    }
//                }
//                //
//                //  The picking logic is a bit complicated
//                //
//                if (to >= thisRawDir.last && from <= thisRawDir.first)
//                {
//                    //
//                    //  We need the whole buffer
//                    //
//                    first_pick = 0;//1;
//                    last_pick  = thisRawDir.nsamp - 1;
//                    if (do_debug)
//                        printf("W");
//                }
//                else if (from > thisRawDir.first)
//                {
//                    first_pick = from - thisRawDir.first;// + 1;
//                    if(to < thisRawDir.last)
//                    {
//                        //
//                        //  Something from the middle
//                        //
//    //                    qDebug() << "This needs to be debugged!";
//                        last_pick = thisRawDir.nsamp + to - thisRawDir.last - 1;//is this alright?
//                        if (do_debug)
//                            printf("M");
//                    }
//                    else
//                    {
//                        //
//                        //  From the middle to the end
//                        //
//                        last_pick = thisRawDir.nsamp - 1;
//                        if (do_debug)
//                            printf("E");
//                    }
//                }
//                else
//                {
//                    //
//                    //  From the beginning to the middle
//                    //
//                    first_pick = 0;//1;
//                    last_pick  = to - thisRawDir.first;// + 1;
//                    if (do_debug)
//                        printf("B");
//                }
//                //
//                //  Now we are ready to pick
//                //
//                picksamp = last_pick - first_pick + 1;

//                if(do_debug)
//                {
//                    qDebug() << "first_pick: " << first_pick;
//                    qDebug() << "last_pick: " << last_pick;
//                    qDebug() << "picksamp: " << picksamp;
//                }

//                if (picksamp > 0)
//                {
//    //                    for(r = 0; r < data->rows(); ++r)
//    //                        for(c = 0; c < picksamp; ++c)
//    //                            (*data)(r,dest + c) = one(r,first_pick + c);
//                    data.block(0,dest,data.rows(),picksamp) = one.block(0, first_pick, data.rows(), picksamp);

//                    dest += picksamp;
//                }
//            }
//            //
//            //  Done?
//            //
//            if (thisRawDir.last >= to)
//            {
//                printf(" [done]\n");
//                break;
//            }
//        }

//    //        fclose(fid);

//        times = MatrixXd(1, to-from+1);

//        for (i = 0; i < times.cols(); ++i)
//            times(0, i) = ((float)(from+i)) / this->info.sfreq;

//    }
    return data;
}


//*************************************************************************************************************

void BabyMEG::run()
{

    MatrixXf matValue;

    qint32 size = 0;

    while(m_bIsRunning)
    {
        if(m_pRawMatrixBuffer)
        {
            //pop matrix
            matValue = m_pRawMatrixBuffer->pop();

            //Write raw data to fif file
            if(m_bWriteToFile)
            {
                size += matValue.rows()*matValue.cols() * 4;

                if(size > MAX_DATA_LEN)
                {
                    size = 0;
                    this->splitRecordingFile();
                }

                m_pOutfid->write_raw_buffer(matValue.cast<double>());
            }
            else
                size = 0;

            if(m_pRTMSABabyMEG)
            {
                //std::cout << "matValue" << matValue.block(0,0,2,2) << std::endl;
                //emit values
//                for(qint32 i = 0; i < matValue.cols(); ++i)
//                    m_pRTMSABabyMEG->data()->setValue(matValue.col(i).cast<double>());
                m_pRTMSABabyMEG->data()->setValue(matValue.cast<double>());
            }
        }
    }

    //Close the fif output stream
    if(m_bWriteToFile)
        this->toggleRecordingFile();
}


//*************************************************************************************************************

void BabyMEG::changeRecordingButton()
{
    if(m_iBlinkStatus == 0)
    {
        m_pActionRecordFile->setIcon(QIcon(":/images/record.png"));
        m_iBlinkStatus = 1;
    }
    else
    {
        m_pActionRecordFile->setIcon(QIcon(":/images/record_active.png"));
        m_iBlinkStatus = 0;
    }
}
