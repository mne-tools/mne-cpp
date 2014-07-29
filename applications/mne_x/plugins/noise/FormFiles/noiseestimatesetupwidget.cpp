//=============================================================================================================
/**
* @file     NoiseEstimatesetupwidget.cpp
* @author   Limin Sun <liminsun@nmr.mgh.haravrd.edu>
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     July, 2014
*
* @section  LICENSE
*
* Copyright (C) 2014, Limin Sun, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Contains the implementation of the NoiseEstimateSetupWidget class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "noiseestimatesetupwidget.h"
#include "../noise_estimate.h"

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace NoiseEstimatePlugin;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

NoiseEstimateSetupWidget::NoiseEstimateSetupWidget(NoiseEstimate* toolbox, QWidget *parent)
: QWidget(parent)
, m_pNoiseEstimate(toolbox)
{
    ui.setupUi(this);

//    connect(ui.m_qPushButton_About, SIGNAL(released()), this, SLOT(showAboutDialog()));
//    connect(ui.m_qcbChannel,SIGNAL(currentIndexChanged(int)), this, SLOT(chgChannelInx));
//    connect(ui.m_cb_nFFT,SIGNAL(currentTextChanged(QString)), this, SLOT(chgnFFT));

//    d_timeplot = new plotter();
//    ui.m_layNoise->addWidget(d_timeplot);

}


//*************************************************************************************************************

NoiseEstimateSetupWidget::~NoiseEstimateSetupWidget()
{

}
void NoiseEstimateSetupWidget::init(qint32 nFFT, double fs)
{

    qDebug() << "Setup Noise Parameters:"<<nFFT<<","<<fs;

    int idx = 0;
    if(nFFT==512) idx = 1;
    else if(nFFT==1024) idx = 2;
    else if(nFFT==2048) idx = 3;
    else if(nFFT==4096) idx = 4;
    else if(nFFT==8192) idx = 5;
    else if(nFFT==16384) idx = 6;

    ui.m_cb_nFFT->setCurrentIndex(idx);

    ui.m_qlabel_fs->setText(QString("%1").arg(fs));
}

void NoiseEstimateSetupWidget::Update(MatrixXf data)
{

}

void NoiseEstimateSetupWidget::chgnFFT(QString tx)
{

}

void NoiseEstimateSetupWidget::Replot(/*MatrixXd tmp*/)
{
    /*
    int cols = tmp.cols();
    int chanIndx = 1;// ui->m_Qcb_channel->currentIndex();//
    // plot the real time data here
    settings.minX = 0.0;
    settings.maxX = cols;
    settings.minY = mmin(tmp,chanIndx);
    settings.maxY = mmax(tmp,chanIndx);
    settings.xlabel = QString("%1 samples/second").arg(1000) ;
    settings.ylabel = QString("Power Spectrum [DB/Hz]");

    d_timeplot->setPlotSettings(settings);
//    qDebug()<<"minY"<<settings.minY<<"maxY"<<settings.maxY;

    QVector <QPointF> F;

    for(int i=0; i<cols;i++)
        F.append(QPointF(i,tmp(chanIndx,i)));

    d_timeplot->setCurveData(0,F);
    d_timeplot->show();
*/
}

//float NoiseEstimateSetupWidget::mmin(MatrixXd tmp,int chan)
//{
//    int cols = tmp.cols();

//    float ret = tmp(chan,0);
//    for (int i=0; i<cols; i++)
//    {
//        if (tmp(chan,i) < ret)
//            ret = tmp(chan,i);
//    }

//    return ret;
//}

//float NoiseEstimateSetupWidget::mmax(MatrixXd tmp,int chan)
//{
//    int cols = tmp.cols();

//    float ret = tmp(chan,0);
//    for (int i=0; i<cols; i++)
//    {
//        if (tmp(chan,i) > ret)
//            ret = tmp(chan,i);
//    }

//    return ret;
//}
