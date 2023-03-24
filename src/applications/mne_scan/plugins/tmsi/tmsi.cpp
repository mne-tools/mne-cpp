//=============================================================================================================
/**
 * @file     tmsi.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     September, 2013
 *
 * @section  LICENSE
 *
 * Copyright (C) 2013, Christoph Dinh, Lorenz Esch. All rights reserved.
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

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "tmsi.h"
#include "tmsiproducer.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore/QtPlugin>
#include <QtCore/QTextStream>
#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace TMSIPLUGIN;
using namespace SCSHAREDLIB;
using namespace SCMEASLIB;
using namespace UTILSLIB;
using namespace FIFFLIB;
using namespace std;
using namespace UTILSLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

TMSI::TMSI()
: m_pRMTSA_TMSI(0)
, m_qStringResourcePath(qApp->applicationDirPath()+"/../resources/mne_scan/plugins/tmsi/")
, m_pTMSIProducer(new TMSIProducer(this))
, m_pCircularBuffer(QSharedPointer<CircularBuffer_Matrix_float>(new CircularBuffer_Matrix_float(8)))
{
    // Create record file option action bar item/button
    m_pActionSetupProject = new QAction(QIcon(":/images/database.png"), tr("Setup project"), this);
    m_pActionSetupProject->setStatusTip(tr("Setup project"));
    connect(m_pActionSetupProject, &QAction::triggered, this, &TMSI::showSetupProjectDialog);
    addPluginAction(m_pActionSetupProject);

    // Create impedance action bar item/button
    m_pActionImpedance = new QAction(QIcon(":/images/impedances.png"), tr("Check impedance values"), this);
    m_pActionImpedance->setStatusTip(tr("Check impedance values"));
    connect(m_pActionImpedance, &QAction::triggered, this, &TMSI::showImpedanceDialog);
    addPluginAction(m_pActionImpedance);
}

//=============================================================================================================

TMSI::~TMSI()
{
    //If the program is closed while the sampling is in process
    if(this->isRunning()) {
        this->stop();
    }
}

//=============================================================================================================

QSharedPointer<AbstractPlugin> TMSI::clone() const
{
    QSharedPointer<TMSI> pTMSIClone(new TMSI());
    return pTMSIClone;
}

//=============================================================================================================

void TMSI::init()
{
    m_pRMTSA_TMSI = PluginOutputData<RealTimeMultiSampleArray>::create(this, "TMSI", "EEG output data");
    m_pRMTSA_TMSI->measurementData()->setName(this->getName());//Provide name to auto store widget settings

    m_outputConnectors.append(m_pRMTSA_TMSI);

    //default values used by the setupGUI class must be set here
    m_iSamplingFreq = 1024;
    m_iNumberOfChannels = 138;
    m_iSamplesPerBlock = 16;

    m_bUseChExponent = true;
    m_bUseUnitGain = true;
    m_bUseUnitOffset = true;
    m_bWriteDriverDebugToFile = false;
    m_bBeepTrigger = false;
    m_bUseCommonAverage = true;
    m_bUseKeyboardTrigger = false;
    m_bCheckImpedances = false;

    m_iTriggerType = 0;

    m_sElcFilePath = QString("./resources/general/3DLayouts/standard_waveguard128_duke.elc");

    m_pFiffInfo = QSharedPointer<FiffInfo>(new FiffInfo());

    //Initialise matrix used to perform a very simple high pass filter operation
    m_matOldMatrix = MatrixXf::Zero(m_iNumberOfChannels, m_iSamplesPerBlock);
}

//=============================================================================================================

void TMSI::unload()
{
}

//=============================================================================================================

void TMSI::setUpFiffInfo()
{
    // Only works for ANT Neuro Waveguard Duke caps
    //Clear old fiff info data
    m_pFiffInfo->clear();

    //Set number of channels, sampling frequency and high/-lowpass
    m_pFiffInfo->nchan = m_iNumberOfChannels;
    m_pFiffInfo->sfreq = m_iSamplingFreq;
    m_pFiffInfo->highpass = (float)0.001;
    m_pFiffInfo->lowpass = m_iSamplingFreq/2;

    //Read electrode positions from .elc file
    QList<QVector<float> > elcLocation3D;
    QList<QVector<float> > elcLocation2D;
    QString unit;
    QStringList elcChannelNames;

    if(!LayoutLoader::readAsaElcFile(m_sElcFilePath,
                                     elcChannelNames,
                                     elcLocation3D,
                                     elcLocation2D,
                                     unit))
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
        QVector<float> tempA(3, 0.0);
        QVector<float> tempB(2, 0.0);
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

    //Set up the channel info
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

    //Set head projection
    m_pFiffInfo->dev_head_t.from = FIFFV_COORD_DEVICE;
    m_pFiffInfo->dev_head_t.to = FIFFV_COORD_HEAD;
    m_pFiffInfo->ctf_head_t.from = FIFFV_COORD_DEVICE;
    m_pFiffInfo->ctf_head_t.to = FIFFV_COORD_HEAD;

    //Set projection data
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

//=============================================================================================================

bool TMSI::start()
{
    if(m_bBeepTrigger) {
        m_qTimerTrigger.start();
    }

    //Setup fiff info
    setUpFiffInfo();

    //Set the channel size of the RMTSA - this needs to be done here and NOT in the init() function because the user can change the number of channels during runtime
    m_pRMTSA_TMSI->measurementData()->initFromFiffInfo(m_pFiffInfo);
    m_pRMTSA_TMSI->measurementData()->setMultiArraySize(m_iSamplesPerBlock);
    m_pRMTSA_TMSI->measurementData()->setSamplingRate(m_iSamplingFreq);

    m_pTMSIProducer->start(m_iNumberOfChannels,
                           m_iSamplingFreq,
                           m_iSamplesPerBlock,
                           m_bUseChExponent,
                           m_bUseUnitGain,
                           m_bUseUnitOffset,
                           m_bWriteDriverDebugToFile,
                           m_bUseCommonAverage,
                           m_bCheckImpedances);
    wait(500);

    if(m_pTMSIProducer->isRunning()) {
        // Init BCIFeatureWindow for visualization
        m_pTmsiManualAnnotationWidget = QSharedPointer<TMSIManualAnnotationWidget>(new TMSIManualAnnotationWidget(this));

        if(m_bUseKeyboardTrigger && !m_bCheckImpedances) {
            m_pTmsiManualAnnotationWidget->initGui();
            m_pTmsiManualAnnotationWidget->show();
        }

        QThread::start();
        return true;
    } else {
        qWarning() << "[TMSI::start] TMSIProducer thread could not be started - Either the device is turned off (check your OS device manager) or the driver DLL (TMSiSDK.dll / TMSiSDK32bit.dll) is not installed in the system32 / SysWOW64 directory" << endl;
        return false;
    }
}

//=============================================================================================================

bool TMSI::stop()
{
    //Stop the producer thread first
    m_pTMSIProducer->stop();

    //Wait until this thread (TMSI) is stopped
    requestInterruption();
    wait(500);

    // Clear all data in the buffer connected to displays and other plugins
    m_pRMTSA_TMSI->measurementData()->clear();
    m_pCircularBuffer->clear();

    if(m_pTmsiManualAnnotationWidget) {
        m_pTmsiManualAnnotationWidget->hide();
    }

    return true;
}

//=============================================================================================================

AbstractPlugin::PluginType TMSI::getType() const
{
    return _ISensor;
}

//=============================================================================================================

QString TMSI::getName() const
{
    return "TMSI";
}

//=============================================================================================================

QWidget* TMSI::setupWidget()
{
    TMSISetupWidget* widget = new TMSISetupWidget(this);//widget is later destroyed by CentralWidget - so it has to be created everytime new

    //init properties dialog
    widget->initGui();

    return widget;
}

//=============================================================================================================

void TMSI::setKeyboardTriggerType(int type)
{
    m_qMutex.lock();
    m_iTriggerType =type;
    m_qMutex.unlock();
}

//=============================================================================================================

void TMSI::run()
{
    qint32 size = 0;
    MatrixXf matData;

    while(!isInterruptionRequested()) {
        // Check impedances - send new impedance values to graphic scene
        if(m_pTMSIProducer->isRunning() && m_bCheckImpedances) {
            if(m_pCircularBuffer->pop(matData)) {
                for(qint32 i = 0; i < matData.cols(); ++i) {
                    m_pTmsiImpedanceWidget->updateGraphicScene(matData.col(i).cast<double>());
                }
            }
        }

        //pop matrix only if the producer thread is running
        if(m_pTMSIProducer->isRunning() && !m_bCheckImpedances) {
            if(m_pCircularBuffer->pop(matData)) {
                // Set Beep trigger (if activated)
                if(m_bBeepTrigger && m_qTimerTrigger.elapsed() >= m_iTriggerInterval) {
                    QtConcurrent::run(Beep, 450, 700);
                    //Set trigger in received data samples - just for one sample, so that this event is easy to detect
                    matData(136, m_iSamplesPerBlock-1) = 252;
                    m_qTimerTrigger.restart();
                }

                // Set keyboard trigger (if activated and !=0)
                if(m_bUseKeyboardTrigger && m_iTriggerType!=0) {
                    matData(136, m_iSamplesPerBlock-1) = m_iTriggerType;
                }

                //Change values of the trigger channel for better plotting - this change is not saved in the produced fif file
                if(m_iNumberOfChannels>137) {
                    for(int i = 0; i<matData.row(137).cols(); i++) {
                        // Left keyboard or capacitive
                        if(matData.row(136)[i] == 254) {
                            matData.row(136)[i] = 4000;
                        }

                        // Right keyboard
                        if(matData.row(136)[i] == 253) {
                            matData.row(136)[i] = 8000;
                        }

                        // Beep
                        if(matData.row(136)[i] == 252) {
                            matData.row(136)[i] = 2000;
                        }
                    }
                }

                //emit values to real time multi sample array
                m_pRMTSA_TMSI->measurementData()->setValue(matData.cast<double>());

                // Reset keyboard trigger
                m_iTriggerType = 0;
            }
        }
    }
}

//=============================================================================================================

void TMSI::showImpedanceDialog()
{
    // Open Impedance dialog only if no sampling process is active
    if(!this->isRunning()) {
        if(m_pTmsiImpedanceWidget == NULL) {
            m_pTmsiImpedanceWidget = QSharedPointer<TMSIImpedanceWidget>(new TMSIImpedanceWidget(this));
        }

        if(!m_pTmsiImpedanceWidget->isVisible()) {
            m_pTmsiImpedanceWidget->setWindowTitle("MNE Scan - Measure impedances");
            m_pTmsiImpedanceWidget->show();
            m_pTmsiImpedanceWidget->raise();
        }

        m_pTmsiImpedanceWidget->initGraphicScene();
    }
}

//=============================================================================================================

void TMSI::showSetupProjectDialog()
{
    // Open setup project widget
    if(m_pTmsiSetupProjectWidget == NULL) {
        m_pTmsiSetupProjectWidget = QSharedPointer<TMSISetupProjectWidget>(new TMSISetupProjectWidget(this));
    }

    if(!m_pTmsiSetupProjectWidget->isVisible()) {
        m_pTmsiSetupProjectWidget->setWindowTitle("TMSI EEG Connector - Setup project");
        m_pTmsiSetupProjectWidget->initGui();
        m_pTmsiSetupProjectWidget->show();
        m_pTmsiSetupProjectWidget->raise();
    }
}

//=============================================================================================================

QString TMSI::getBuildInfo()
{
    return QString(TMSIPLUGIN::buildDateTime()) + QString(" - ")  + QString(TMSIPLUGIN::buildHash());
}
