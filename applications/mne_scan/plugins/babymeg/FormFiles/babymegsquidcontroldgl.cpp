//=============================================================================================================
/**
 * @file     babymegsquidcontroldgl.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     May, 2013
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
 * @brief    BabyMEGSQUIDControlDgl class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../babymeg.h"
#include "babymegsquidcontroldgl.h"
#include "ui_babymegsquidcontroldgl.h"

#include <iostream>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace BABYMEGPLUGIN;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

BabyMEGSQUIDControlDgl::BabyMEGSQUIDControlDgl(BabyMEG* p_pBabyMEG,QWidget *parent)
: QDialog(parent)
, m_pBabyMEG(p_pBabyMEG)
, ui(new Ui::BabyMEGSQUIDControlDgl)
{
    connect(this,&BabyMEGSQUIDControlDgl::SendCMDToMEGSource,p_pBabyMEG,&BabyMEG::comFLL);
    connect(m_pBabyMEG,&BabyMEG::dataToSquidCtrlGUI,this,&BabyMEGSQUIDControlDgl::TuneGraphDispProc);
    connect(m_pBabyMEG,&BabyMEG::sendCMDDataToSQUIDControl,this,&BabyMEGSQUIDControlDgl::RcvCMDData);

    ui->setupUi(this);

    // button connects
    connect(ui->m_Qbn_Cancel, &QPushButton::released, this, &BabyMEGSQUIDControlDgl::Cancel);
    connect(ui->m_Qbn_SyncGUI,&QPushButton::released, this, &BabyMEGSQUIDControlDgl::SyncGUI);

    connect(ui->m_Qbn_retune, &QPushButton::released, this, &BabyMEGSQUIDControlDgl::Retune);
    connect(ui->m_Qbn_heat,   &QPushButton::released, this, &BabyMEGSQUIDControlDgl::Heat);
    connect(ui->m_Qbn_atune,  &QPushButton::released, this, &BabyMEGSQUIDControlDgl::Atune);
    connect(ui->m_Qbn_reset,  &QPushButton::released, this, &BabyMEGSQUIDControlDgl::Reset);
    connect(ui->m_Qbn_heatTune, &QPushButton::released, this, &BabyMEGSQUIDControlDgl::HeatTune);
    connect(ui->m_Qbn_save,   &QPushButton::released, this, &BabyMEGSQUIDControlDgl::Save);
    connect(ui->m_Qbn_save1,  &QPushButton::released, this, &BabyMEGSQUIDControlDgl::Save1);
    connect(ui->m_Qbn_groupheat,  &QPushButton::released, this, &BabyMEGSQUIDControlDgl::GroupHeat);
    connect(ui->m_Qbn_last,   &QPushButton::released, this, &BabyMEGSQUIDControlDgl::Last);
    connect(ui->m_Qbn_default,&QPushButton::released, this, &BabyMEGSQUIDControlDgl::Default);

    connect(ui->m_Qbn_tunecheck,&QPushButton::released, this, &BabyMEGSQUIDControlDgl::TuneCheck);
    connect(ui->m_Qbn_amp,    &QPushButton::released, this, &BabyMEGSQUIDControlDgl::Amp);
    connect(ui->m_Qbn_int_reset,&QPushButton::released, this, &BabyMEGSQUIDControlDgl::IntReset);
    connect(ui->m_Qbn_mirco_reset,&QPushButton::released, this, &BabyMEGSQUIDControlDgl::MicroReset);

    // combobox connects
    connect(ui->m_Qcb_commtype,SIGNAL(activated(int)), this,SLOT(CommType(int)));
    connect(ui->m_Qcb_channel,SIGNAL(activated(int)), this,SLOT(ChanSele(int)));
    connect(ui->m_Qcb_opermode,SIGNAL(activated(int)), this,SLOT(OperMode(int)));
    connect(ui->m_Qcb_hp,SIGNAL(activated(int)), this,SLOT(HighPass(int)));
    connect(ui->m_Qcb_lp,SIGNAL(activated(int)), this,SLOT(LowPass1(int)));
    connect(ui->m_Qcb_slew,SIGNAL(activated(int)), this,SLOT(SlewSele(int)));
    connect(ui->m_Qcb_pregain,SIGNAL(activated(int)), this,SLOT(PreGaini(int)));
    connect(ui->m_Qcb_postgain,SIGNAL(activated(int)), this,SLOT(PostGain(int)));
    connect(ui->m_Qcb_auto_reset,SIGNAL(activated(int)), this,SLOT(AutoRest(int)));
    connect(ui->m_Qcb_reset_lock,SIGNAL(activated(int)), this,SLOT(RestLock(int)));

    connect(ui->m_Qcb_bar_graph_select,SIGNAL(activated(int)), this,SLOT(BarGraph(int)));

    // spinbox-button connects
    connect(ui->m_Qbn_heattime,&QPushButton::released, this, &BabyMEGSQUIDControlDgl::HeatTime);
    connect(ui->m_Qbn_cooltime,&QPushButton::released, this, &BabyMEGSQUIDControlDgl::CoolTime);
    connect(ui->m_Qbn_offset,&QPushButton::released, this, &BabyMEGSQUIDControlDgl::AdOffset);
    connect(ui->m_Qbn_bias,&QPushButton::released, this, &BabyMEGSQUIDControlDgl::AdjuBias);
    connect(ui->m_Qbn_mod,&QPushButton::released, this, &BabyMEGSQUIDControlDgl::AdjuModu);

    //connect(ui->m_Qbn_Init,&QPushButton::released, this, &BabyMEGSQUIDControlDgl::Init);

    setModal(false);

    // init the table for dispalying SQUID parameters
    TableRows = 80;
    TableCols = 10;
    ui->tbw_parameters->setRowCount(TableRows);
    ui->tbw_parameters->setColumnCount(TableCols);
    for (int i=0; i<TableCols; i++)
        ui->tbw_parameters->setColumnWidth(i,70);
    //
    ui->tbw_parameters->setHorizontalHeaderLabels(QString("Channel;Value;Channel;Value;Channel;Value;Channel;Value;Channel;Value;Channel;Value").split(";"));

    for (int i=0; i<TableRows;i++)
        for(int j=0;j<TableCols;j++)
            ui->tbw_parameters->setItem(i,j,new QTableWidgetItem(" "));

    ui->tbw_parameters->setCurrentCell(0,0);
    // End of init parameter table

    // init the plot
    initplotflag = false;

    d_timeplot = new Plotter();
    ui->lay_tune->addWidget(d_timeplot);

    //this->Init();
}

//=============================================================================================================

BabyMEGSQUIDControlDgl::~BabyMEGSQUIDControlDgl()
{
    delete d_timeplot;
    delete ui;
}

//=============================================================================================================

void BabyMEGSQUIDControlDgl::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event)
    SendCMD("CANC");
}

//=============================================================================================================

void BabyMEGSQUIDControlDgl::UpdateParaGraph()
{
    int NumRect = m_GUISM.ParaGraph.size();

    for (int i=0; i< NumRect ; i++)
    {
        int ro = floor(i/(TableCols/2));
        int co = i - ro*(TableCols/2);
        ui->tbw_parameters->item(ro,1+2*co)->setText(tr("%1").arg(m_GUISM.ParaGraph.at(i)));
        ui->tbw_parameters->item(ro,0+2*co)->setText(chanNames.at(i));
    }
}

//=============================================================================================================

float BabyMEGSQUIDControlDgl::mmin(MatrixXf tmp,int chan)
{
    int cols = tmp.cols();

    float ret = tmp(chan,0);
    for (int i=0; i<cols; i++)
    {
        if (tmp(chan,i) < ret)
            ret = tmp(chan,i);
    }

    return ret;
}

//=============================================================================================================

float BabyMEGSQUIDControlDgl::mmax(MatrixXf tmp,int chan)
{
    int cols = tmp.cols();

    float ret = tmp(chan,0);
    for (int i=0; i<cols; i++)
    {
        if (tmp(chan,i) > ret)
            ret = tmp(chan,i);
    }

    return ret;
}
void BabyMEGSQUIDControlDgl::TuneGraphDispProc(MatrixXf tmp)
{
//    std::cout << "first ten elements \n" << tmp.block(0,0,1,10) << std::endl;

    int cols = tmp.cols();
    int chanIndx = ui->m_Qcb_channel->currentIndex();//1;//
    // plot the real time data here
    settings.minX = 0.0;
    settings.maxX = cols;
    settings.minY = mmin(tmp,chanIndx);
    settings.maxY = mmax(tmp,chanIndx);
    settings.xlabel = QString("%1 samples/second").arg(m_pBabyMEG->m_pFiffInfo->sfreq) ;
    settings.ylabel = QString("Amplitude [rel. unit]");

    d_timeplot->setPlotSettings(settings);
//    qDebug()<<"minY"<<settings.minY<<"maxY"<<settings.maxY;

    QVector <QPointF> F;

    for(int i=0; i<cols;i++)
        F.append(QPointF(i,tmp(chanIndx,i)));

    d_timeplot->setCurveData(0,F);
    d_timeplot->show();
}

//=============================================================================================================

void BabyMEGSQUIDControlDgl::TuneCheck()
{
    int index;
    index = 0;
    ProcCmd("BUTNTUNECHEC",index,"TuneCheck is processing !");
}

//=============================================================================================================

void BabyMEGSQUIDControlDgl::Amp()
{
    int index;
    index = 0;
    ProcCmd("BUTNDOOOOAMP",index,"Requiring Amp is processing !");
}

//=============================================================================================================

void BabyMEGSQUIDControlDgl::IntReset()
{
    int index;
    index = 0;
    ProcCmd("BUTNINTRESET",index,"IntReset is processing !");
}

//=============================================================================================================

void BabyMEGSQUIDControlDgl::MicroReset()
{
    int index;
    index = 0;
    ProcCmd("BUTNMICRESET",index,"MicroReset is processing !");
}

//=============================================================================================================

void BabyMEGSQUIDControlDgl::Save()
{
    int index;
    index = 0;
    ProcCmd("BUTNDOOOSAVE",index,"Save is processing !");
}

//=============================================================================================================

void BabyMEGSQUIDControlDgl::Save1()
{
    int index;
    index = 0;
    ProcCmd("BUTNDOOSAVE1",index,"Save1 is processing !");
}

//=============================================================================================================

void BabyMEGSQUIDControlDgl::GroupHeat()
{
    int index;
    index = 0;
    ProcCmd("BUTNGROUPHEA",index,"GroupHeat is processing !");
}

//=============================================================================================================

void BabyMEGSQUIDControlDgl::Last()
{
    int index;
    index = 0;
    ProcCmd("BUTNDOOOLAST",index,"Last is processing !");
}

//=============================================================================================================

void BabyMEGSQUIDControlDgl::Default()
{
    int index;
    index = 0;
    ProcCmd("BUTNDDEFAULT",index,"Default is processing !");
}

//=============================================================================================================

void BabyMEGSQUIDControlDgl::Retune()
{
    int index;
    index = 0;
    ProcCmd("BUTNDORETUNE",index,"Retune is processing !");
}

//=============================================================================================================

void BabyMEGSQUIDControlDgl::Heat()
{
    int index;
    index = 0;
    ProcCmd("BUTNDOOOHEAT",index,"Heat is processing !");
}

//=============================================================================================================

void BabyMEGSQUIDControlDgl::Atune()
{
    int index;
    index = 0;
    ProcCmd("BUTNDOOATUNE",index,"Atune is processing !");
}

//=============================================================================================================

void BabyMEGSQUIDControlDgl::Reset()
{
    int index;
    index = 0;
    ProcCmd("BUTNDOIRESET",index,"Reset is processing !");
}

//=============================================================================================================

void BabyMEGSQUIDControlDgl::HeatTune()
{
    int index;
    index = 0;
    ProcCmd("BUTNHEATTUNE",index,"HeatTune is processing !");
}

//=============================================================================================================

void BabyMEGSQUIDControlDgl::AdOffset()
{
    int index;
    index = ui->m_Qsb_offset->value();
    ProcCmd("UPDEADOFFSET",index,"Offset is changed !");
}

//=============================================================================================================

void BabyMEGSQUIDControlDgl::AdjuBias()
{
    int index;
    index = ui->m_Qsb_bias->value();
    ProcCmd("UPDEADJUBIAS",index,"Bias is changed !");
}

//=============================================================================================================

void BabyMEGSQUIDControlDgl::AdjuModu()
{
    int index;
    index = ui->m_Qsb_mod->value();
    ProcCmd("UPDEADJUMODU",index,"Modulation is changed !");
}

//=============================================================================================================

void BabyMEGSQUIDControlDgl::HeatTime()
{
    int index;
    index = ui->m_Qsb_heattime->value();
    ProcCmd("UPDEHEATTIME",index,"Heat Time is changed !");
}

//=============================================================================================================

void BabyMEGSQUIDControlDgl::CoolTime()
{
    int index;
    index = ui->m_Qsb_cooltime->value();
    ProcCmd("UPDECOOLTIME",index,"Cool Time is changed !");
}

//=============================================================================================================

void BabyMEGSQUIDControlDgl::AutoRest(int index)
{
    ProcCmd("UPDEAUTOREST",index,"Auto Reset is changed !");
}

//=============================================================================================================

void BabyMEGSQUIDControlDgl::RestLock(int index)
{
    ProcCmd("UPDERESTLOCK",index,"Reset Lock is changed !");
}

//=============================================================================================================

void BabyMEGSQUIDControlDgl::PreGaini(int index)
{
    ProcCmd("UPDEPREGAINI",++index,"Pre Gain is changed !");
}

//=============================================================================================================

void BabyMEGSQUIDControlDgl::PostGain(int index)
{
    ProcCmd("UPDEPOSTGAIN",++index,"Post Gain is changed !");
}

//=============================================================================================================

void BabyMEGSQUIDControlDgl::SlewSele(int index)
{
    ProcCmd("UPDESLEWSELE",++index,"Slew Selection is changed !");
}

void BabyMEGSQUIDControlDgl::LowPass1(int index)
{
    ProcCmd("UPDELOWPASS1",index,"low pass filter is changed !");
}

//=============================================================================================================

void BabyMEGSQUIDControlDgl::HighPass(int index)
{
    ProcCmd("UPDEHIGHPASS",index,"high pass filter is changed !");
}

//=============================================================================================================

void BabyMEGSQUIDControlDgl::OperMode(int index)
{
    ProcCmd("UPDEOPERMODE",index,"Operate Mode is changed !");
}

//=============================================================================================================

void BabyMEGSQUIDControlDgl::ChanSele(int index)
{
    ProcCmd("UPDECHANSELE",index,"Channel is changed !");
}

//=============================================================================================================

void BabyMEGSQUIDControlDgl::BarGraph(int index)
{
    ProcCmd("UPDEBARGRAPH",index,"Bar graph select is changed !");
}

//=============================================================================================================

void BabyMEGSQUIDControlDgl::CommType(int index)
{
    QString A;
    if (index==0) A = tr("255");
    else if(index == 9) {A=tr("11");}
    else if(index > 0 && index < 9){A = tr("%1").arg(index+130);}

    QString CMDStr = "UPDECOMMTYPE|"+A+"|";
    QString newline = QString("Communication Type is changed!|"+CMDStr+"index"+tr("%1").arg(index));
    UpdateInfo(newline);
    SendCMD(CMDStr);
}

//=============================================================================================================

void BabyMEGSQUIDControlDgl::UpdateInfo(QString newText)
{
    QString content = ui->m_tx_info->toPlainText();
    content = content + "\n" + newText;
    ui->m_tx_info->setText(content);
    ui->m_tx_info->verticalScrollBar()->setValue(ui->m_tx_info->verticalScrollBar()->maximum());
}

//=============================================================================================================

void BabyMEGSQUIDControlDgl::Init()
{
    // Send the init command to labview to call SQUID VI.
    //SendCMD("INIC");
    qDebug()<<"Send init command\n";
    SendCMD("INIT");
}

//=============================================================================================================

void BabyMEGSQUIDControlDgl::Cancel()
{
    SendCMD("CANC");
    this->close();
}

//=============================================================================================================

void BabyMEGSQUIDControlDgl::SyncGUI()
{
    SendCMD("SYNC");
}

//=============================================================================================================

void BabyMEGSQUIDControlDgl::SendCMD(QString CMDSTR)
{
    emit SendCMDToMEGSource(CMDSTR);
}

//=============================================================================================================

void BabyMEGSQUIDControlDgl::RcvCMDData(QByteArray DATA)
{
    QString t_sReply(DATA);

    QString newline = QString("Reply:")+t_sReply;
    UpdateInfo(newline);

    ReplyCmdProc(t_sReply);
}

//=============================================================================================================

void BabyMEGSQUIDControlDgl::ReplyCmdProc(QString sReply)
{

    QString cmd = sReply.left(4);
    sReply.remove(0,4);
    int fcmd = 0;
    QList < QString > tmp;

    if (cmd == "INIT") fcmd = 1;
    else if(cmd=="INIC") fcmd = 2;
    else if(cmd=="SYNC") fcmd = 3;
    else if(cmd=="UPDE") fcmd = 4;
    else if(cmd=="BUTN") fcmd = 5;

    //qDebug() << "Reply Command String ------------------ \n" << sReply ;
    //qDebug() << "Reply Command ------------------ \n" << cmd ;

    switch (fcmd)
    {
    case 1:
        // if the reply is coming from INIT command, then initialize the FLL config.
        tmp = sReply.split("#");

        //qDebug()<<"reply string:\n"<<tmp[0];
        //get the channels
        //load channel information from files
        //tmp[1] = GenChnInfo(tmp[1]);

        InitChannels(tmp[2]);

        //init GUI controls
        InitGUIConfig(tmp[0]);
        UpdateGUI();
        break;
    case 2:
        // get the channels
        InitChannels(sReply);
        break;
    case 3:
        InitGUIConfig(sReply);
        UpdateGUI();
        break;
    case 4://UPDE
        InitGUIConfig(sReply);
        UpdateGUI();
        break;
    case 5://BUTN - button press

        break;
    default:
        break;
    }
}

//=============================================================================================================

QString BabyMEGSQUIDControlDgl::GenChnInfo(QString nChan)
{
    QString chaninfo;
    for (int i=0;i<nChan.toInt();i++){
        chaninfo += "MEG_"+tr("%1").arg(i+1)+"|";
    }
    return chaninfo;
}

//=============================================================================================================

void BabyMEGSQUIDControlDgl::InitChannels(QString sReply)
{
//    QList < QString > tmp = sReply.split("|");
//    ui->m_Qcb_channel->addItems(tmp);

//    chanNames =  sReply.split("|");
//    ui->m_Qcb_channel->addItems(chanNames);

    // select the MEG channels we need
    QList <QString> t_chanNames = sReply.split("|");

    chanNames.clear();

    for (int i=0;i<t_chanNames.size();i++)
    {
        QString T = t_chanNames.at(i);
        if (T.left(3)=="MEG")
            chanNames.push_back(T);
    }

    ui->m_Qcb_channel->addItems(chanNames);
}

//=============================================================================================================

void BabyMEGSQUIDControlDgl::UpdateGUI()
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

    QString newline = QString("Debug: ChannelStat = ")+tr("%1").arg(m_GUISM.ChannelStat);
    UpdateInfo(newline);

    if (m_GUISM.ChannelStat == 0)
        ui->m_Qcb_channel->setEnabled(true);
    else
        ui->m_Qcb_channel->setDisabled(true);

    //set operate mode
    ui->m_Qcb_opermode->setCurrentIndex( m_GUISM.OperMode );

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
    ui->m_Qsb_offset->setValue(m_GUISM.offset);
    //bias
    ui->m_Qsb_bias->setValue(m_GUISM.bias);
    //modulation
    ui->m_Qsb_mod->setValue(m_GUISM.modulation);
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
//    //integrator Reset
//    if(m_GUISM.IntegratorReset == 0)
//        ui->m_Qbn_int_reset->setEnabled(true);
//    else
//        ui->m_Qbn_int_reset->setDisabled(true);
//    //MicroReset
//    if(m_GUISM.MicroReset == 0)
//        ui->m_Qbn_mirco_reset->setEnabled(true);
//    else
//        ui->m_Qbn_mirco_reset->setDisabled(true);

    //set Bar Graph
    //slew
    ui->m_Qcb_bar_graph_select->setCurrentIndex( m_GUISM.BarGraphSelect);

    //Update Bar Graph  -- m_GUISM.ParaGraph
    //std::cout << "Start Update Para Graph ------------------ \n" <<std::endl;
    UpdateParaGraph();
    //std::cout << "End Update Para Graph ------------------ \n" <<std::endl;
}

//=============================================================================================================

void BabyMEGSQUIDControlDgl::InitGUIConfig(QString sReply)
{
    QList < QString > tmp = sReply.split("|");

    int ind = 0;
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

    // get the status of Bar graph select
    m_GUISM.BarGraphSelect = tmp[ind+30].toInt();
    //get the graph wave data
    int count = tmp[ind+31].toInt();
    //set ParaGraph
    if (m_GUISM.ParaGraph.size()>0) m_GUISM.ParaGraph.clear();
    for (int i=0;i<count;i++)
        m_GUISM.ParaGraph.append(tmp[ind+32+i].toFloat());
}

//=============================================================================================================

void BabyMEGSQUIDControlDgl::ProcCmd(QString cmd, int index, QString Info)
{
    QString A = tr("%1").arg(index);
    QString CMDStr = cmd+"|"+A+"|";
    QString newline = Info+"|"+CMDStr+"index"+A;
    UpdateInfo(newline);
    SendCMD(CMDStr);
}
