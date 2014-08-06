//=============================================================================================================
/**
* @file     mnertclientsquidcontroldgl.cpp
* @author   Limin Sun <liminsun@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     May, 2013
*
* @section  LICENSE
*
* Copyright (C) 2013, Limin Sun and Matti Hamalainen. All rights reserved.
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
* @brief    Contains the implementation of the mnertclientSQUIDControlDgl class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../mnertclient.h"
#include "mnertclientsquidcontroldgl.h"
#include "ui_mnertclientsquidcontroldgl.h"

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MneRtClientPlugin;

//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

mnertclientSQUIDControlDgl::mnertclientSQUIDControlDgl(MneRtClient* p_pMneRtClient,QWidget *parent) :
    QDialog(parent),
    ui(new Ui::mnertclientSQUIDControlDgl),
    m_pMneRtClient(p_pMneRtClient)

{
    ui->setupUi(this);

    // retune connect
    connect(ui->m_Qbn_retune, &QPushButton::released, this, &mnertclientSQUIDControlDgl::SendRetune);
    connect(ui->m_Qbn_Cancel, &QPushButton::released, this, &mnertclientSQUIDControlDgl::Cancel);
    connect(ui->m_Qbn_SyncGUI,&QPushButton::released, this, &mnertclientSQUIDControlDgl::SyncGUI);

    // init
    Init();
}

mnertclientSQUIDControlDgl::~mnertclientSQUIDControlDgl()
{
    delete ui;
}
void mnertclientSQUIDControlDgl::Init()
{
    // Send the init command to labview to call SQUID VI.
    //SendCMD("INIC");
    SendCMD("INIT");
}

void mnertclientSQUIDControlDgl::SendRetune()
{
    SendCMD("RETU");
}
void mnertclientSQUIDControlDgl::Cancel()
{
    SendCMD("CANC");
    this->close();
}

void mnertclientSQUIDControlDgl::SyncGUI()
{
    SendCMD("SYNC");
}

void mnertclientSQUIDControlDgl::SendCMD(QString CMDSTR)
{

    if(m_pMneRtClient->m_bCmdClientIsConnected)
    {
    //set the control field in FLL JSON value as CMDSTR
    (*m_pMneRtClient->m_pRtCmdClient)["FLL"].pValues()[0].setValue(CMDSTR);
    (*m_pMneRtClient->m_pRtCmdClient)["FLL"].send();

    this->ui->m_tx_info->setText(CMDSTR);
    // Read reply
    QString t_sReply = m_pMneRtClient->m_pRtCmdClient->readAvailableData();

    this->ui->m_tx_info->setText(QString("Reply:")+t_sReply);

    ReplyCmdProc(t_sReply);

    }
}
void mnertclientSQUIDControlDgl::ReplyCmdProc(QString sReply)
{

    QString cmd = sReply.left(4);
    sReply.remove(0,4);
    int fcmd = 0;
    QList < QString > tmp;

    if (cmd == "INIT") fcmd = 1;
    else if(cmd=="INIC") fcmd = 2;
    else if(cmd=="SYNC") fcmd = 3;

    switch (fcmd)
    {
    case 1:
        // if the reply is coming from INIT command, then initialize the FLL config.
        tmp = sReply.split("#");
        //get the channels
        //load channel information from files
        tmp[1] = GenChnInfo(tmp[1]);
        InitChannels(tmp[1]);
        //init GUI controls
        InitGUIConfig(tmp[0]);
        UpdateGUI();
        break;
    case 2:
        // get the channels
        InitChannels(sReply);
        break;
    case 3:
        InitGUIConfig(tmp[0]);
        UpdateGUI();
        break;
    default:
        break;
    }
}
QString mnertclientSQUIDControlDgl::GenChnInfo(QString nChan)
{
    QString chaninfo;
    for (int i=0;i<nChan.toInt();i++){
        chaninfo += "MEG_"+tr("%1").arg(i+1)+"|";
    }
    return chaninfo;
}

void mnertclientSQUIDControlDgl::InitChannels(QString sReply)
{
    QList < QString > tmp = sReply.split("|");
    ui->m_Qcb_channel->addItems(tmp);

}

void mnertclientSQUIDControlDgl::UpdateGUI()
{
    // according to m_GUISM to set the items status in the GUI
    //set CommType 255-group, 11-single channel
    int curindex = 0;

    switch (m_GUISM.CommType){
    case 255:
        curindex = 0;
        break;
    case 11:
        curindex = 9;
        break;
    default:
        curindex = m_GUISM.CommType - 130;
        break;
    }

    ui->m_Qcb_commtype->setCurrentIndex( curindex );

    //set Channel
    ui->m_Qcb_channel->setCurrentIndex( m_GUISM.ChannelSel );
    if (m_GUISM.ChannelStat == 0)
        ui->m_Qcb_channel->setEnabled(true);
    else
        ui->m_Qcb_channel->setDisabled(true);

    //set operate mode
    ui->m_Qcb_opermode->setCurrentIndex( m_GUISM.OperMode-1 );

    //set Retune
    if(m_GUISM.Retune == 0)
        ui->m_Qbn_retune->setEnabled(true);
    else
        ui->m_Qbn_retune->setDisabled(true);
    //set Heat this
    if(m_GUISM.HeatThis == 0)
        ui->m_Qbn_heat->setEnabled(true);
    else
        ui->m_Qbn_heat->setDisabled(true);

    //Atune
    if(m_GUISM.Atune == 0)
        ui->m_Qbn_atune->setEnabled(true);
    else
        ui->m_Qbn_atune->setDisabled(true);

    //reset
    if(m_GUISM.Reset == 0)
        ui->m_Qbn_reset->setEnabled(true);
    else
        ui->m_Qbn_reset->setDisabled(true);
    //Heat&Tune
    if(m_GUISM.HeatAndTune == 0)
        ui->m_Qbn_heatTune->setEnabled(true);
    else
        ui->m_Qbn_heatTune->setDisabled(true);

    //save
    if(m_GUISM.Save == 0)
        ui->m_Qbn_save->setEnabled(true);
    else
        ui->m_Qbn_save->setDisabled(true);

    //_save --- %$save
    if(m_GUISM._Save == 0)
        ui->m_Qbn_save1->setEnabled(true);
    else
        ui->m_Qbn_save1->setDisabled(true);

    //Group Heat
    if(m_GUISM.GroupHeat == 0)
        ui->m_Qbn_groupheat->setEnabled(true);
    else
        ui->m_Qbn_groupheat->setDisabled(true);

    //Last
    if(m_GUISM.Last == 0)
        ui->m_Qbn_last->setEnabled(true);
    else
        ui->m_Qbn_last->setDisabled(true);

    //Default
    if(m_GUISM.Default == 0)
        ui->m_Qbn_default->setEnabled(true);
    else
        ui->m_Qbn_default->setDisabled(true);

    // high pass
    ui->m_Qcb_hp->setCurrentIndex( m_GUISM.HighPass );
    // low pass
    ui->m_Qcb_lp->setCurrentIndex( m_GUISM.LowPass );
    //pregain
    ui->m_Qcb_pregain->setCurrentIndex( m_GUISM.PreGain-1 );
    //postgain
    ui->m_Qcb_postgain->setCurrentIndex( m_GUISM.PostGain-1 );
    //slew
    ui->m_Qcb_slew->setCurrentIndex( m_GUISM.Slew-1);

    //heattime
    ui->m_Qsb_heattime->setValue(m_GUISM.HeatTime);
    //cooltime
    ui->m_Qsb_cooltime->setValue(m_GUISM.CoolTime);
    //offset
    ui->m_Qdsb_offset->setValue(m_GUISM.offset);
    //bias
    ui->m_Qdsb_bias->setValue(m_GUISM.bias);
    //modulation
    ui->m_Qdsb_mod->setValue(m_GUISM.modulation);
    //autoreset
    ui->m_Qcb_auto_reset->setCurrentIndex(m_GUISM.AutoRest);
    //resetlock
    ui->m_Qcb_reset_lock->setCurrentIndex(m_GUISM.ResetLock);

    //tunecheck
    if(m_GUISM.TuneCheck == 0)
        ui->m_Qbn_tunecheck->setEnabled(true);
    else
        ui->m_Qbn_tunecheck->setDisabled(true);
    //amp
    if(m_GUISM.Amp == 0)
        ui->m_Qbn_amp->setEnabled(true);
    else
        ui->m_Qbn_amp->setDisabled(true);
    //integrator Reset
    if(m_GUISM.IntegratorReset == 0)
        ui->m_Qbn_int_reset->setEnabled(true);
    else
        ui->m_Qbn_int_reset->setDisabled(true);
    //MicroReset
    if(m_GUISM.MicroReset == 0)
        ui->m_Qbn_mirco_reset->setEnabled(true);
    else
        ui->m_Qbn_mirco_reset->setDisabled(true);
}
void mnertclientSQUIDControlDgl::InitGUIConfig(QString sReply)
{
    QList < QString > tmp = sReply.split("|");

    int ind = 0;
//    int nFLL = tmp[1].toInt();
//     for (int i=0; i<nFLL; i++)
//    {
//        FLLPara t;
//        t.Bias = 0;
//        t.Mod = 0;
//        t.Mode = 0;
//        t.LoPass = 0;
//        t.LPBW = 0;
//        t.HiPass = 0;
//        t.HPBW =0;
//        t.offset =0;
//        t.PreGain = 1;
//        t.PostGain =1;
//        t.slew =0;

//        t.AutoReset =0;
//        t.ResetLock =0;
//        t.channame = "MEG_"+tr("%1").arg(i+1);
//        m_FLLConfig.m_Fll.push_back(t);
//    }

    // Set the current status of GUI Status Machine
    m_GUISM.CommType = tmp[ind].toInt();
    m_GUISM.ChannelSel = tmp[ind+1].toInt();
    m_GUISM.ChannelStat = tmp[ind+2].toInt();
    m_GUISM.OperMode = tmp[ind+3].toInt();

    m_GUISM.Retune = tmp[ind+4].toInt();
    m_GUISM.HeatThis = tmp[ind+5].toInt();
    m_GUISM.Atune = tmp[ind+6].toInt();
    m_GUISM.Reset = tmp[ind+7].toInt();
    m_GUISM.HeatAndTune = tmp[ind+8].toInt();
    m_GUISM.GroupHeat = tmp[ind+9].toInt();
    m_GUISM.Last = tmp[ind+10].toInt();
    m_GUISM.Default = tmp[ind+11].toInt();
    m_GUISM.Save = tmp[ind+12].toInt();
    m_GUISM._Save = tmp[ind+13].toInt();


    m_GUISM.HighPass = tmp[ind+14].toInt();
    m_GUISM.LowPass = tmp[ind+15].toInt();
    m_GUISM.Slew    = tmp[ind+16].toInt();
    m_GUISM.PreGain = tmp[ind+17].toInt();
    m_GUISM.PostGain= tmp[ind+18].toInt();
    m_GUISM.AutoRest= tmp[ind+19].toInt();
    m_GUISM.ResetLock=tmp[ind+20].toInt();

    m_GUISM.offset = tmp[ind+21].toFloat();
    m_GUISM.bias   = tmp[ind+22].toFloat();
    m_GUISM.modulation = tmp[ind+23].toFloat();
    m_GUISM.HeatTime = tmp[ind+24].toFloat();
    m_GUISM.CoolTime = tmp[ind+25].toFloat();

    m_GUISM.TuneCheck = tmp[ind+26].toInt();
    m_GUISM.Amp = tmp[ind+27].toInt();
    m_GUISM.IntegratorReset = tmp[ind+28].toInt();
    m_GUISM.MicroReset = tmp[ind+29].toInt();

}

