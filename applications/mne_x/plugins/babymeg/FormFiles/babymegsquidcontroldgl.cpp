//=============================================================================================================
/**
* @file     babymegsquidcontroldgl.cpp
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
* @brief    Contains the implementation of the BabyMEGSQUIDControlDgl class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================
#include <QPen>
#include <QGLWidget>
#include "glwidget_OnDisp.h"

#include <QDebug>


#include <iostream>

#include "../babymeg.h"
#include "babymegsquidcontroldgl.h"
#include "ui_babymegsquidcontroldgl.h"

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace BabyMEGPlugin;

//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

BabyMEGSQUIDControlDgl::BabyMEGSQUIDControlDgl(BabyMEG* p_pBabyMEG,QWidget *parent) :
    QDialog(parent)
  , ui(new Ui::BabyMEGSQUIDControlDgl)
  , m_pBabyMEG(p_pBabyMEG)

{
    connect(this,&BabyMEGSQUIDControlDgl::SendCMDToMEGSource,p_pBabyMEG,&BabyMEG::comFLL);
    connect(m_pBabyMEG,&BabyMEG::DataToSquidCtrlGUI,this,&BabyMEGSQUIDControlDgl::TuneGraphDispProc);
    connect(m_pBabyMEG,&BabyMEG::SendCMDDataToSQUIDControl,this,&BabyMEGSQUIDControlDgl::RcvCMDData);

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

    // init
    initplotflag = false;
    //InitTuneGraph();
    initparaplotflag = false;

    d_timeplot = new plotter();
    ui->lay_tune->addWidget(d_timeplot);

//    d_tuneplot = new plotter();
//    ui->lay_tunepara->addWidget(d_tuneplot);


    this->Init();
    this->StartDisp();
}

BabyMEGSQUIDControlDgl::~BabyMEGSQUIDControlDgl()
{
    delete ui;
}

void BabyMEGSQUIDControlDgl::InitTuneGraph()
{

//    QGraphicsScene * scene = new QGraphicsScene(ui->gv_tunegraph);
//    scene->setItemIndexMethod(QGraphicsScene::NoIndex);
//    scene->setSceneRect(ui->gv_tunegraph->rect());

//    //scene->addText("Hello World");

////    QBrush brush(QColor(0x70, 0x80, 0x50, 255));
////    scene.setBackgroundBrush(brush);

////    scene.setSceneRect(ui->gv_tunegraph->rect());
////    QString newline = QString("InitTuneGraph:")+tr("x=%1").arg(ui->gv_tunegraph->rect().x())
////            +tr("y=%1").arg(ui->gv_tunegraph->rect().y())
////            +tr("width=%1").arg(ui->gv_tunegraph->rect().width())
////            +tr("height=%1").arg(ui->gv_tunegraph->rect().height());

////    UpdateInfo(newline);


//    //QGraphicsLineItem * PolyLine;
//    //PolyLine = new QGraphicsLineItem();
//    //scene->addItem(PolyLine);
//    //PolyLine->setLine(0,ui->gv_tunegraph->rect().height()/2,ui->gv_tunegraph->rect().width(),ui->gv_tunegraph->rect().height()/2);

//    ui->gv_tunegraph->setScene(scene);
//    ui->gv_tunegraph->setRenderHint(QPainter::Antialiasing);
//    ui->gv_tunegraph->setCacheMode(QGraphicsView::CacheBackground);
//    ui->gv_tunegraph->setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
//    //ui->gv_tunegraph->resize(400,600);

}

void BabyMEGSQUIDControlDgl::StartDisp()
{
    //emit SCStart();
}

void BabyMEGSQUIDControlDgl::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event)
    SendCMD("CANC");
    //emit SCStop();
}
void BabyMEGSQUIDControlDgl::UpdateParaGraph()
{

    std::cout << "Update Para Graph \n" << initparaplotflag<< std::endl;

    int NumRect = m_GUISM.ParaGraph.size();

//    float minval = 0.0;
//    float maxval = 0.0;

//    QVector <QPointF> F;

//    for(int i=0; i<NumRect;i++){
//        F.append(QPointF(i,m_GUISM.ParaGraph.at(i)));

//        float t = m_GUISM.ParaGraph.at(i);
//        if ( minval > t ) minval = t;
//        if ( maxval < t ) maxval = t;
//    }
//    // plot tune parameters here
//    settings_tune.minX = 0.0;
//    settings_tune.maxX = NumRect;
//    settings_tune.minY = minval;
//    settings_tune.maxY = maxval;

//    d_tuneplot->setPlotSettings(settings_tune);
//    d_tuneplot->setCurveData(0,F);
//    d_tuneplot->show();

    int hSideSpace = 30;
    int vSideSpace = 30;
    //(x1,y1) and (x2,y2)
    float x1 = 0+1.5*hSideSpace;
    float y1 = 0+vSideSpace;
    float x2 = ui->gv_paragraph->rect().width()-2*hSideSpace;
    float y2 = ui->gv_paragraph->rect().height()-2*vSideSpace;
    // define the x tick
    float dis = (x2-x1)/(1.0*NumRect);

    if (initparaplotflag)
    {
        for (int i=0;i<NumRect;i++)
            PolyRectPtr.at(i)->setRect(x1+i*dis,y2/2,dis/2,m_GUISM.ParaGraph.at(i)+10);
    }
    else
    {
        QGraphicsScene * scene = new QGraphicsScene(ui->gv_paragraph);
        scene->setItemIndexMethod(QGraphicsScene::NoIndex);
        scene->setSceneRect(ui->gv_paragraph->rect());

        // plot the ax
        //x1,y1 -> x2,y1
        scene->addLine(x1,y1,x2,y1,QPen(Qt::black));
        //x2,y1 -> x2,y2
        scene->addLine(x2,y1,x2,y2,QPen(Qt::black));
        //x2,y2 -> x1,y2
        scene->addLine(x2,y2,x1,y2,QPen(Qt::black));
        //x1,y2 -> x1,y1
        scene->addLine(x1,y2,x1,y1,QPen(Qt::black));

        //label
        QGraphicsTextItem * io = new QGraphicsTextItem;
        io->setPos(x2/2,y2+vSideSpace/2);
        io->setPlainText("Channels");
        scene->addItem(io);

        QGraphicsTextItem * io1 = new QGraphicsTextItem;
        io1->setPos(x1-1.5*hSideSpace,y2);
        io1->setPlainText(ui->m_Qcb_bar_graph_select->currentText());
        io1->setRotation(-90);
        scene->addItem(io1);

        // define the y tick
        int NumTick = 3;

        QVector <float> ylabel;
        QVector <float> ycoor;

        ycoor.append(y2);
        ylabel.append(-10);

        ycoor.append(y2/2);
        ylabel.append(0);

        ycoor.append(y1);
        ylabel.append(10);

        for (int j=0;j<NumTick;j++){
            QGraphicsTextItem * yticklabel = new QGraphicsTextItem;
            yticklabel->setPos(x1-hSideSpace/2,ycoor.at(j));
            yticklabel->setPlainText(tr("%1").arg(ylabel.at(j)));
            scene->addItem(yticklabel);
         }

        // plot bars
        for (int i=0;i<NumRect;i++){
            QGraphicsRectItem * barRect;
            barRect = new QGraphicsRectItem(x1+i*dis,y2/2,
                                            dis/2,m_GUISM.ParaGraph.at(i)+10);
            scene->addItem(barRect);
            barRect->setBrush(* new QBrush(Qt::blue));

            PolyRectPtr.append(barRect);

            if (i%50==0){//xlabel
            QGraphicsTextItem * ticklabel = new QGraphicsTextItem;
            ticklabel->setPos(x1+i*dis-dis/2,y2+vSideSpace/8);
            ticklabel->setPlainText(tr("%1").arg(i));
            scene->addItem(ticklabel);
            }

        }
        ui->gv_paragraph->setScene(scene);
        ui->gv_paragraph->setRenderHint(QPainter::Antialiasing);
        ui->gv_paragraph->setCacheMode(QGraphicsView::CacheBackground);
        ui->gv_paragraph->setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);

        initparaplotflag = true;
    }
}

float BabyMEGSQUIDControlDgl::mmin(MatrixXf tmp,int chan)
{
    int cols = tmp.cols();

    float ret = 0.0;
    for (int i=0; i<cols; i++)
    {
        if (ret > tmp(chan,i))
            ret = tmp(chan,i);
    }

    return ret;
}

float BabyMEGSQUIDControlDgl::mmax(MatrixXf tmp,int chan)
{
    int cols = tmp.cols();

    float ret = 0.0;
    for (int i=0; i<cols; i++)
    {
        if (ret < tmp(chan,i))
            ret = tmp(chan,i);
    }

    return ret;
}
void BabyMEGSQUIDControlDgl::TuneGraphDispProc(MatrixXf tmp)
{
    std::cout << "first ten elements \n" << tmp.block(0,0,1,10) << std::endl;

    int cols = tmp.cols();
    int chanIndx = ui->m_Qcb_channel->currentIndex();
    // plot the real time data here
    settings.minX = 0.0;
    settings.maxX = cols;
    settings.minY = mmin(tmp,chanIndx);
    settings.maxY = mmax(tmp,chanIndx);

    d_timeplot->setPlotSettings(settings);


    QVector <QPointF> F;

    for(int i=0; i<cols;i++)
        F.append(QPointF(i,tmp(chanIndx,i)));

    d_timeplot->setCurveData(0,F);
    d_timeplot->show();
////    float * samples = tmp.data();

////    int rows = tmp.rows();
//
////    int wise_type = 1;

//    int NumRect = cols;
//    int hSideSpace = 30;
//    int vSideSpace = 30;
//    //(x1,y1) and (x2,y2)
//    float x1 = 0+1.5*hSideSpace;
//    float y1 = 0+vSideSpace;
//    float x2 = ui->gv_tunegraph->rect().width()-2*hSideSpace;
//    float y2 = ui->gv_tunegraph->rect().height()-2*vSideSpace;
//    // define the x tick
//    float dis = (x2-x1)/(1.0*NumRect);


//

//    float scale = 1;//100000;

//    if (initplotflag){
//        //update the tune graph
//        for(int i=0;i<cols-1;i++){
//            PolyLinePtr.at(i)->setLine(x1+i*dis,scale*tmp(chanIndx,i),x1+(i+1)*dis,scale*tmp(chanIndx,i+1));
//        }
////        if(wise_type==0){ //0 --column wise
////            for(int i=0;i<cols;i++){
////                PolyLinePtr.at(i)->setLine(x1+i*dis,scale*samples[i*rows+chanIndx],x1+(i+1)*dis,scale*samples[(i+1)*rows+chanIndx]);
////            }
////        }
////        else
////        { // 1 -- raw wise
////            for(int i=0;i<cols;i++){
////                PolyLinePtr.at(i)->setLine(x1+i*dis,scale*samples[i+chanIndx*cols],x1+(i+1)*dis,scale*samples[(i+1)+chanIndx*cols]);
////            }
////        }
//    }
//    else
//    {// the first time to plot
//        QGraphicsScene * scene = new QGraphicsScene(ui->gv_tunegraph);
//        scene->setItemIndexMethod(QGraphicsScene::NoIndex);
//        scene->setSceneRect(ui->gv_tunegraph->rect());

//        // plot ax
//        //x1,y1 -> x2,y1
//        scene->addLine(x1,y1,x2,y1,QPen(Qt::black));
//        //x2,y1 -> x2,y2
//        scene->addLine(x2,y1,x2,y2,QPen(Qt::black));
//        //x2,y2 -> x1,y2
//        scene->addLine(x2,y2,x1,y2,QPen(Qt::black));
//        //x1,y2 -> x1,y1
//        scene->addLine(x1,y2,x1,y1,QPen(Qt::black));

//        //label
//        QGraphicsTextItem * io = new QGraphicsTextItem;
//        io->setPos(x2/2,y2+vSideSpace/2);
//        io->setPlainText(QString(tr("%1").arg(fs)+"samples per second"));
//        scene->addItem(io);

//        QGraphicsTextItem * io1 = new QGraphicsTextItem;
//        io1->setPos(x1-1.5*hSideSpace,y2);
//        io1->setPlainText("Amplitude");
//        io1->setRotation(-90);
//        scene->addItem(io1);

//        // plot lines
//        for (int i=0;i< cols;i++){
//            QGraphicsLineItem * PolyLine;
//            PolyLine = new QGraphicsLineItem();
//            scene->addItem(PolyLine);

//            PolyLinePtr.append(PolyLine);

//            if (i%500==0){//x-ticklabel
//            QGraphicsTextItem * ticklabel = new QGraphicsTextItem;
//            ticklabel->setPos(x1+i*dis-dis/2,y2+vSideSpace/8);
//            ticklabel->setPlainText(tr("%1").arg(i));
//            scene->addItem(ticklabel);
//            }
//        }
//        ui->gv_tunegraph->setScene(scene);
//        ui->gv_tunegraph->setRenderHint(QPainter::Antialiasing);
//        ui->gv_tunegraph->setCacheMode(QGraphicsView::CacheBackground);
//        ui->gv_tunegraph->setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
//        initplotflag = true;
//    }

//    QString newline = QString("New data arrived");
//    UpdateInfo(newline);

}


void BabyMEGSQUIDControlDgl::TuneCheck()
{
    int index;
    index = 0;
    ProcCmd("BUTNTUNECHEC",index,"TuneCheck is processing !");
}
void BabyMEGSQUIDControlDgl::Amp()
{
    int index;
    index = 0;
    ProcCmd("BUTNDOOOOAMP",index,"Requiring Amp is processing !");
}
void BabyMEGSQUIDControlDgl::IntReset()
{
    int index;
    index = 0;
    ProcCmd("BUTNINTRESET",index,"IntReset is processing !");
}
void BabyMEGSQUIDControlDgl::MicroReset()
{
    int index;
    index = 0;
    ProcCmd("BUTNMICRESET",index,"MicroReset is processing !");
}

void BabyMEGSQUIDControlDgl::Save()
{
    int index;
    index = 0;
    ProcCmd("BUTNDOOOSAVE",index,"Save is processing !");
}

void BabyMEGSQUIDControlDgl::Save1()
{
    int index;
    index = 0;
    ProcCmd("BUTNDOOSAVE1",index,"Save1 is processing !");
}

void BabyMEGSQUIDControlDgl::GroupHeat()
{
    int index;
    index = 0;
    ProcCmd("BUTNGROUPHEA",index,"GroupHeat is processing !");
}

void BabyMEGSQUIDControlDgl::Last()
{
    int index;
    index = 0;
    ProcCmd("BUTNDOOOLAST",index,"Last is processing !");
}

void BabyMEGSQUIDControlDgl::Default()
{
    int index;
    index = 0;
    ProcCmd("BUTNDDEFAULT",index,"Default is processing !");
}


void BabyMEGSQUIDControlDgl::Retune()
{
    int index;
    index = 0;
    ProcCmd("BUTNDORETUNE",index,"Retune is processing !");
}

void BabyMEGSQUIDControlDgl::Heat()
{
    int index;
    index = 0;
    ProcCmd("BUTNDOOOHEAT",index,"Heat is processing !");
}
void BabyMEGSQUIDControlDgl::Atune()
{
    int index;
    index = 0;
    ProcCmd("BUTNDOOATUNE",index,"Atune is processing !");
}
void BabyMEGSQUIDControlDgl::Reset()
{
    int index;
    index = 0;
    ProcCmd("BUTNDOIRESET",index,"Reset is processing !");
}
void BabyMEGSQUIDControlDgl::HeatTune()
{
    int index;
    index = 0;
    ProcCmd("BUTNHEATTUNE",index,"HeatTune is processing !");
}


void BabyMEGSQUIDControlDgl::AdOffset()
{
    int index;
    index = ui->m_Qsb_offset->value();
    ProcCmd("UPDEADOFFSET",index,"Offset is changed !");

}

void BabyMEGSQUIDControlDgl::AdjuBias()
{
    int index;
    index = ui->m_Qsb_bias->value();
    ProcCmd("UPDEADJUBIAS",index,"Bias is changed !");

}

void BabyMEGSQUIDControlDgl::AdjuModu()
{
    int index;
    index = ui->m_Qsb_mod->value();
    ProcCmd("UPDEADJUMODU",index,"Modulation is changed !");

}

void BabyMEGSQUIDControlDgl::HeatTime()
{
    int index;
    index = ui->m_Qsb_heattime->value();
    ProcCmd("UPDEHEATTIME",index,"Heat Time is changed !");
}

void BabyMEGSQUIDControlDgl::CoolTime()
{
    int index;
    index = ui->m_Qsb_cooltime->value();
    ProcCmd("UPDECOOLTIME",index,"Cool Time is changed !");
}

void BabyMEGSQUIDControlDgl::AutoRest(int index)
{
    ProcCmd("UPDEAUTOREST",index,"Auto Reset is changed !");
}
void BabyMEGSQUIDControlDgl::RestLock(int index)
{
    ProcCmd("UPDERESTLOCK",index,"Reset Lock is changed !");
}

void BabyMEGSQUIDControlDgl::PreGaini(int index)
{
    ProcCmd("UPDEPREGAINI",index,"Pre Gain is changed !");
}

void BabyMEGSQUIDControlDgl::PostGain(int index)
{
    ProcCmd("UPDEPOSTGAIN",index,"Post Gain is changed !");
}


void BabyMEGSQUIDControlDgl::SlewSele(int index)
{
    ProcCmd("UPDESLEWSELE",index,"Slew Selection is changed !");
}


void BabyMEGSQUIDControlDgl::LowPass1(int index)
{
    ProcCmd("UPDELOWPASS1",index,"low pass filter is changed !");
}


void BabyMEGSQUIDControlDgl::HighPass(int index)
{
    ProcCmd("UPDEHIGHPASS",index,"high pass filter is changed !");
}


void BabyMEGSQUIDControlDgl::OperMode(int index)
{
    ProcCmd("UPDEOPERMODE",index,"Operate Mode is changed !");
}

void BabyMEGSQUIDControlDgl::ChanSele(int index)
{
    ProcCmd("UPDECHANSELE",index,"Channel is changed !");
}

void BabyMEGSQUIDControlDgl::BarGraph(int index)
{
    ProcCmd("UPDEBARGRAPH",index,"Bar graph select is changed !");
}

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

void BabyMEGSQUIDControlDgl::UpdateInfo(QString newText)
{
    QString content = ui->m_tx_info->toPlainText();
    content = content + "\n" + newText;
    ui->m_tx_info->setText(content);
    ui->m_tx_info->verticalScrollBar()->setValue(ui->m_tx_info->verticalScrollBar()->maximum());
}

void BabyMEGSQUIDControlDgl::Init()
{
    // Send the init command to labview to call SQUID VI.
    //SendCMD("INIC");
    SendCMD("INIT");
}

void BabyMEGSQUIDControlDgl::Cancel()
{
    SendCMD("CANC");
    //emit SCStop();
    this->close();
}

void BabyMEGSQUIDControlDgl::SyncGUI()
{
    SendCMD("SYNC");
}

void BabyMEGSQUIDControlDgl::SendCMD(QString CMDSTR)
{
    emit SendCMDToMEGSource(CMDSTR);
}

void BabyMEGSQUIDControlDgl::RcvCMDData(QByteArray DATA)
{
    QString t_sReply(DATA);

    //ui->m_tx_info->setText(QString("Reply:")+t_sReply);
    QString newline = QString("Reply:")+t_sReply;
    UpdateInfo(newline);

    ReplyCmdProc(t_sReply);
}

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

//    qDebug() << "Reply Command String ------------------ \n" << sReply ;
//    qDebug() << "Reply Command ------------------ \n" << cmd ;

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
QString BabyMEGSQUIDControlDgl::GenChnInfo(QString nChan)
{
    QString chaninfo;
    for (int i=0;i<nChan.toInt();i++){
        chaninfo += "MEG_"+tr("%1").arg(i+1)+"|";
    }
    return chaninfo;
}

void BabyMEGSQUIDControlDgl::InitChannels(QString sReply)
{
    QList < QString > tmp = sReply.split("|");
    ui->m_Qcb_channel->addItems(tmp);

}

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
    ui->m_Qcb_pregain->setCurrentIndex( m_GUISM.PreGain );
    //postgain
    ui->m_Qcb_postgain->setCurrentIndex( m_GUISM.PostGain );
    //slew
    ui->m_Qcb_slew->setCurrentIndex( m_GUISM.Slew);

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

    //set Bar Graph
    //slew
    ui->m_Qcb_bar_graph_select->setCurrentIndex( m_GUISM.BarGraphSelect);

    //Update Bar Graph  -- m_GUISM.ParaGraph
    std::cout << "Start Update Para Graph ------------------ \n" <<std::endl;
    UpdateParaGraph();
    std::cout << "End Update Para Graph ------------------ \n" <<std::endl;

}
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

void BabyMEGSQUIDControlDgl::ProcCmd(QString cmd, int index, QString Info)
{
    QString A = tr("%1").arg(index);
    QString CMDStr = cmd+"|"+A+"|";
    QString newline = Info+"|"+CMDStr+"index"+A;
    UpdateInfo(newline);
    SendCMD(CMDStr);
}
