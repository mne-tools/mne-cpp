//=============================================================================================================
/**
 * @file     realtimeneurofeedbackwidget.cpp
 * @author   Simon Marxgut <simon.marxgut@umit-tirol.at>
 * @since    0.1.0
 * @date     November, 2021
 *
 * @section  LICENSE
 *
 * Copyright (C) 2021, Simon Marxgut. All rights reserved.
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
 * @brief    Definition of the RealTimeNeurofeedbackWidget Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "realtimeneurofeedbackwidget.h"

#include <disp/viewers/filterdesignview.h>
#include <disp/viewers/channelselectionview.h>
#include <disp/viewers/helpers/channelinfomodel.h>
#include <disp/viewers/rtfiffrawview.h>
#include <disp/viewers/scalingview.h>
#include <disp/viewers/projectorsview.h>
#include <disp/viewers/filtersettingsview.h>
#include <disp/viewers/compensatorview.h>
#include <disp/viewers/spharasettingsview.h>
#include <disp/viewers/fiffrawviewsettings.h>
#include <disp/viewers/triggerdetectionview.h>

#include <scMeas/realtimeneurofeedbackresult.h>

#include <rtprocessing/helpers/filterkernel.h>


//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QAction>
#include <QDate>
#include <QVBoxLayout>
#include <QCheckBox>
#include <QDir>
#include <QSettings>
#include <QToolBar>
#include <QPainter>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace SCDISPLIB;
using namespace SCMEASLIB;
using namespace DISPLIB;
using namespace RTPROCESSINGLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RealTimeNeurofeedbackWidget::RealTimeNeurofeedbackWidget(QSharedPointer<QTime> &pTime,
                                                               QWidget* parent)
: MeasurementWidget(parent)
, m_iMaxFilterTapSize(-1)
{
    Q_UNUSED(pTime)

    qRegisterMetaType<QMap<int,QList<QPair<int,double> > > >();

}

//=============================================================================================================

RealTimeNeurofeedbackWidget::~RealTimeNeurofeedbackWidget()
{
    QSettings settings("MNECPP");

    if(m_pChannelDataView && m_pRTNR) {
        settings.setValue(QString("RTNW/showHideBad"), m_pChannelDataView->getBadChannelHideStatus());
    }
}

//=============================================================================================================

void RealTimeNeurofeedbackWidget::update(SCMEASLIB::Measurement::SPtr pMeasurement)
{

    if(!m_pRTNR) {
        m_pRTNR = qSharedPointerDynamicCast<RealTimeNeurofeedbackResult>(pMeasurement);
    }

    if(m_pRTNR) {
        if(m_pRTNR->isChInit() && !m_pFiffInfo) {
            m_pFiffInfo = m_pRTNR->info();
            m_iMaxFilterTapSize = m_pRTNR->getNeurofeedback().first().cols();


            if(!m_bDisplayWidgetsInitialized) {
                initDisplayControllWidgets();
                if(m_pRTNR->getNeuroOutput()=="Frequency"){
                    if(m_pRTNR->getallCh() == true || (unsigned int)m_pRTNR->getSlider()>m_pRTNR->getNumChannels()){
                       m_pRTNR->setSlider(m_pRTNR->getNumChannels());
                    }
                    QStringList startchannels;
                    for(int k = 0; k<m_pRTNR->getSlider(); ++k){
                        startchannels.append(m_pRTNR->chInfo()[k].getChannelName());
                    }
                    m_pChannelSelectionView->selectChannels(startchannels);

                }
                else if(m_pRTNR->getNeuroOutput()=="Balloon"){
                    QStringList balloonchannel;
                    balloonchannel.append(m_pRTNR->chInfo()[0].getChannelName());
                    m_pChannelSelectionView->selectChannels(balloonchannel);
                }
            }
        } else if (!m_pRTNR->getNeurofeedback().isEmpty()) {
            m_matNeurofeedbackData = m_pRTNR->getNeurofeedback().last();
            QStringList selectedchannels;
            selectedchannels = m_pChannelSelectionView->getSelectedChannels();
            if (m_pRTNR->getNeuroOutput() == "Classifier"){
                bool noClass = true;
                for(int i = 0; i<m_pRTNR->getNumbofClass(); ++i){
                    unsigned int j = 0;
                    if(selectedchannels.isEmpty()){
                        updateLabel("no Channels selected", "red");
                    }
                    else{
                        while(selectedchannels[0]!=m_pRTNR->chInfo()[j].getChannelName()){
                            ++j;
                            if(j==m_pRTNR->getNumChannels()){
                                updateLabel("no Channel available", "red");
                                break;
                            }
                        }
                        if(j < m_matNeurofeedbackData.rows()){
                            if(m_matNeurofeedbackData(j,0) == m_pRTNR->getClassI(i)){
                                if(m_pRTNR->getClassImg(i).isNull()){
                                    updateLabel(m_pRTNR->getClassL(i), m_pRTNR->getClassCol(i));
                                }
                                else{
                                    updateImage(m_pRTNR->getClassImg(i));
                                }
                                noClass = false;
                            }
                            if(noClass == true){
                                updateLabel("no Classification available", "black");
                            }
                        }
                        else{
                            updateLabel("no Channel available", "red");
                        }
                    }
                }                
             }
            else if (m_pRTNR->getNeuroOutput() == "Frequency"){
                clearLayout(m_pLayout);
                m_pRTNR->setSlider(selectedchannels.size());

                if(selectedchannels.isEmpty()){
                    updateLabel("no Channels selected", "red");
                }
                else{
                    for(int i = 0; i<m_pRTNR->getSlider(); ++i){
                        unsigned int j = 0;
                        bool channelavail = true;
                        while(selectedchannels[i]!=m_pRTNR->chInfo()[j].getChannelName()){
                            ++j;
                            if(j==m_pRTNR->getNumChannels()){
                                channelavail = false;
                                break;
                            }
                        }

                        bool resetMinimum = false;
                        if(j < m_matNeurofeedbackData.rows() && channelavail == true){
                            if(m_pRTNR->getMaxAutoScale()==true){
                                for(int l = 0; l<selectedchannels.size(); l++){
                                    for(unsigned int m = 0; m<m_pRTNR->getNumChannels(); m++){
                                        if(selectedchannels[l]== m_pRTNR->chInfo()[m].getChannelName()){
                                            if(m_pRTNR->getResetAutoscale()==true){
                                                m_pRTNR->setFMax(m_matNeurofeedbackData(m,0));
                                                m_pRTNR->setResetAutoScale(false);
                                                resetMinimum = true;
                                            }
                                            else if((m_pRTNR->getNeurofeedback().last())(m,0)>m_pRTNR->getFMax()){
                                                m_pRTNR->setFMax(m_matNeurofeedbackData(m,0));
                                            }
                                        }
                                    }
                                }
                            }
                            if(m_pRTNR->getMinAutoScale()==true){
                                for(int l = 0; l<selectedchannels.size(); l++){
                                    for(unsigned int m = 0; m<m_pRTNR->getNumChannels(); m++){
                                        if(selectedchannels[l]== m_pRTNR->chInfo()[m].getChannelName()){
                                            if(resetMinimum == true || m_pRTNR->getResetAutoscale()==true){
                                                m_pRTNR->setFMin(m_matNeurofeedbackData(m,0));
                                                resetMinimum = false;
                                                m_pRTNR->setResetAutoScale(false);
                                            }
                                            else if((m_pRTNR->getNeurofeedback().last())(m,0)<m_pRTNR->getFMin()){
                                                m_pRTNR->setFMin(m_matNeurofeedbackData(m,0));
                                            }
                                        }
                                    }
                                }
                            }

                            addSlider(i, m_matNeurofeedbackData(j,0), m_pRTNR->getFMax(), m_pRTNR->getFMin());


                            QLabel* Label= new QLabel(this);
                            Label->setText(m_pRTNR->chInfo()[j].getChannelName());
                            Label->setAlignment(Qt::AlignCenter);
                            QFont font;
                            font.setBold(true);
                            font.setPointSize(20);
                            Label->setFont(font);
                            m_pLayout->addWidget(Label, 6, 2*i, 1, 2);
                        }
                        else{
                            QLabel* Label= new QLabel(this);
                            Label->setText("not available");
                            Label->setAlignment(Qt::AlignCenter);
                            QFont font;
                            font.setBold(true);
                            font.setPointSize(20);
                            Label->setFont(font);
                            m_pLayout->addWidget(Label, 3, i+1);
                            break;
                        }

                    }
                }
                m_pLayout->setContentsMargins(3,0,3,0);
                m_pLayout->update();
             }
            else if(m_pRTNR->getNeuroOutput() == "Balloon"){

                QLabel* Label= new QLabel(this);
                QPixmap background = m_pRTNR->getImgBackground();
                Label->setPixmap(background.scaledToHeight(300, Qt::FastTransformation));
                Label->setAlignment(Qt::AlignCenter);

                QLabel* Label1 = new QLabel(this);
                QPixmap Balloon = m_pRTNR->getImgObject();

                Label1->setPixmap(Balloon.scaled(40,40, Qt::KeepAspectRatio));
                Label1->setAlignment(Qt::AlignCenter);

                int maximum = m_pRTNR->getBMax();
                int minimum = m_pRTNR->getBMin();
                if(maximum <= minimum){
                    maximum = minimum + 1;
                }
                int span = 300;
                unsigned int i = 0;
                while(selectedchannels[0]!=m_pRTNR->chInfo()[i].getChannelName()){
                    ++i;
                    if(i==m_pRTNR->getNumChannels()){
                        i = 0;
                        break;
                    }
                }
                int heightballoon = qRound(-span/(maximum-minimum)*m_matNeurofeedbackData(i,0)+250);
                if (heightballoon<5){
                    heightballoon = 5;
                }

                clearLayout(m_pLayout);

                m_pLayout->addWidget(Label1, heightballoon, 0, Qt::AlignTop);
                m_pLayout->addWidget(Label, 0, 0, 300, 0, Qt::AlignTop);
                m_pLayout->update();

            }
        }
    }
}

//=============================================================================================================

void RealTimeNeurofeedbackWidget::initDisplayControllWidgets()
{
    qDebug()<<"RealTimeNeurofeedbackWidget initDisplayContorllWidgets";
    if(m_pFiffInfo) {
        //Create table view and set layout
        m_pChannelDataView = new RtFiffRawView(QString("MNESCAN/RTNW"),
                                               this);
        m_pChannelDataView->hide();

        QGridLayout *rtnLayout = new QGridLayout(this);
        rtnLayout->setContentsMargins(0,0,0,0);
        this->setLayout(rtnLayout);
        this->setMinimumSize(300,50);

        m_pLayout = new QGridLayout();

        QToolBar* pToolBar = new QToolBar;

        QAction* pActionSelectSensors = new QAction(QIcon(":/images/selectSensors.png"), tr("Show the channel selection view"),this);
        pActionSelectSensors->setToolTip(tr("Show the channel selection view"));
        connect(pActionSelectSensors, &QAction::triggered,
                this, &RealTimeNeurofeedbackWidget::showSensorSelectionWidget);
        pActionSelectSensors->setVisible(true);
        pToolBar->addAction(pActionSelectSensors);


        clearLayout(m_pLayout);
        rtnLayout->addWidget(pToolBar, 0, 0);
        rtnLayout->addWidget(m_pChannelDataView);
        rtnLayout->addLayout(m_pLayout, 1, 0, -1, -1, Qt::AlignCenter);
        rtnLayout->update();

        // Init channel view
        QSettings settings("MNECPP");
        QString sRTNWName = m_pRTNR->getName();

//        m_pChannelDataView->show();
        m_pChannelDataView->init(m_pFiffInfo);

        if(settings.value(QString("RTNW/showHideBad"), false).toBool()) {
            this->onHideBadChannels();
        }

        m_pChannelInfoModel = ChannelInfoModel::SPtr::create(m_pFiffInfo,
                                                             this);

        m_pChannelSelectionView = ChannelSelectionView::SPtr::create(QString("MNESCAN/RTNW"),
                                                                     this,
                                                                     m_pChannelInfoModel,
                                                                     Qt::Window);
        m_pChannelSelectionView->setWindowTitle(tr(QString("%1: Channel Selection Window").arg(sRTNWName).toUtf8()));

        connect(m_pChannelSelectionView.data(), &ChannelSelectionView::loadedLayoutMap,
                m_pChannelInfoModel.data(), &ChannelInfoModel::layoutChanged);


        m_pChannelInfoModel->layoutChanged(m_pChannelSelectionView->getLayoutMap());

        //Init control widgets
        QList<QWidget*> lControlWidgets;


        //Initialized
        m_bDisplayWidgetsInitialized = true;
    }
}

//=============================================================================================================

void RealTimeNeurofeedbackWidget::showSensorSelectionWidget()
{
    qDebug()<<"RealTimeNeurofeedbackWidget showSensorSelectionWidget";
    if(m_pChannelSelectionView->isActiveWindow()) {
        m_pChannelSelectionView->hide();
    } else {
        m_pChannelSelectionView->activateWindow();
        m_pChannelSelectionView->show();
    }
}

//=============================================================================================================

void RealTimeNeurofeedbackWidget::onMakeScreenshot(const QString& imageType)
{
    qDebug()<<"RealTimeNeurofeedbackWidget onMakeScreenshot";
    // Create file name
    QString sDate = QDate::currentDate().toString("yyyy_MM_dd");
    QString sTime = QTime::currentTime().toString("hh_mm_ss");

    if(!QDir("./Screenshots").exists()) {
        QDir().mkdir("./Screenshots");
    }

    QString fileName;

    if(imageType.contains("SVG")) {
        fileName = QString("./Screenshots/%1-%2-DataView.svg").arg(sDate).arg(sTime);
    } else if(imageType.contains("PNG")) {
        fileName = QString("./Screenshots/%1-%2-DataView.png").arg(sDate).arg(sTime);
    }

    m_pChannelDataView->takeScreenshot(fileName);
}

//=============================================================================================================

void RealTimeNeurofeedbackWidget::onHideBadChannels()
{
    qDebug()<<"RealTimeNeurofeedbackWidget onHIdeBadChannels";
    m_pChannelDataView->hideBadChannels();

    if(m_pActionHideBad->toolTip() == "Show all bad channels") {
        m_pActionHideBad->setIcon(QIcon(":/images/hideBad.png"));
        m_pActionHideBad->setToolTip("Hide all bad channels");
        m_pActionHideBad->setStatusTip(tr("Hide all bad channels"));
    } else {
        m_pActionHideBad->setIcon(QIcon(":/images/showBad.png"));
        m_pActionHideBad->setToolTip("Show all bad channels");
        m_pActionHideBad->setStatusTip(tr("Show all bad channels"));
    }
}

//=============================================================================================================

void RealTimeNeurofeedbackWidget::updateOpenGLViewport()
{
    qDebug()<<"RealTimeNeurofeedbackWidget updateOpbenGLViewport";
    if(m_pChannelDataView) {
        m_pChannelDataView->updateOpenGLViewport();
    }
}

//=============================================================================================================

void RealTimeNeurofeedbackWidget::updateLabel(QString text, QString color)
{
    qDebug()<<"RealTimeNeurofeedbackWidget updateLabel";


    QLabel* Label= new QLabel(this);
    Label->setText(text);
    Label->setAlignment(Qt::AlignCenter);
    QFont font;   
    font.setBold(true);
    font.setPointSize(20);
    Label->setFont(font);
    if(color == "red"){
        Label->setStyleSheet("QLabel {color: red;}");
    }
    else if (color == "green"){
        Label->setStyleSheet("QLabel {color: green;}");
    }
    else if(color =="black"){
        Label->setStyleSheet("QLabel {color: black;}");
    }
    clearLayout(m_pLayout);
    m_pLayout->addWidget(Label);
    m_pLayout->setContentsMargins(3,0,3,0);
    m_pLayout->update();
}

//=============================================================================================================

void RealTimeNeurofeedbackWidget::updateImage(QPixmap image)
{
    qDebug()<<"RealTimeNeurofeedbackWidget updateImage";

    QLabel* Label= new QLabel(this);
    Label->setPixmap(image.scaled(600, 300, Qt::KeepAspectRatio));
    Label->setAlignment(Qt::AlignCenter);
    clearLayout(m_pLayout);
    m_pLayout->addWidget(Label);
    m_pLayout->setContentsMargins(3,0,3,0);
    m_pLayout->update();
}

//=============================================================================================================

void RealTimeNeurofeedbackWidget::addSlider(int value, int output, int Max, int Min)
{
   int tickint;
   QVector<int> vec;
   vec.push_back(1);
   vec.push_back(2);
   vec.push_back(5);
   vec.push_back(10);
   int j = round((Max-Min)/10);

   auto const i = std::lower_bound(vec.begin(), vec.end(), j);
   if(i == vec.end()){
       tickint = 10;
   }
   else{
       tickint = *i;
   }

   QSlider* slider = new QSlider(Qt::Vertical);
   slider->setMaximum(Max);
   slider->setMinimum(Min);
   slider->setValue(output);
   slider->setTickPosition(QSlider::TicksBelow);
   slider->setTickInterval(tickint);
   slider->setMinimumHeight(300);

   QLabel* labelmin = new QLabel(this);
   QLabel* labelmax = new QLabel(this);
   labelmin->setText(QString::number(Min));
   labelmin->setAlignment(Qt::AlignBottom);
   labelmax->setText(QString::number(Max));
   labelmax->setAlignment(Qt::AlignTop);
   m_pLayout->addWidget(slider, 2, 2*value, 4, 1);
   m_pLayout->addWidget(labelmin, 5, 2*value+1, 1, 1);
   m_pLayout->addWidget(labelmax, 2, 2*value+1, 1, 1);
}

//=============================================================================================================

void RealTimeNeurofeedbackWidget::clearLayout(QLayout *layout){
    if(layout == NULL)
        return;
    QLayoutItem *item;

    while((item = layout->takeAt(0))){
        if(item->layout()){
            clearLayout(item->layout());
            delete item->layout();
        }
        if(item->widget()){
            delete item->widget();
        }
        delete item;
    }
}
