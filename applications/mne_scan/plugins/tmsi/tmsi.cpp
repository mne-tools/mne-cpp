//=============================================================================================================
/**
* @file     tmsi.cpp
* @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     September, 2013
*
* @section  LICENSE
*
* Copyright (C) 2013, Lorenz Esch, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Contains the implementation of the TMSI class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "tmsi.h"
#include "tmsiproducer.h"


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

using namespace TMSIPlugin;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

TMSI::TMSI()
: m_pRMTSA_TMSI(0)
, m_qStringResourcePath(qApp->applicationDirPath()+"/resources/mne_scan/plugins/tmsi/")
, m_pRawMatrixBuffer_In(0)
, m_pTMSIProducer(new TMSIProducer(this))
{
    // Create record file option action bar item/button
    m_pActionSetupProject = new QAction(QIcon(":/images/database.png"), tr("Setup project"), this);
    m_pActionSetupProject->setStatusTip(tr("Setup project"));
    connect(m_pActionSetupProject, &QAction::triggered, this, &TMSI::showSetupProjectDialog);
    addPluginAction(m_pActionSetupProject);

    // Create start recordin action bar item/button
    m_pActionStartRecording = new QAction(QIcon(":/images/record.png"), tr("Start recording data to fif file"), this);
    m_pActionStartRecording->setStatusTip(tr("Start recording data to fif file"));
    connect(m_pActionStartRecording, &QAction::triggered, this, &TMSI::showStartRecording);
    addPluginAction(m_pActionStartRecording);

    // Create impedance action bar item/button
    m_pActionImpedance = new QAction(QIcon(":/images/impedances.png"), tr("Check impedance values"), this);
    m_pActionImpedance->setStatusTip(tr("Check impedance values"));
    connect(m_pActionImpedance, &QAction::triggered, this, &TMSI::showImpedanceDialog);
    addPluginAction(m_pActionImpedance);
}


//*************************************************************************************************************

TMSI::~TMSI()
{
    //std::cout << "TMSI::~TMSI() " << std::endl;

    //If the program is closed while the sampling is in process
    if(this->isRunning())
        this->stop();
}


//*************************************************************************************************************

QSharedPointer<IPlugin> TMSI::clone() const
{
    QSharedPointer<TMSI> pTMSIClone(new TMSI());
    return pTMSIClone;
}


//*************************************************************************************************************

void TMSI::init()
{
    m_pRMTSA_TMSI = PluginOutputData<NewRealTimeMultiSampleArray>::create(this, "TMSI", "EEG output data");

    m_outputConnectors.append(m_pRMTSA_TMSI);

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
    m_bUseFiltering = false;
    m_bUseFFT = false;
    m_bIsRunning = false;
    m_bBeepTrigger = false;
    m_bUseCommonAverage = true;
    m_bUseKeyboardTrigger = false;
    m_bCheckImpedances = false;
    m_bSplitFile = false;

    m_iTriggerType = 0;

    QDate date;
    m_sOutputFilePath = QString ("%1Sequence_01/Subject_01/%2_%3_%4_EEG_001_raw.fif").arg(m_qStringResourcePath).arg(date.currentDate().year()).arg(date.currentDate().month()).arg(date.currentDate().day());

    m_sElcFilePath = QString("./resources/general/3DLayouts/standard_waveguard128_duke.elc");

    m_pFiffInfo = QSharedPointer<FiffInfo>(new FiffInfo());

    //Initialise matrix used to perform a very simple high pass filter operation
    m_matOldMatrix = MatrixXf::Zero(m_iNumberOfChannels, m_iSamplesPerBlock);
}


//*************************************************************************************************************

void TMSI::unload()
{

}


//*************************************************************************************************************

void TMSI::setUpFiffInfo()
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
    QList<QVector<double> > elcLocation3D;
    QList<QVector<double> > elcLocation2D;
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
        cout<<"Plugin TMSI - ERROR - LE2 not found. Check loaded layout."<<endl;

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
        cout<<"Plugin TMSI - ERROR - Z1 not found. Check loaded layout."<<endl;

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
        cout<<"Plugin TMSI - ERROR - RE2 not found. Check loaded layout."<<endl;

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
            if(!elcChannelNames.empty() && i<elcChannelNames.size()) {
                sChType = QString("EEG ");
                sChType.append(elcChannelNames.at(i));
                fChInfo.ch_name = sChType;
            } else {
                sChType = QString("EEG ");
                if(i<10)
                    sChType.append("00");

                if(i>=10 && i<100)
                    sChType.append("0");

                fChInfo.ch_name = sChType.append(sChType.number(i));
            }

            //Set channel type
            fChInfo.kind = FIFFV_EEG_CH;

            //Set coil type
            fChInfo.chpos.coil_type = FIFFV_COIL_EEG;

            //Set logno
            fChInfo.logNo = i;

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
            fChInfo.chpos.r0[0] = elcLocation3D[i][0]*0.001;
            fChInfo.chpos.r0[1] = elcLocation3D[i][1]*0.001;
            fChInfo.chpos.r0[2] = elcLocation3D[i][2]*0.001;

            fChInfo.chpos.ex[0] = center_pos.x()*0.001;
            fChInfo.chpos.ex[1] = center_pos.y()*0.001;
            fChInfo.chpos.ex[2] = center_pos.z()*0.001;

            fChInfo.chpos.ey[0] = 0;
            fChInfo.chpos.ey[1] = 1;
            fChInfo.chpos.ey[2] = 0;

            fChInfo.chpos.ez[0] = 0;
            fChInfo.chpos.ez[1] = 0;
            fChInfo.chpos.ez[2] = 1;

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

bool TMSI::start()
{
    //Check if the thread is already or still running. This can happen if the start button is pressed immediately after the stop button was pressed. In this case the stopping process is not finished yet but the start process is initiated.
    if(this->isRunning())
        QThread::wait();

    if(m_bBeepTrigger)
        m_qTimerTrigger.start();

    //Setup fiff info
    setUpFiffInfo();

    //Set the channel size of the RMTSA - this needs to be done here and NOT in the init() function because the user can change the number of channels during runtime
    m_pRMTSA_TMSI->data()->initFromFiffInfo(m_pFiffInfo);
    m_pRMTSA_TMSI->data()->setMultiArraySize(m_iSamplesPerBlock);
    m_pRMTSA_TMSI->data()->setSamplingRate(m_iSamplingFreq);

    //Buffer
    m_pRawMatrixBuffer_In = QSharedPointer<RawMatrixBuffer>(new RawMatrixBuffer(8, m_iNumberOfChannels, m_iSamplesPerBlock));

    m_pTMSIProducer->start(m_iNumberOfChannels,
                       m_iSamplingFreq,
                       m_iSamplesPerBlock,
                       m_bUseChExponent,
                       m_bUseUnitGain,
                       m_bUseUnitOffset,
                       m_bWriteDriverDebugToFile,
                       m_sOutputFilePath,
                       m_bUseCommonAverage,
                       m_bCheckImpedances);

    if(m_pTMSIProducer->isRunning())
    {
        // Init BCIFeatureWindow for visualization
        m_tmsiManualAnnotationWidget = QSharedPointer<TMSIManualAnnotationWidget>(new TMSIManualAnnotationWidget(this));

        if(m_bUseKeyboardTrigger && !m_bCheckImpedances)
        {
            m_tmsiManualAnnotationWidget->initGui();
            m_tmsiManualAnnotationWidget->show();
        }

        m_bIsRunning = true;
        QThread::start();
        return true;
    }
    else
    {
        qWarning() << "Plugin TMSI - ERROR - TMSIProducer thread could not be started - Either the device is turned off (check your OS device manager) or the driver DLL (TMSiSDK.dll / TMSiSDK32bit.dll) is not installed in the system32 / SysWOW64 directory" << endl;
        return false;
    }
}


//*************************************************************************************************************

bool TMSI::stop()
{
    //Stop the producer thread first
    m_pTMSIProducer->stop();

    //Wait until this thread (TMSI) is stopped
    m_bIsRunning = false;

    //In case the semaphore blocks the thread -> Release the QSemaphore and let it exit from the pop function (acquire statement)
    m_pRawMatrixBuffer_In->releaseFromPop();

    m_pRawMatrixBuffer_In->clear();

    m_pRMTSA_TMSI->data()->clear();

    m_tmsiManualAnnotationWidget->hide();

    return true;
}


//*************************************************************************************************************

IPlugin::PluginType TMSI::getType() const
{
    return _ISensor;
}


//*************************************************************************************************************

QString TMSI::getName() const
{
    return "TMSI EEG";
}


//*************************************************************************************************************

QWidget* TMSI::setupWidget()
{
    TMSISetupWidget* widget = new TMSISetupWidget(this);//widget is later destroyed by CentralWidget - so it has to be created everytime new

    //init properties dialog
    widget->initGui();

    return widget;
}


//*************************************************************************************************************

void TMSI::setKeyboardTriggerType(int type)
{
    m_qMutex.lock();
        m_iTriggerType =type;
    m_qMutex.unlock();
}


//*************************************************************************************************************

void TMSI::splitRecordingFile()
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

void TMSI::run()
{
    qint32 size = 0;

    while(m_bIsRunning)
    {
        //std::cout<<"TMSI::run(s)"<<std::endl;

        // Check impedances - send new impedance values to graphic scene
        if(m_pTMSIProducer->isRunning() && m_bCheckImpedances)
        {
            MatrixXf matValue = m_pRawMatrixBuffer_In->pop();

            for(qint32 i = 0; i < matValue.cols(); ++i)
                m_pTmsiImpedanceWidget->updateGraphicScene(matValue.col(i).cast<double>());
        }

        //pop matrix only if the producer thread is running
        if(m_pTMSIProducer->isRunning() && !m_bCheckImpedances)
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

            // Set keyboard trigger (if activated and !=0)
            if(m_bUseKeyboardTrigger && m_iTriggerType!=0)
                matValue(136, m_iSamplesPerBlock-1) = m_iTriggerType;

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

            // TODO: Use preprocessing if wanted by the user
            if(m_bUseFiltering)
            {
                MatrixXf temp = matValue;

                matValue = matValue - m_matOldMatrix;
                m_matOldMatrix = temp;

                //    //Check filter class - will be removed in the future - testing purpose only!
                //    FilterTools* filterObject = new FilterTools();

                //    //kaiser window testing
                //    qint32 numberCoeff = 51;
                //    QVector<float> impulseResponse(numberCoeff);
                //    filterObject->createDynamicFilter(QString('LP'), numberCoeff, (float)0.3, impulseResponse);

                //    ofstream outputFileStream("resources/mne_scan/plugins/tmsi/filterToolsTest.txt", ios::out);

                //    outputFileStream << "impulseResponse:\n";
                //    for(int i=0; i<impulseResponse.size(); i++)
                //        outputFileStream << impulseResponse[i] << " ";
                //    outputFileStream << endl;

                //    //convolution testing
                //    QVector<float> in (12, 2);
                //    QVector<float> kernel (4, 2);

                //    QVector<float> out = filterObject->convolve(in, kernel);

                //    outputFileStream << "convolution result:\n";
                //    for(int i=0; i<out.size(); i++)
                //        outputFileStream << out[i] << " ";
                //    outputFileStream << endl;
            }

            // TODO: Perform a fft if wanted by the user
            if(m_bUseFFT)
            {
                QElapsedTimer timer;
                timer.start();

                FFT<float> fft;
                Matrix<complex<float>, 138, 16> freq;

                for(qint32 i = 0; i < matValue.rows(); ++i)
                    fft.fwd(freq.row(i), matValue.row(i));

//                cout<<"FFT postprocessing done in "<<timer.nsecsElapsed()<<" nanosec"<<endl;
//                cout<<"matValue before FFT:"<<endl<<matValue<<endl;
//                cout<<"freq after FFT:"<<endl<<freq<<endl;
//                matValue = freq.cwiseAbs();
//                cout<<"matValue after FFT:"<<endl<<matValue<<endl;
            }

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
            m_pRMTSA_TMSI->data()->setValue(matValue.cast<double>());

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

    //std::cout<<"EXITING - TMSI::run()"<<std::endl;
}


//*************************************************************************************************************

void TMSI::showImpedanceDialog()
{
    // Open Impedance dialog only if no sampling process is active
    if(!m_bIsRunning)
    {
        if(m_pTmsiImpedanceWidget == NULL)
            m_pTmsiImpedanceWidget = QSharedPointer<TMSIImpedanceWidget>(new TMSIImpedanceWidget(this));

        if(!m_pTmsiImpedanceWidget->isVisible())
        {
            m_pTmsiImpedanceWidget->setWindowTitle("MNE-X - Measure impedances");
            m_pTmsiImpedanceWidget->show();
            m_pTmsiImpedanceWidget->raise();
        }

        m_pTmsiImpedanceWidget->initGraphicScene();
    }
}

//*************************************************************************************************************

void TMSI::showSetupProjectDialog()
{
    // Open setup project widget
    if(m_pTmsiSetupProjectWidget == NULL)
        m_pTmsiSetupProjectWidget = QSharedPointer<TMSISetupProjectWidget>(new TMSISetupProjectWidget(this));

    if(!m_pTmsiSetupProjectWidget->isVisible())
    {
        m_pTmsiSetupProjectWidget->setWindowTitle("TMSI EEG Connector - Setup project");
        m_pTmsiSetupProjectWidget->initGui();
        m_pTmsiSetupProjectWidget->show();
        m_pTmsiSetupProjectWidget->raise();
    }
}

//*************************************************************************************************************

void TMSI::showStartRecording()
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
        connect(m_pTimerRecordingChange.data(), &QTimer::timeout, this, &TMSI::changeRecordingButton);
        m_pTimerRecordingChange->start(500);
    }
}


//*************************************************************************************************************

void TMSI::changeRecordingButton()
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

bool TMSI::dirExists(const std::string& dirName_in)
{
    DWORD ftyp = GetFileAttributesA(dirName_in.c_str());
    if (ftyp == INVALID_FILE_ATTRIBUTES)
        return false;  //something is wrong with your path!

    if (ftyp & FILE_ATTRIBUTE_DIRECTORY)
        return true;   // this is a directory!

    return false;    // this is not a directory!
}


