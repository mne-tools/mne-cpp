//=============================================================================================================
/**
 * @file     eegosports.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Viktor Klueber <Viktor.Klueber@tu-ilmenau.de>;
 *           Johannes Vorwerk <johannes.vorwerk@umit.at>
 * @version  dev
 * @date     February, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020, Christoph Dinh, Lorenz Esch, Viktor Klueber, Johannes Vorwerk. All rights reserved.
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
 * @brief    Contains the implementation of the EEGoSports class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "eegosports.h"
#include "eegosportsproducer.h"

#include "FormFiles/eegosportssetupwidget.h"
#include "FormFiles/eegosportssetupprojectwidget.h"

#include <utils/layoutloader.h>
#include <utils/layoutmaker.h>

#include <scMeas/realtimemultisamplearray.h>

#include <fiff/fiff.h>

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSettings>
#include <QDate>
#include <QVector3D>
#include <QTimer>
#include <QMessageBox>
#include <QDir>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace EEGOSPORTSPLUGIN;
using namespace SCSHAREDLIB;
using namespace SCMEASLIB;
using namespace IOBUFFER;
using namespace FIFFLIB;
using namespace UTILSLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

EEGoSports::EEGoSports()
: m_pRMTSA_EEGoSports(0)
, m_qStringResourcePath(qApp->applicationDirPath()+"/resources/mne_scan/plugins/eegosports/")
, m_pEEGoSportsProducer(new EEGoSportsProducer(this))
, m_dLPAShift(0.01)
, m_dRPAShift(0.01)
, m_dNasionShift(0.06)
, m_bUseTrackedCardinalMode(true)
, m_bUseElectrodeShiftMode(false)
, m_sLPA("2LD")
, m_sRPA("2RD")
, m_sNasion("0Z")
{
    // Create record file option action bar item/button
    m_pActionSetupProject = new QAction(QIcon(":/images/database.png"), tr("Setup project"), this);
    m_pActionSetupProject->setStatusTip(tr("Setup project"));
    connect(m_pActionSetupProject, &QAction::triggered, this, &EEGoSports::showSetupProjectDialog);
    addPluginAction(m_pActionSetupProject);

    // Create start recordin action bar item/button
    m_pActionStartRecording = new QAction(QIcon(":/images/record.png"), tr("Start recording data to fif file"), this);
    m_pActionStartRecording->setStatusTip(tr("Start recording data to fif file"));
    connect(m_pActionStartRecording, &QAction::triggered, this, &EEGoSports::showStartRecording);
    addPluginAction(m_pActionStartRecording);

    // Create impedance action bar item/button
    m_pActionImpedance = new QAction(QIcon(":/images/impedances.png"), tr("Check impedance values"), this);
    m_pActionImpedance->setStatusTip(tr("Check impedance values"));
    connect(m_pActionImpedance, &QAction::triggered, this, &EEGoSports::showImpedanceDialog);
    addPluginAction(m_pActionImpedance);
}

//=============================================================================================================

EEGoSports::~EEGoSports()
{
    //If the program is closed while the sampling is in process
    if(this->isRunning()) {
        this->stop();    
    }

    //Store settings for next use
    QSettings settings;
    settings.setValue(QString("EEGOSPORTS/sFreq"), m_iSamplingFreq);
    settings.setValue(QString("EEGOSPORTS/samplesPerBlock"), m_iSamplesPerBlock);
    settings.setValue(QString("EEGOSPORTS/LPAShift"), m_dLPAShift);
    settings.setValue(QString("EEGOSPORTS/RPAShift"), m_dRPAShift);
    settings.setValue(QString("EEGOSPORTS/NasionShift"), m_dNasionShift);
    settings.setValue(QString("EEGOSPORTS/LPAElectrode"), m_sLPA);
    settings.setValue(QString("EEGOSPORTS/RPAElectrode"), m_sRPA);
    settings.setValue(QString("EEGOSPORTS/NasionElectrode"), m_sNasion);
    settings.setValue(QString("EEGOSPORTS/outputFilePath"), m_sOutputFilePath);
    settings.setValue(QString("EEGOSPORTS/elcFilePath"), m_sElcFilePath);
    settings.setValue(QString("EEGOSPORTS/cardinalFilePath"), m_sCardinalFilePath);
    settings.setValue(QString("EEGOSPORTS/useTrackedCardinalsMode"), m_bUseTrackedCardinalMode);
    settings.setValue(QString("EEGOSPORTS/useElectrodeshiftMode"), m_bUseElectrodeShiftMode);
}

//=============================================================================================================

QSharedPointer<IPlugin> EEGoSports::clone() const
{
    QSharedPointer<EEGoSports> pEEGoSportsClone(new EEGoSports());
    return pEEGoSportsClone;
}

//=============================================================================================================

void EEGoSports::init()
{
    m_pRMTSA_EEGoSports = PluginOutputData<RealTimeMultiSampleArray>::create(this, "EEGoSports", "EEG output data");

    m_outputConnectors.append(m_pRMTSA_EEGoSports);

    //default values used by the setupGUI class must be set here

    QSettings settings;
    m_iSamplingFreq = settings.value(QString("EEGOSPORTS/sFreq"), 512).toInt();
    m_iNumberOfChannels = 90;
    m_iNumberOfEEGChannels = 64;
    m_iSamplesPerBlock = settings.value(QString("EEGOSPORTS/samplesPerBlock"), 512).toInt();
    m_bWriteToFile = false;
    m_bWriteDriverDebugToFile = false;
    m_bIsRunning = false;
    m_bCheckImpedances = false;

    QDate date;
    m_sOutputFilePath = settings.value(QString("EEGOSPORTS/outputFilePath"), QString("%1Sequence_01/Subject_01/%2_%3_%4_EEG_001_raw.fif").arg(m_qStringResourcePath).arg(date.currentDate().year()).arg(date.currentDate().month()).arg(date.currentDate().day())).toString();

    m_sElcFilePath = settings.value(QString("EEGOSPORTS/elcFilePath"), QString(QCoreApplication::applicationDirPath() + "/resources/3DLayouts/standard_waveguard64_duke.elc")).toString();

    m_sCardinalFilePath = settings.value(QString("EEGOSPORTS/cardinalFilePath"), QString("")).toString();

    m_dLPAShift = settings.value(QString("EEGOSPORTS/LPAShift"), 0.0).toFloat();
    m_dRPAShift = settings.value(QString("EEGOSPORTS/RPAShift"), 0.0).toFloat();
    m_dNasionShift = settings.value(QString("EEGOSPORTS/NasionShift"), 0.0).toFloat();
    m_sLPA = settings.value(QString("EEGOSPORTS/LPAElectrode"), QString("")).toString();
    m_sRPA = settings.value(QString("EEGOSPORTS/RPAElectrode"), QString("")).toString();
    m_sNasion = settings.value(QString("EEGOSPORTS/NasionElectrode"), QString("")).toString();
    m_bUseTrackedCardinalMode = settings.value(QString("EEGOSPORTS/useTrackedCardinalsMode"), true).toBool();
    m_bUseElectrodeShiftMode = settings.value(QString("EEGOSPORTS/useElectrodeshiftMode"), false).toBool();

    m_pFiffInfo = QSharedPointer<FiffInfo>(new FiffInfo());
}

//=============================================================================================================

void EEGoSports::unload()
{
}

//=============================================================================================================

void EEGoSports::setUpFiffInfo()
{
    //
    //Clear old fiff info data
    //
    m_pFiffInfo->clear();

    //
    //Set number of channels, sampling frequency and high/-lowpass
    //
    m_pFiffInfo->nchan = m_iNumberOfChannels;
    m_pFiffInfo->sfreq = m_iSamplingFreq;

    //
    //Get amplifier data
    //
    /*QList<uint> channellist = m_pEEGoSportsProducer->getChannellist();

    for(QList<uint>::iterator i=channellist.begin(); i!=channellist.end(); ++i)
        std::cout << *i << std::endl;*/

    //
    //Read electrode positions from .elc file
    //
    QList<QVector<float> > elcLocation3D;
    QList<QVector<float> > elcLocation2D;
    QString unit;
    QStringList elcChannelNames;

    if(!LayoutLoader::readAsaElcFile(m_sElcFilePath, elcChannelNames, elcLocation3D, elcLocation2D, unit)) {
        qWarning() << "Unable to read elc file.";
    }

    bool breflocation = elcChannelNames.contains("ref",Qt::CaseInsensitive);
    bool bgndlocation = elcChannelNames.contains("gnd",Qt::CaseInsensitive);

    //qDebug() << elcLocation3D;
    //qDebug() << elcLocation2D;
    //qDebug() << elcChannelNames;

    //The positions read from the asa elc file do not correspond to a RAS coordinate system - use a simple 90° z transformation to fix this
    Matrix3f rotation_z;
    rotation_z = AngleAxisf((float)M_PI/2, Vector3f::UnitZ()); //M_PI/2 = 90°
    QVector3D center_pos;

    for(int i = 0; i<elcLocation3D.size(); i++) {
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
    //Check if channel size by user corresponds with read channel informations from the elc file. Adding 1 for reference channel. If not append zeros and string 'unknown' until the size matches.
    if((m_iNumberOfEEGChannels + int(breflocation) + int(bgndlocation && m_bCheckImpedances)) > elcLocation3D.size()) {
        qWarning()<<"[EEGoSports::setUpFiffInfo] Not enough positions read from the elc file. Filling missing channel names and positions with zeroes and 'unknown' strings.";

        QVector<float> tempA(3, 0.0);
        QVector<float> tempB(2, 0.0);
        int size = (m_iNumberOfEEGChannels + int(breflocation) + int(bgndlocation && m_bCheckImpedances)) - elcLocation3D.size();

        for(int i = 0; i<size; i++) {
            elcLocation3D.push_back(tempA);
            elcLocation2D.push_back(tempB);
            elcChannelNames.append(QString("Unknown"));
        }
    }

    //
    //Append cardinal points LPA RPA Nasion
    //
    QList<QVector<float> > cardinals3D;
    QList<QVector<float> > cardinals2D;
    QStringList cardinalNames;

    if(!LayoutLoader::readAsaElcFile(m_sCardinalFilePath, cardinalNames, cardinals3D, cardinals2D, unit)) {
        qWarning() << "[EEGoSports::setUpFiffInfo] Unable to read elc cardinal file.";
    }

    //Rotate cardinal points
    for(int i = 0; i < cardinals3D.size(); i++) {
        Vector3f point;
        point << cardinals3D[i][0], cardinals3D[i][1] , cardinals3D[i][2];
        Vector3f point_rot = rotation_z * point;

        cardinals3D[i][0] = point_rot[0];
        cardinals3D[i][1] = point_rot[1];
        cardinals3D[i][2] = point_rot[2];
    }

//    qDebug()<<"cardinals3D"<<cardinals3D;
//    qDebug()<<"cardinals2D"<<cardinals2D;
//    qDebug()<<"cardinalNames"<<cardinalNames;

    //Append LAP value to digitizer data.
    FiffDigPoint digPoint;
    int indexLPA = elcChannelNames.indexOf(m_sLPA);
    digPoint.kind = FIFFV_POINT_CARDINAL;
    digPoint.ident = FIFFV_POINT_LPA;//digitizerInfo.size();

    //Set EEG electrode location - Convert from mm to m
    if(m_bUseTrackedCardinalMode && !m_sCardinalFilePath.isEmpty() && cardinals3D.size() == 3 && cardinalNames.contains("LPA")) {
        indexLPA = cardinalNames.indexOf("LPA");

        digPoint.r[0] = cardinals3D[indexLPA][0]*0.001;
        digPoint.r[1] = cardinals3D[indexLPA][1]*0.001;
        digPoint.r[2] = cardinals3D[indexLPA][2]*0.001;
        digitizerInfo.push_back(digPoint);
    } else if(m_bUseElectrodeShiftMode) {
        if(indexLPA != -1) {
            digPoint.r[0] = elcLocation3D[indexLPA][0]*0.001;
            digPoint.r[1] = elcLocation3D[indexLPA][1]*0.001;
            digPoint.r[2] = (elcLocation3D[indexLPA][2]-m_dLPAShift*10)*0.001;
            digitizerInfo.push_back(digPoint);
        } else {
            qWarning() << "[EEGoSports::setUpFiffInfo] LPA" << m_sLPA << " not found. Check loaded layout.";
        }
    }

    //Append nasion value to digitizer data.
    int indexNasion = elcChannelNames.indexOf(m_sNasion);
    digPoint.kind = FIFFV_POINT_CARDINAL;//FIFFV_POINT_NASION;
    digPoint.ident = FIFFV_POINT_NASION;//digitizerInfo.size();

    //Set EEG electrode location - Convert from mm to m
    if(m_bUseTrackedCardinalMode && !m_sCardinalFilePath.isEmpty() && cardinals3D.size() == 3 && cardinalNames.contains("Nasion")) {
        indexNasion = cardinalNames.indexOf("Nasion");

        digPoint.r[0] = cardinals3D[indexNasion][0]*0.001;
        digPoint.r[1] = cardinals3D[indexNasion][1]*0.001;
        digPoint.r[2] = cardinals3D[indexNasion][2]*0.001;
        digitizerInfo.push_back(digPoint);
    } else if(m_bUseElectrodeShiftMode) {
        if(indexNasion != -1) {
            digPoint.r[0] = elcLocation3D[indexNasion][0]*0.001;
            digPoint.r[1] = elcLocation3D[indexNasion][1]*0.001;
            digPoint.r[2] = (elcLocation3D[indexNasion][2]-m_dNasionShift*10)*0.001;
            digitizerInfo.push_back(digPoint);
        } else {
            qWarning() << "[EEGoSports::setUpFiffInfo] Nasion" << m_sNasion << " not found. Check loaded layout.";
        }
    }

    //Append RAP value to digitizer data.
    int indexRPA = elcChannelNames.indexOf(m_sRPA);
    digPoint.kind = FIFFV_POINT_CARDINAL;
    digPoint.ident = FIFFV_POINT_RPA;//digitizerInfo.size();

    //Set EEG electrode location - Convert from mm to m
    if(m_bUseTrackedCardinalMode && !m_sCardinalFilePath.isEmpty() && cardinals3D.size() == 3 && cardinalNames.contains("RPA")) {
        indexRPA = cardinalNames.indexOf("RPA");

        digPoint.r[0] = cardinals3D[indexRPA][0]*0.001;
        digPoint.r[1] = cardinals3D[indexRPA][1]*0.001;
        digPoint.r[2] = cardinals3D[indexRPA][2]*0.001;
        digitizerInfo.push_back(digPoint);
    } else if(m_bUseElectrodeShiftMode) {
        if(indexRPA != -1) {
            digPoint.r[0] = elcLocation3D[indexRPA][0]*0.001;
            digPoint.r[1] = elcLocation3D[indexRPA][1]*0.001;
            digPoint.r[2] = (elcLocation3D[indexRPA][2]-m_dRPAShift*10)*0.001;
            digitizerInfo.push_back(digPoint);
        } else {
            qWarning() << "[EEGoSports::setUpFiffInfo] RPA" << m_sRPA << " not found. Check loaded layout.";
        }
    }

    //Add EEG electrode positions as digitizers
    for(int i=0; i < (m_iNumberOfEEGChannels + int(breflocation) + int(bgndlocation && m_bCheckImpedances)); i++) {
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

    for(int i=0; i<m_iNumberOfChannels; i++) {
        //Create information for each channel
        QString sChType;
        FiffChInfo fChInfo;

        //EEG Channels
        if(i < m_iNumberOfEEGChannels || (i > (m_iNumberOfEEGChannels + m_iNumberOfBipolarChannels)  && m_bCheckImpedances)) {
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
            fChInfo.chpos.r0(0) = elcLocation3D[i][0]*0.001;
            fChInfo.chpos.r0(1) = elcLocation3D[i][1]*0.001;
            fChInfo.chpos.r0(2) = elcLocation3D[i][2]*0.001;

            fChInfo.chpos.ex(0) = center_pos.x()*0.001;
            fChInfo.chpos.ex(1) = center_pos.y()*0.001;
            fChInfo.chpos.ex(2) = center_pos.z()*0.001;

            fChInfo.chpos.ey(0) = 0;
            fChInfo.chpos.ey(1) = 1;
            fChInfo.chpos.ey(2) = 0;

            fChInfo.chpos.ez(0) = 0;
            fChInfo.chpos.ez(1) = 0;
            fChInfo.chpos.ez(2) = 1;

            //cout<<i<<endl<<fChInfo.eeg_loc<<endl;
        }
        //Bipolar channels -- no idea how this behaves for impedance measurement
        else if(!m_bCheckImpedances && (i >= m_iNumberOfEEGChannels && i < m_iNumberOfEEGChannels + m_iNumberOfBipolarChannels)) {
            //Set channel type
            fChInfo.kind = FIFFV_MISC_CH;

            sChType = QString("BIPO ");
            fChInfo.ch_name = sChType.append(sChType.number(i - m_iNumberOfEEGChannels));
        }
        //Digital input channel
        else if(!m_bCheckImpedances && i == m_iNumberOfEEGChannels + m_iNumberOfBipolarChannels) {
            //Set channel type
            fChInfo.kind = FIFFV_STIM_CH;

            sChType = QString("STIM");
            fChInfo.ch_name = sChType;
        }
        //Internally generated test signal - ramp signal
        else if(!m_bCheckImpedances && i == m_iNumberOfEEGChannels + m_iNumberOfBipolarChannels + 1) {
            //Set channel type
            fChInfo.kind = FIFFV_MISC_CH;

            sChType = QString("TEST");
            fChInfo.ch_name = sChType;
        }
        //Add reference channel for EEG measurement
        //Set channel name
        else if(!m_bCheckImpedances && i == m_iNumberOfChannels - 1) {
            int refindex = elcChannelNames.indexOf(QRegExp("ref", Qt::CaseInsensitive, QRegExp::RegExp));

            sChType = QString("EEG REF");
            fChInfo.ch_name = sChType;

            //Set channel type
            fChInfo.kind = FIFFV_EEG_CH;

            //Set logno
            fChInfo.logNo = m_iNumberOfChannels;

            //Set coord frame
            fChInfo.coord_frame = FIFFV_COORD_HEAD;

            //Set unit
            fChInfo.unit = FIFF_UNIT_V;
            fChInfo.unit_mul = 0;

            //Set EEG electrode location - Convert from mm to m
            fChInfo.eeg_loc(0,0) = elcLocation3D[refindex][0]*0.001;
            fChInfo.eeg_loc(1,0) = elcLocation3D[refindex][1]*0.001;
            fChInfo.eeg_loc(2,0) = elcLocation3D[refindex][2]*0.001;

            //Set EEG electrode direction - Convert from mm to m
            fChInfo.eeg_loc(0,1) = center_pos.x()*0.001;
            fChInfo.eeg_loc(1,1) = center_pos.y()*0.001;
            fChInfo.eeg_loc(2,1) = center_pos.z()*0.001;

            //Also write the eeg electrode locations into the meg loc variable (mne_ex_read_raw() matlab function wants this)
            fChInfo.chpos.r0(0) = elcLocation3D[refindex][0]*0.001;
            fChInfo.chpos.r0(1) = elcLocation3D[refindex][1]*0.001;
            fChInfo.chpos.r0(2) = elcLocation3D[refindex][2]*0.001;

            fChInfo.chpos.ex(0) = center_pos.x()*0.001;
            fChInfo.chpos.ex(1) = center_pos.y()*0.001;
            fChInfo.chpos.ex(2) = center_pos.z()*0.001;

            fChInfo.chpos.ey(0) = 0;
            fChInfo.chpos.ey(1) = 1;
            fChInfo.chpos.ey(2) = 0;

            fChInfo.chpos.ez(0) = 0;
            fChInfo.chpos.ez(1) = 0;
            fChInfo.chpos.ez(2) = 1;
        }

        QSLChNames << sChType;

        m_pFiffInfo->chs.append(fChInfo);
    }

    qInfo() << "[EEGoSports::setUpFiffInfo] Number of Channels " << QSLChNames.length();

    //Set channel names in fiff_info_base
    m_pFiffInfo->ch_names = QSLChNames;

    //
    //Set head projection
    //
    m_pFiffInfo->dev_head_t.from = FIFFV_COORD_DEVICE;
    m_pFiffInfo->dev_head_t.to = FIFFV_COORD_HEAD;
    m_pFiffInfo->ctf_head_t.from = FIFFV_COORD_DEVICE;
    m_pFiffInfo->ctf_head_t.to = FIFFV_COORD_HEAD;
}

//=============================================================================================================

void EEGoSports::setNumberOfChannels(int iNumberOfChannels, int iNumberOfEEGChannels, int iNumberOfBipolarChannels)
{
    m_iNumberOfChannels = iNumberOfChannels;
    m_iNumberOfEEGChannels = iNumberOfEEGChannels;
    m_iNumberOfBipolarChannels = iNumberOfBipolarChannels;
}

//=============================================================================================================

bool EEGoSports::start()
{
    //Check if the thread is already or still running. This can happen if the start button is pressed immediately after the stop button was pressed. In this case the stopping process is not finished yet but the start process is initiated.
    if(this->isRunning()) {
        QThread::wait();
    }

    //Initialize amplifier
    if(!m_pEEGoSportsProducer->init(m_bWriteDriverDebugToFile,
                                    m_sOutputFilePath,
                                    m_bCheckImpedances))
    {
        qWarning() << "Plugin EEGoSports - ERROR - EEGoSportsProducer thread could not be started - Either the device is turned off (check your OS device manager) or the driver DLL (EEGO-SDK.dll) is not installed in one of the monitored dll path." << endl;
        return false;
    }

    //Setup fiff info
    setUpFiffInfo();

    //Set the channel size of the RMTSA - this needs to be done here and NOT in the init() function because the user can change the number of channels during runtime
    m_pRMTSA_EEGoSports->data()->initFromFiffInfo(m_pFiffInfo);
    m_pRMTSA_EEGoSports->data()->setMultiArraySize(1);
    m_pRMTSA_EEGoSports->data()->setSamplingRate(m_iSamplingFreq);

    //Buffer
    m_qListReceivedSamples.clear();

    m_pEEGoSportsProducer->start(m_iSamplesPerBlock,
                       m_iSamplingFreq,
                       m_bCheckImpedances);

    if(m_pEEGoSportsProducer->isRunning()) {
        m_bIsRunning = true;
        QThread::start();
        return true;
    } else {
        qWarning() << "[EEGoSports::start] Producer thread could not be started - Either the device is turned off (check your OS device manager) or the driver DLL (EEGO-SDK.dll) is not installed in one of the monitored dll path." << endl;
        return false;
    }
}

//=============================================================================================================

bool EEGoSports::stop()
{
    //Stop the producer thread first
    m_pEEGoSportsProducer->stop();

    //Wait until this thread (EEGoSports) is stopped
    m_bIsRunning = false;

    m_pRMTSA_EEGoSports->data()->clear();

    m_qListReceivedSamples.clear();

    return true;
}

//=============================================================================================================

void EEGoSports::setSampleData(MatrixXd &matRawBuffer)
{
    m_mutex.lock();
    m_qListReceivedSamples.append(matRawBuffer);
    m_mutex.unlock();
}

//=============================================================================================================

IPlugin::PluginType EEGoSports::getType() const
{
    return _ISensor;
}

//=============================================================================================================

QString EEGoSports::getName() const
{
    return "EEGoSports EEG";
}

//=============================================================================================================

QWidget* EEGoSports::setupWidget()
{
    EEGoSportsSetupWidget* widget = new EEGoSportsSetupWidget(this);//widget is later destroyed by CentralWidget - so it has to be created everytime new

    //init properties dialog
    widget->initGui();

    return widget;
}

//=============================================================================================================

void EEGoSports::onUpdateCardinalPoints(const QString& sLPA, double dLPA, const QString& sRPA, double dRPA, const QString& sNasion, double dNasion)
{
    m_dLPAShift = dLPA;
    m_dRPAShift = dRPA;
    m_dNasionShift = dNasion;

    m_sLPA = sLPA;
    m_sRPA = sRPA;
    m_sNasion = sNasion;
}

//=============================================================================================================

void EEGoSports::showImpedanceDialog()
{
    // Open Impedance dialog only if no sampling process is active
    if(!m_bIsRunning)
    {
        if(m_pEEGoSportsImpedanceWidget == NULL)
            m_pEEGoSportsImpedanceWidget = QSharedPointer<EEGoSportsImpedanceWidget>(new EEGoSportsImpedanceWidget(this));

        if(!m_pEEGoSportsImpedanceWidget->isVisible())
        {
            m_pEEGoSportsImpedanceWidget->setWindowTitle("EEGoSports - Measure impedances");
            m_pEEGoSportsImpedanceWidget->show();
            m_pEEGoSportsImpedanceWidget->raise();
        }

        m_pEEGoSportsImpedanceWidget->initGraphicScene();
    }
}

//=============================================================================================================

void EEGoSports::showSetupProjectDialog()
{
    // Open setup project widget
    if(m_pEEGoSportsSetupProjectWidget == Q_NULLPTR) {
        m_pEEGoSportsSetupProjectWidget = QSharedPointer<EEGoSportsSetupProjectWidget>(new EEGoSportsSetupProjectWidget(this));

        connect(m_pEEGoSportsSetupProjectWidget.data(), &EEGoSportsSetupProjectWidget::cardinalPointsChanged,
                this, &EEGoSports::onUpdateCardinalPoints);
    }

    if(!m_pEEGoSportsSetupProjectWidget->isVisible()) {
        m_pEEGoSportsSetupProjectWidget->setWindowTitle("EEGoSports EEG Connector - Setup project");
        m_pEEGoSportsSetupProjectWidget->show();
        m_pEEGoSportsSetupProjectWidget->raise();
    }
}

//=============================================================================================================

void EEGoSports::showStartRecording()
{
    //Setup writing to file
    if(m_bWriteToFile) {
        m_pOutfid->finish_writing_raw();
        m_bWriteToFile = false;
        m_pTimerRecordingChange->stop();
        m_pActionStartRecording->setIcon(QIcon(":/images/record.png"));
    } else {
        if(!m_bIsRunning) {
            QMessageBox msgBox;
            msgBox.setText("Start data acquisition first!");
            msgBox.exec();
            return;
        }

        if(!m_pFiffInfo) {
            QMessageBox msgBox;
            msgBox.setText("FiffInfo missing!");
            msgBox.exec();
            return;
        }

        //Initiate the stream for writing to the fif file
        m_fileOut.setFileName(m_sOutputFilePath);

        if(m_fileOut.exists()) {
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

        QDir dir;
        if(!dir.exists(fileDir))
            if(!dir.mkpath(fileDir))
                return;

        m_pOutfid = Fiff::start_writing_raw(m_fileOut, *m_pFiffInfo, m_cals);
        fiff_int_t first = 0;
        m_pOutfid->write_int(FIFF_FIRST_SAMPLE, &first);

        m_bWriteToFile = true;

        m_pTimerRecordingChange = QSharedPointer<QTimer>(new QTimer);
        connect(m_pTimerRecordingChange.data(), &QTimer::timeout, this, &EEGoSports::changeRecordingButton);
        m_pTimerRecordingChange->start(500);
    }
}

//=============================================================================================================

void EEGoSports::changeRecordingButton()
{
    if(m_iBlinkStatus == 0) {
        m_pActionStartRecording->setIcon(QIcon(":/images/record.png"));
        m_iBlinkStatus = 1;
    } else {
        m_pActionStartRecording->setIcon(QIcon(":/images/record_active.png"));
        m_iBlinkStatus = 0;
    }
}

//=============================================================================================================

void EEGoSports::run()
{
    MatrixXd matValue;

    while(m_bIsRunning) {
        if(m_pEEGoSportsProducer->isRunning()) {
            m_mutex.lock();

            // Check impedances - send new impedance values to graphic scene
            if(m_bCheckImpedances)
            {
                /*MatrixXf matValue = m_pRawMatrixBuffer_In->pop();

                for(qint32 i = 0; i < matValue.cols(); ++i)
                    m_pTmsiImpedanceWidget->updateGraphicScene(matValue.col(i).cast<double>());*/
                if(m_qListReceivedSamples.isEmpty() == false) {
                    matValue = m_qListReceivedSamples.takeFirst();
                    //for(qint32 i = 0; i < matValue.cols(); ++i)
                    m_pEEGoSportsImpedanceWidget->updateGraphicScene(matValue.col(0).cast<double>());

                    m_qListReceivedSamples.clear();
                }
            }

            //pop matrix only if the producer thread is running
            if(!m_bCheckImpedances)
            {

                if(m_qListReceivedSamples.isEmpty() == false) {

                    matValue = m_qListReceivedSamples.takeFirst();

                    //Write raw data to fif file
                    if(m_bWriteToFile) {
                        m_pOutfid->write_raw_buffer(matValue, m_cals);
                    }

                    //emit values to real time multi sample array
                    //qDebug()<<"EEGoSports::run - mat size"<<matValue.rows()<<"x"<<matValue.cols();
                    m_pRMTSA_EEGoSports->data()->setValue(matValue);
                }
            }

            m_mutex.unlock();            
        }
    }

    //Close the fif output stream
    if(m_bWriteToFile) {
        m_pOutfid->finish_writing_raw();
        m_bWriteToFile = false;
        m_pTimerRecordingChange->stop();
        m_pActionStartRecording->setIcon(QIcon(":/images/record.png"));
    }

    //qDebug() << "[EEGoSports::run] Thread's run routine finished";
}

