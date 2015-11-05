//=============================================================================================================
/**
* @file     gusbamp.cpp
* @author   Viktor Klüber <viktor.klueber@tu-ilmenau.de>;
*           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
* @version  1.0
* @date     November, 2015
*
* @section  LICENSE
*
* Copyright (C) 2015, Viktor Klüber, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
* @brief    Contains the implementation of the GUSBAmp class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "gusbamp.h"
#include "gusbampproducer.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore/QtPlugin>
#include <QtCore/QTextStream>
#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace GUSBAmpPlugin;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

GUSBAmp::GUSBAmp()
: m_pRMTSA_GUSBAmp(0)
, m_qStringResourcePath(qApp->applicationDirPath()+"/mne_x_plugins/resources/gusbamp/")
, m_pRawMatrixBuffer_In(0)
, m_pGUSBAmpProducer(new GUSBAmpProducer(this))
{
    // Create record file option action bar item/button
    m_pActionSetupProject = new QAction(QIcon(":/images/database.png"), tr("Setup project"), this);
    m_pActionSetupProject->setStatusTip(tr("Setup project"));
    connect(m_pActionSetupProject, &QAction::triggered, this, &GUSBAmp::showSetupProjectDialog);
    addPluginAction(m_pActionSetupProject);

    // Create start recordin action bar item/button
    m_pActionStartRecording = new QAction(QIcon(":/images/record.png"), tr("Start recording data to fif file"), this);
    m_pActionStartRecording->setStatusTip(tr("Start recording data to fif file"));
    connect(m_pActionStartRecording, &QAction::triggered, this, &GUSBAmp::showStartRecording);
    addPluginAction(m_pActionStartRecording);
}


//*************************************************************************************************************

GUSBAmp::~GUSBAmp()
{
    //std::cout << "GUSBAmp::~GUSBAmp() " << std::endl;

    //If the program is closed while the sampling is in process
    if(this->isRunning())
        this->stop();
}


//*************************************************************************************************************

QSharedPointer<IPlugin> GUSBAmp::clone() const
{
    QSharedPointer<GUSBAmp> pGUSBAmpClone(new GUSBAmp());
    return pGUSBAmpClone;
}


//*************************************************************************************************************

void GUSBAmp::init()
{
    m_pRMTSA_GUSBAmp = PluginOutputData<NewRealTimeMultiSampleArray>::create(this, "GUSBAmp", "EEG output data");

    m_outputConnectors.append(m_pRMTSA_GUSBAmp);

    //default values used by the setupGUI class must be set here
    m_iSamplingFreq = 1024;
    m_iNumberOfChannels = 138;
    m_iSamplesPerBlock = 16;
    m_iTriggerInterval = 5000;
    m_iSplitFileSizeMs = 10;
    m_iSplitCount = 0;

    m_bUseChExponent = true;
    m_bUseUnitGain = true;
    m_bUseUnitOffset = true;
    m_bWriteToFile = false;
    m_bWriteDriverDebugToFile = false;
    m_bIsRunning = false;
    m_bBeepTrigger = false;
    m_bSplitFile = false;

    m_iTriggerType = 0;

    QDate date;
    m_sOutputFilePath = QString ("%1Sequence_01/Subject_01/%2_%3_%4_EEG_001_raw.fif").arg(m_qStringResourcePath).arg(date.currentDate().year()).arg(date.currentDate().month()).arg(date.currentDate().day());

    m_sElcFilePath = QString("./mne_x_plugins/resources/GUSBAmp/loc_files/Lorenz-Duke128-28-11-2013.elc");

    m_pFiffInfo = QSharedPointer<FiffInfo>(new FiffInfo());
}


//*************************************************************************************************************

void GUSBAmp::unload()
{

}


//*************************************************************************************************************

void GUSBAmp::setUpFiffInfo()
{
    // Only works for ANT Neuro Waveguard Duke caps
    //
    //Clear old fiff info data
    //
    m_pFiffInfo->clear();

    //
    //Set number of channels, sampling frequency and high/-lowpass
    //
    m_pFiffInfo->nchan = m_iNumberOfChannels;
    m_pFiffInfo->sfreq = m_iSamplingFreq;
    m_pFiffInfo->highpass = (float)0.001;
    m_pFiffInfo->lowpass = m_iSamplingFreq/2;

    //
    //Read electrode positions from .elc file
    //
    QVector< QVector<double> > elcLocation3D;
    QVector< QVector<double> > elcLocation2D;
    QString unit;
    QStringList elcChannelNames;

    if(!LayoutLoader::readAsaElcFile(m_sElcFilePath, elcChannelNames, elcLocation3D, elcLocation2D, unit))
        qDebug() << "Error: Reading elc file.";

    //qDebug() << elcLocation3D;
    //qDebug() << elcLocation2D;
    //qDebug() << elcChannelNames;

    //The positions read from the asa elc file do not correspond to a RAS coordinate system - use a simple 90° z transformation to fix this
    Matrix3f rotation_z;
    rotation_z = AngleAxisf((float)M_PI/2, Vector3f::UnitZ()); //M_PI/2 = 90°
    QVector3D center_pos;

    for(int i = 0; i<elcLocation3D.size(); i++)
    {
        Vector3f point;
        point << elcLocation3D[i][0], elcLocation3D[i][1] , elcLocation3D[i][2];
        Vector3f point_rot = rotation_z * point;
//        cout<<"point: "<<endl<<point<<endl<<endl;
//        cout<<"matrix: "<<endl<<rotation_z<<endl<<endl;
//        cout<<"point_rot: "<<endl<<point_rot<<endl<<endl;
//        cout<<"-----------------------------"<<endl;
        elcLocation3D[i][0] = point_rot[0];
        elcLocation3D[i][1] = point_rot[1];
        elcLocation3D[i][2] = point_rot[2];

        //Also calculate the center position of the electrode positions in this for routine
        center_pos.setX(center_pos.x() + elcLocation3D[i][0]);
        center_pos.setY(center_pos.y() + elcLocation3D[i][1]);
        center_pos.setZ(center_pos.z() + elcLocation3D[i][2]);
    }

    center_pos.setX(center_pos.x()/elcLocation3D.size());
    center_pos.setY(center_pos.y()/elcLocation3D.size());
    center_pos.setZ(center_pos.z()/elcLocation3D.size());

    //
    //Write electrode positions to the digitizer info in the fiffinfo
    //
    QList<FiffDigPoint> digitizerInfo;

    //Only write the EEG channel positions to the fiff info. The Refa devices have next to the EEG input channels 10 other input channels (Bipolar, Auxilary, Digital, Test)
    int numberEEGCh;
    if(m_iNumberOfChannels>128)
        numberEEGCh = 138 - (m_iNumberOfChannels-128);
    else
        numberEEGCh = m_iNumberOfChannels;

    //Check if channel size by user corresponds with read channel informations from the elc file. If not append zeros and string 'unknown' until the size matches.
    if(numberEEGCh > elcLocation3D.size())
    {
        qDebug()<<"Warning: setUpFiffInfo() - Not enough positions read from the elc file. Filling missing channel names and positions with zeroes and 'unknown' strings.";
        QVector<double> tempA(3, 0.0);
        QVector<double> tempB(2, 0.0);
        int size = numberEEGCh-elcLocation3D.size();
        for(int i = 0; i<size; i++)
        {
            elcLocation3D.push_back(tempA);
            elcLocation2D.push_back(tempB);
            elcChannelNames.append(QString("Unknown"));
        }
    }

    //Append LAP value to digitizer data. Take location of LE2 electrode minus 1 cm as approximation.
    FiffDigPoint digPoint;
    int indexLE2 = elcChannelNames.indexOf("LE2");
    digPoint.kind = FIFFV_POINT_CARDINAL;
    digPoint.ident = FIFFV_POINT_LPA;//digitizerInfo.size();

    //Set EEG electrode location - Convert from mm to m
    if(indexLE2!=-1)
    {
        digPoint.r[0] = elcLocation3D[indexLE2][0]*0.001;
        digPoint.r[1] = elcLocation3D[indexLE2][1]*0.001;
        digPoint.r[2] = (elcLocation3D[indexLE2][2]-10)*0.001;
        digitizerInfo.push_back(digPoint);
    }
    else
        cout<<"Plugin GUSBAmp - ERROR - LE2 not found. Check loaded layout."<<endl;

    //Append nasion value to digitizer data. Take location of Z1 electrode minus 6 cm as approximation.
    int indexZ1 = elcChannelNames.indexOf("Z1");
    digPoint.kind = FIFFV_POINT_CARDINAL;//FIFFV_POINT_NASION;
    digPoint.ident = FIFFV_POINT_NASION;//digitizerInfo.size();

    //Set EEG electrode location - Convert from mm to m
    if(indexZ1!=-1)
    {
        digPoint.r[0] = elcLocation3D[indexZ1][0]*0.001;
        digPoint.r[1] = elcLocation3D[indexZ1][1]*0.001;
        digPoint.r[2] = (elcLocation3D[indexZ1][2]-60)*0.001;
        digitizerInfo.push_back(digPoint);
    }
    else
        cout<<"Plugin GUSBAmp - ERROR - Z1 not found. Check loaded layout."<<endl;

    //Append RAP value to digitizer data. Take location of RE2 electrode minus 1 cm as approximation.
    int indexRE2 = elcChannelNames.indexOf("RE2");
    digPoint.kind = FIFFV_POINT_CARDINAL;
    digPoint.ident = FIFFV_POINT_RPA;//digitizerInfo.size();

    //Set EEG electrode location - Convert from mm to m
    if(indexRE2!=-1)
    {
        digPoint.r[0] = elcLocation3D[indexRE2][0]*0.001;
        digPoint.r[1] = elcLocation3D[indexRE2][1]*0.001;
        digPoint.r[2] = (elcLocation3D[indexRE2][2]-10)*0.001;
        digitizerInfo.push_back(digPoint);
    }
    else
        cout<<"Plugin GUSBAmp - ERROR - RE2 not found. Check loaded layout."<<endl;

    //Add EEG electrode positions as digitizers
    for(int i=0; i<numberEEGCh; i++)
    {
        FiffDigPoint digPoint;
        digPoint.kind = FIFFV_POINT_EEG;
        digPoint.ident = i;

        //Set EEG electrode location - Convert from mm to m
        digPoint.r[0] = elcLocation3D[i][0]*0.001;
        digPoint.r[1] = elcLocation3D[i][1]*0.001;
        digPoint.r[2] = elcLocation3D[i][2]*0.001;
        digitizerInfo.push_back(digPoint);
    }

    //Set the final digitizer values to the fiff info
    m_pFiffInfo->dig = digitizerInfo;

    //
    //Set up the channel info
    //
    QStringList QSLChNames;
    m_pFiffInfo->chs.clear();

    for(int i=0; i<m_iNumberOfChannels; i++)
    {
        //Create information for each channel
        QString sChType;
        FiffChInfo fChInfo;

        //EEG Channels
        if(i<=numberEEGCh-1)
        {
            //Set channel name
            //fChInfo.ch_name = elcChannelNames.at(i);
            sChType = QString("EEG ");
            if(i<10)
                sChType.append("00");

            if(i>=10 && i<100)
                sChType.append("0");

            fChInfo.ch_name = sChType.append(sChType.number(i));

            //Set channel type
            fChInfo.kind = FIFFV_EEG_CH;

            //Set coil type
            fChInfo.coil_type = FIFFV_COIL_EEG;

            //Set logno
            fChInfo.logno = i;

            //Set coord frame
            fChInfo.coord_frame = FIFFV_COORD_HEAD;

            //Set unit
            fChInfo.unit = FIFF_UNIT_V;
            fChInfo.unit_mul = 0;

            //Set EEG electrode location - Convert from mm to m
            fChInfo.eeg_loc(0,0) = elcLocation3D[i][0]*0.001;
            fChInfo.eeg_loc(1,0) = elcLocation3D[i][1]*0.001;
            fChInfo.eeg_loc(2,0) = elcLocation3D[i][2]*0.001;

            //Set EEG electrode direction - Convert from mm to m
            fChInfo.eeg_loc(0,1) = center_pos.x()*0.001;
            fChInfo.eeg_loc(1,1) = center_pos.y()*0.001;
            fChInfo.eeg_loc(2,1) = center_pos.z()*0.001;

            //Also write the eeg electrode locations into the meg loc variable (mne_ex_read_raw() matlab function wants this)
            fChInfo.loc(0,0) = elcLocation3D[i][0]*0.001;
            fChInfo.loc(1,0) = elcLocation3D[i][1]*0.001;
            fChInfo.loc(2,0) = elcLocation3D[i][2]*0.001;

            fChInfo.loc(3,0) = center_pos.x()*0.001;
            fChInfo.loc(4,0) = center_pos.y()*0.001;
            fChInfo.loc(5,0) = center_pos.z()*0.001;

            fChInfo.loc(6,0) = 0;
            fChInfo.loc(7,0) = 1;
            fChInfo.loc(8,0) = 0;

            fChInfo.loc(9,0) = 0;
            fChInfo.loc(10,0) = 0;
            fChInfo.loc(11,0) = 1;

            //cout<<i<<endl<<fChInfo.eeg_loc<<endl;
        }

        //Bipolar channels
        if(i>=128 && i<=131)
        {
            //Set channel type
            fChInfo.kind = FIFFV_MISC_CH;

            sChType = QString("BIPO ");
            fChInfo.ch_name = sChType.append(sChType.number(i-128));
        }

        //Auxilary input channels
        if(i>=132 && i<=135)
        {
            //Set channel type
            fChInfo.kind = FIFFV_MISC_CH;

            sChType = QString("AUX ");
            fChInfo.ch_name = sChType.append(sChType.number(i-132));
        }

        //Digital input channel
        if(i==136)
        {
            //Set channel type
            fChInfo.kind = FIFFV_STIM_CH;

            sChType = QString("STI 014");
            fChInfo.ch_name = sChType;
        }

        //Internally generated test signal - ramp signal
        if(i==137)
        {
            //Set channel type
            fChInfo.kind = FIFFV_MISC_CH;

            sChType = QString("TEST RAMP");
            fChInfo.ch_name = sChType;
        }

        QSLChNames << sChType;

        m_pFiffInfo->chs.append(fChInfo);
    }

    //Set channel names in fiff_info_base
    m_pFiffInfo->ch_names = QSLChNames;

    //
    //Set head projection
    //
    m_pFiffInfo->dev_head_t.from = FIFFV_COORD_DEVICE;
    m_pFiffInfo->dev_head_t.to = FIFFV_COORD_HEAD;
    m_pFiffInfo->ctf_head_t.from = FIFFV_COORD_DEVICE;
    m_pFiffInfo->ctf_head_t.to = FIFFV_COORD_HEAD;

    //
    //Set projection data
    //
    m_pFiffInfo->projs.clear();
    FiffProj proj;
    proj.kind = 1;
    proj.active = false;

    FiffNamedMatrix::SDPtr namedMatrix = proj.data;
    namedMatrix->ncol = numberEEGCh/3;
    namedMatrix->nrow = 1;
    namedMatrix->data = MatrixXd::Ones(1, namedMatrix->ncol);

    //Set projection 1
    for(int i=0; i<namedMatrix->ncol; i++)
        namedMatrix->col_names << QSLChNames.at(i);

    proj.data = namedMatrix;
    proj.desc = QString("PCA-v1");
    m_pFiffInfo->projs.append(proj);

    //Set projection 2
    namedMatrix->col_names.clear();
    for(int i=0; i<namedMatrix->ncol; i++)
        namedMatrix->col_names << QSLChNames.at(i+namedMatrix->ncol);

    proj.data = namedMatrix;
    proj.desc = QString("PCA-v2");
    m_pFiffInfo->projs.append(proj);

    //Set projection 3
    namedMatrix->col_names.clear();
    for(int i=0; i<namedMatrix->ncol; i++)
        namedMatrix->col_names << QSLChNames.at(i+(2*namedMatrix->ncol));

    proj.data = namedMatrix;
    proj.desc = QString("PCA-v3");
    m_pFiffInfo->projs.append(proj);
}


//*************************************************************************************************************

bool GUSBAmp::start()
{
    //Check if the thread is already or still running. This can happen if the start button is pressed immediately after the stop button was pressed. In this case the stopping process is not finished yet but the start process is initiated.
    if(this->isRunning())
        QThread::wait();

    if(m_bBeepTrigger)
        m_qTimerTrigger.start();

    //Setup fiff info
    setUpFiffInfo();

    //Set the channel size of the RMTSA - this needs to be done here and NOT in the init() function because the user can change the number of channels during runtime
    m_pRMTSA_GUSBAmp->data()->initFromFiffInfo(m_pFiffInfo);
    m_pRMTSA_GUSBAmp->data()->setMultiArraySize(m_iSamplesPerBlock);
    m_pRMTSA_GUSBAmp->data()->setSamplingRate(m_iSamplingFreq);

    //Buffer
    m_pRawMatrixBuffer_In = QSharedPointer<RawMatrixBuffer>(new RawMatrixBuffer(8, m_iNumberOfChannels, m_iSamplesPerBlock));

    m_pGUSBAmpProducer->start(m_iNumberOfChannels,
                       m_iSamplingFreq,
                       m_iSamplesPerBlock,
                       m_bUseChExponent,
                       m_bUseUnitGain,
                       m_bUseUnitOffset,
                       m_bWriteDriverDebugToFile,
                       m_sOutputFilePath);

    if(m_pGUSBAmpProducer->isRunning())
    {
        m_bIsRunning = true;
        QThread::start();
        return true;
    }
    else
    {
        qWarning() << "Plugin GUSBAmp - ERROR - GUSBAmpProducer thread could not be started - Either the device is turned off (check your OS device manager) or the driver DLL (GUSBAmpSDK.dll / GUSBAmpSDK32bit.dll) is not installed in the system32 / SysWOW64 directory" << endl;
        return false;
    }
}


//*************************************************************************************************************

bool GUSBAmp::stop()
{
    //Stop the producer thread first
    m_pGUSBAmpProducer->stop();

    //Wait until this thread (GUSBAmp) is stopped
    m_bIsRunning = false;

    //In case the semaphore blocks the thread -> Release the QSemaphore and let it exit from the pop function (acquire statement)
    m_pRawMatrixBuffer_In->releaseFromPop();

    m_pRawMatrixBuffer_In->clear();

    m_pRMTSA_GUSBAmp->data()->clear();

    return true;
}


//*************************************************************************************************************

IPlugin::PluginType GUSBAmp::getType() const
{
    return _ISensor;
}


//*************************************************************************************************************

QString GUSBAmp::getName() const
{
    return "GUSBAmp EEG";
}


//*************************************************************************************************************

QWidget* GUSBAmp::setupWidget()
{
    GUSBAmpSetupWidget* widget = new GUSBAmpSetupWidget(this);//widget is later destroyed by CentralWidget - so it has to be created everytime new

    //init properties dialog
    widget->initGui();

    return widget;
}


//*************************************************************************************************************

void GUSBAmp::splitRecordingFile()
{
    qDebug() << "Split recording file";
    ++m_iSplitCount;
    QString nextFileName = m_sOutputFilePath.remove("_raw.fif");
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
    m_fileOut.setFileName(nextFileName);
    m_pOutfid = Fiff::start_writing_raw(m_fileOut, *m_pFiffInfo, m_cals);
    fiff_int_t first = 0;
    m_pOutfid->write_int(FIFF_FIRST_SAMPLE, &first);
}


//*************************************************************************************************************

void GUSBAmp::run()
{
    qint32 size = 0;

    while(m_bIsRunning)
    {
        //std::cout<<"GUSBAmp::run(s)"<<std::endl;

        //pop matrix only if the producer thread is running
        if(m_pGUSBAmpProducer->isRunning())
        {
            MatrixXf matValue = m_pRawMatrixBuffer_In->pop();

            // Set Beep trigger (if activated)
            if(m_bBeepTrigger && m_qTimerTrigger.elapsed() >= m_iTriggerInterval)
            {
                QtConcurrent::run(Beep, 450, 700);
                //Set trigger in received data samples - just for one sample, so that this event is easy to detect
                matValue(136, m_iSamplesPerBlock-1) = 252;
                m_qTimerTrigger.restart();
            }

            //Write raw data to fif file
            if(m_bWriteToFile) {
                m_pOutfid->write_raw_buffer(matValue.cast<double>(), m_cals);
                size += matValue.cols();

//                qDebug()<<"size"<<size;
//                qDebug()<<"(m_iSplitFileSizeMs/1000)*m_pFiffInfo->sfreq"<<(double(m_iSplitFileSizeMs)/1000.0)*m_pFiffInfo->sfreq;
                if(size > (double(m_iSplitFileSizeMs)/1000.0)*m_pFiffInfo->sfreq && m_bSplitFile) {
                    size = 0;
                    splitRecordingFile();
                }
            } else
                size = 0;

            //Change values of the trigger channel for better plotting - this change is not saved in the produced fif file
            if(m_iNumberOfChannels>137)
            {
                for(int i = 0; i<matValue.row(137).cols(); i++)
                {
                    // Left keyboard or capacitive
                    if(matValue.row(136)[i] == 254)
                        matValue.row(136)[i] = 4000;

                    // Right keyboard
                    if(matValue.row(136)[i] == 253)
                        matValue.row(136)[i] = 8000;

                    // Beep
                    if(matValue.row(136)[i] == 252)
                        matValue.row(136)[i] = 2000;
                }
            }

            //emit values to real time multi sample array
            m_pRMTSA_GUSBAmp->data()->setValue(matValue.cast<double>());

            // Reset keyboard trigger
            m_iTriggerType = 0;
        }
    }

    //Close the fif output stream
    if(m_bWriteToFile)
    {
        m_pOutfid->finish_writing_raw();
        m_bWriteToFile = false;
        m_pTimerRecordingChange->stop();
        m_pActionStartRecording->setIcon(QIcon(":/images/record.png"));
    }

    //std::cout<<"EXITING - GUSBAmp::run()"<<std::endl;
}


//*************************************************************************************************************

void GUSBAmp::showSetupProjectDialog()
{
    // Open setup project widget
    if(m_pGUSBAmpSetupProjectWidget == NULL)
        m_pGUSBAmpSetupProjectWidget = QSharedPointer<GUSBAmpSetupProjectWidget>(new GUSBAmpSetupProjectWidget(this));

    if(!m_pGUSBAmpSetupProjectWidget->isVisible())
    {
        m_pGUSBAmpSetupProjectWidget->setWindowTitle("GUSBAmp EEG Connector - Setup project");
        m_pGUSBAmpSetupProjectWidget->initGui();
        m_pGUSBAmpSetupProjectWidget->show();
        m_pGUSBAmpSetupProjectWidget->raise();
    }
}

//*************************************************************************************************************

void GUSBAmp::showStartRecording()
{
    m_iSplitCount = 0;

    //Setup writing to file
    if(m_bWriteToFile)
    {
        m_pOutfid->finish_writing_raw();
        m_bWriteToFile = false;
        m_pTimerRecordingChange->stop();
        m_pActionStartRecording->setIcon(QIcon(":/images/record.png"));
    }
    else
    {
        if(!m_bIsRunning)
        {
            QMessageBox msgBox;
            msgBox.setText("Start data acquisition first!");
            msgBox.exec();
            return;
        }

        if(!m_pFiffInfo)
        {
            QMessageBox msgBox;
            msgBox.setText("FiffInfo missing!");
            msgBox.exec();
            return;
        }

        //Initiate the stream for writing to the fif file
        m_fileOut.setFileName(m_sOutputFilePath);
        if(m_fileOut.exists())
        {
            QMessageBox msgBox;
            msgBox.setText("The file you want to write already exists.");
            msgBox.setInformativeText("Do you want to overwrite this file?");
            msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
            int ret = msgBox.exec();
            if(ret == QMessageBox::No)
                return;
        }

        // Check if path exists -> otherwise create it
        QStringList list = m_sOutputFilePath.split("/");
        list.removeLast(); // remove file name
        QString fileDir = list.join("/");

        if(!dirExists(fileDir.toStdString()))
        {
            QDir dir;
            dir.mkpath(fileDir);
        }

        m_pOutfid = Fiff::start_writing_raw(m_fileOut, *m_pFiffInfo, m_cals);
        fiff_int_t first = 0;
        m_pOutfid->write_int(FIFF_FIRST_SAMPLE, &first);

        m_bWriteToFile = true;

        m_pTimerRecordingChange = QSharedPointer<QTimer>(new QTimer);
        connect(m_pTimerRecordingChange.data(), &QTimer::timeout, this, &GUSBAmp::changeRecordingButton);
        m_pTimerRecordingChange->start(500);
    }
}


//*************************************************************************************************************

void GUSBAmp::changeRecordingButton()
{
    if(m_iBlinkStatus == 0)
    {
        m_pActionStartRecording->setIcon(QIcon(":/images/record.png"));
        m_iBlinkStatus = 1;
    }
    else
    {
        m_pActionStartRecording->setIcon(QIcon(":/images/record_active.png"));
        m_iBlinkStatus = 0;
    }
}


//*************************************************************************************************************

bool GUSBAmp::dirExists(const std::string& dirName_in)
{
    DWORD ftyp = GetFileAttributesA(dirName_in.c_str());
    if (ftyp == INVALID_FILE_ATTRIBUTES)
        return false;  //something is wrong with your path!

    if (ftyp & FILE_ATTRIBUTE_DIRECTORY)
        return true;   // this is a directory!

    return false;    // this is not a directory!
}


