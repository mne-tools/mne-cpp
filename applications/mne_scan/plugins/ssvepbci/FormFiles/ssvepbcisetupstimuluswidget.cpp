//=============================================================================================================
/**
* @file     ssvepbcisetupstimuluswidget.cpp
* @author   Viktor Klüber <viktor.klueber@tu-ilmenauz.de>;
*           Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     April, 2016
*
* @section  LICENSE
*
* Copyright (C) 2016, Viktor Klüber Lorenz Esch and Matti Hamalainen. All rights reserved.
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
* @brief    Definition of the ssvepBCISetupStimulusWidget class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ssvepbcisetupstimuluswidget.h"
#include "ui_ssvepbcisetupstimuluswidget.h"
#include "../ssvepbci.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace SSVEPBCIPLUGIN;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

SsvepBciSetupStimulusWidget::SsvepBciSetupStimulusWidget(SsvepBci *pSsvepBci, QWidget *parent)
: QDialog(parent)
, ui(new Ui::SsvepBciSetupStimulusWidget)
, m_pSsvepBci(pSsvepBci)
, m_bIsRunning(false)
, m_bReadFreq(false)
{
    ui->setupUi(this);

    //setup the test screen and initialize screen for subject
    m_pSsvepBciScreen = QSharedPointer<SsvepBciScreen>(new SsvepBciScreen(m_pSsvepBci, this));
    m_pScreen  =  QSharedPointer<QScreen>(QGuiApplication::screens()[1]); // specify which screen to use
    m_pSsvepBciScreen->move(m_pScreen->geometry().x(), m_pScreen->geometry().y());
    m_pSsvepBciScreen->showFullScreen(); // showFullScreen();

    // connect signal for frequency change
    connect(this, &SsvepBciSetupStimulusWidget::frequencyChanged, m_pSsvepBci, &SsvepBci::setChangeSSVEPParameterFlag);

    // connect screen buttons
    connect(ui->m_pushButton_clearScreen, &QPushButton::clicked, this, &SsvepBciSetupStimulusWidget::clearItems);
    connect(ui->m_pushButton_closeScreen, &QPushButton::clicked, this, &SsvepBciSetupStimulusWidget::minimizeScreen);
    connect(ui->m_pushButton_showScreen, &QPushButton::clicked, this, &SsvepBciSetupStimulusWidget::showTestScreen);

    // connect stimulation buttons
    connect(ui->m_pushButton_test1, &QPushButton::clicked, this, &SsvepBciSetupStimulusWidget::test1);
    connect(ui->m_pushButton_test2, &QPushButton::clicked, this, &SsvepBciSetupStimulusWidget::test2);
    connect(ui->m_pushButton_test3, &QPushButton::clicked, this, &SsvepBciSetupStimulusWidget::test3);
    connect(ui->m_pushButton_screenKeyboard, &QPushButton::clicked, this, &SsvepBciSetupStimulusWidget::screenKeyboard);

    // connect panel buttons
    connect(ui->m_comboBox_panelSelection, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &SsvepBciSetupStimulusWidget::panelSelect);
    connect(ui->m_comboBox_frequencySelect, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &SsvepBciSetupStimulusWidget::frequencySelect);

    //Map for all frequencies according to their key
    m_idFreqMap.insert(15,  6   );
    m_idFreqMap.insert(14,  6.66);
    m_idFreqMap.insert(13,  7.05);
    m_idFreqMap.insert(12,  7.5 );
    m_idFreqMap.insert(11,  8   );
    m_idFreqMap.insert(10,  8.57);
    m_idFreqMap.insert( 9,  9.23);
    m_idFreqMap.insert( 8, 10   );
    m_idFreqMap.insert( 7, 10.91);
    m_idFreqMap.insert( 6, 12   );
    m_idFreqMap.insert( 5, 13.33);
    m_idFreqMap.insert( 4, 15   );
    m_idFreqMap.insert( 3, 17.14);
    m_idFreqMap.insert( 2, 20   );
    m_idFreqMap.insert( 1, 24   );
    m_idFreqMap.insert( 0, 30   );

    //initialize combobox for frequencies
    foreach(int i, m_idFreqMap.keys()){
        ui->m_comboBox_frequencySelect->addItem(QString().number(m_idFreqMap[i]));
    }

    //getting refreshrate of the subject's screen and add it to the setupWidget
    ui->label_6->setText(QString().number(m_pScreen->refreshRate()));
}


//*************************************************************************************************************

SsvepBciSetupStimulusWidget::~SsvepBciSetupStimulusWidget()
{
    delete ui;
}


//*************************************************************************************************************

void SsvepBciSetupStimulusWidget::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event)
    m_pSsvepBciScreen->close();
}


//*************************************************************************************************************

void SsvepBciSetupStimulusWidget::clear()
{
    m_bIsRunning = false;
    m_pSsvepBciScreen->m_Items.clear();
    m_pSsvepBciScreen->clearScreen();
}


//*************************************************************************************************************

void SsvepBciSetupStimulusWidget::changeComboBox()
{

    //clear ComboBox
    ui->m_comboBox_panelSelection->clear();

    //create new comobBox list
    for(int i = 1; i <= m_pSsvepBciScreen->m_Items.size(); i++ ){
        ui->m_comboBox_panelSelection->addItem(QString().number(i));
    }

    m_bIsRunning = true;
    panelSelect(0);
}


//*************************************************************************************************************

void SsvepBciSetupStimulusWidget::showTestScreen()
{
    m_pSsvepBciScreen->showFullScreen();
}


//*************************************************************************************************************

void SsvepBciSetupStimulusWidget::clearItems()
{
    clear();
}


//*************************************************************************************************************

void SsvepBciSetupStimulusWidget::minimizeScreen()
{
    m_pSsvepBciScreen->setWindowState(Qt::WindowMinimized);
}


//*************************************************************************************************************

void SsvepBciSetupStimulusWidget::test3()
{
    // TEST 3
    //clear  Items from screen
    clear();
    m_pSsvepBciScreen->useScreenKeyboard(false);

    SsvepBciFlickeringItem item1;
    SsvepBciFlickeringItem item2;
    SsvepBciFlickeringItem item3;
    SsvepBciFlickeringItem item4;

    //set Frequencies
    setFreq(item1,15);
    setFreq(item2,12);
    setFreq(item3,8);
    setFreq(item4,4);

    //set dimensions and positions
    item1.setDim(0.2,0.2);
    item2.setDim(0.2,0.2);
    item3.setDim(0.2,0.2);
    item4.setDim(0.2,0.2);
    item1.setPos(0.2,0.2);
    item2.setPos(1-0.4,0.2);
    item3.setPos(1-0.4,1-0.4);
    item4.setPos(0.2,1-0.4);

    m_pSsvepBciScreen->m_Items <<item1<<item2<<item3<<item4 ;
    changeComboBox();
}


//*************************************************************************************************************

void SsvepBciSetupStimulusWidget::test1()
{
    // TEST 1
    //clear  Items from screen
    clear();
    m_pSsvepBciScreen->useScreenKeyboard(false);

    SsvepBciFlickeringItem item1;

    //whole screen with 15 Hz
    setFreq(item1,11);
    item1.setDim(1,1);
    m_pSsvepBciScreen->m_Items <<item1 ;

    changeComboBox();
}


//*************************************************************************************************************

void SsvepBciSetupStimulusWidget::test2()
{
    // TEST 2

    //clear  Items from screen
    clear();
    m_pSsvepBciScreen->useScreenKeyboard(false);

    SsvepBciFlickeringItem item1;
    SsvepBciFlickeringItem item2;
    SsvepBciFlickeringItem item3;
    SsvepBciFlickeringItem item4;
    SsvepBciFlickeringItem item5;

    //set frequencies
    setFreq(item1,14);
    setFreq(item2,12);
    setFreq(item3,10);
    setFreq(item4, 8);
    setFreq(item5, 6);

    //set dimensions and positions
    item1.setDim(0.2,0.2);
    item2.setDim(0.2,0.2);
    item3.setDim(0.2,0.2);
    item4.setDim(0.2,0.2);
    item5.setDim(0.2,0.2);
    item1.setPos(0.5-0.1,0.1);
    item2.setPos(1-0.3,0.5-0.1);
    item3.setPos(0.5-0.1,1-0.3);
    item4.setPos(0.1,0.5-0.1);
    item5.setPos(0,0);
    //add items to List
    m_pSsvepBciScreen->m_Items <<item1<<item2<<item3<<item4<<item5 ;

    changeComboBox();
}


//*************************************************************************************************************

void SsvepBciSetupStimulusWidget::screenKeyboard()
{
    // Screen Keyboard
    //clear  Items from screen
    clear();


    SsvepBciFlickeringItem item1;
    SsvepBciFlickeringItem item2;
    SsvepBciFlickeringItem item3;
    SsvepBciFlickeringItem item4;
    SsvepBciFlickeringItem item5;

    //set frequencies
    setFreq(item1,14);
    setFreq(item2,12);
    setFreq(item3,10);
    setFreq(item4, 8);
    setFreq(item5, 6);
    //set dimensions and positions
    item1.setDim(0.2,0.2);
    item2.setDim(0.2,0.2);
    item3.setDim(0.2,0.2);
    item4.setDim(0.2,0.2);
    item5.setDim(0.2,0.2);
    item1.setPos(0.5-0.1,0);
    item2.setPos(1-0.2,0.5-0.1);
    item3.setPos(0.5-0.1,1-0.2);
    item4.setPos(0,0.5-0.1);
    item5.setPos(0,0);

    item1.addSign("↑");
    item2.addSign("→");
    item3.addSign("↓");
    item4.addSign("←");
    item5.addSign("Select");

    //add items to List
    m_pSsvepBciScreen->m_Items <<item1<<item2<<item3<<item4<<item5;

    changeComboBox();

    m_pSsvepBciScreen->useScreenKeyboard(true);
}


//*************************************************************************************************************

void SsvepBciSetupStimulusWidget::panelSelect(int index)
{
    if(m_bIsRunning){
        m_bReadFreq = true;
        ui->m_comboBox_frequencySelect->setCurrentIndex(m_pSsvepBciScreen->m_Items.at(index).m_iFreqKey);//get key of frequency
    }
}


//*************************************************************************************************************

void SsvepBciSetupStimulusWidget::frequencySelect(int index)
{
    if(m_bIsRunning){
        if(m_bReadFreq){
            m_bReadFreq=false;
        }
        else{
            //get selected Item from comboBox
            int ItemSelect = ui->m_comboBox_panelSelection->currentIndex();
            //adjust the rendering order of the selected Plugin
            setFreq(m_pSsvepBciScreen->m_Items[ItemSelect],index);
        }
    }
}


//*************************************************************************************************************

void SsvepBciSetupStimulusWidget::setFreq(SsvepBciFlickeringItem &item, int freqKey)
{
    QList<bool> renderOrder;

    //choose the rendereing orders according to evoked flickerfrequency (being valid for a 60 Hz monitor)
    switch(freqKey){
    case 0:
        renderOrder<< 0 << 1;
        break;  // 30 Hz
    case 1:
        renderOrder<< 0 << 1 << 0 << 1 << 1;
        break;  // 24 Hz
    case 2:
        renderOrder<< 0 << 1 << 1;
        break;  // 20 Hz
    case 3:
        renderOrder<< 0 << 0 << 1 << 1 << 0 << 1 << 1;
        break;  // 17.14 Hz
    case 4:
        renderOrder<< 0 << 0 << 1 << 1;
        break;  // 15 Hz
    case 5:
        renderOrder<< 0 << 0 << 1 << 1 << 0 << 0 << 1 << 1 << 1;
        break;  // 13.33 Hz
    case 6:
        renderOrder<< 0 << 0 << 1 << 1 << 1;
        break;  // 12 Hz
    case 7:
        renderOrder<< 0 << 0 << 1 << 1 << 1 << 0 << 0 << 0 << 1 << 1 << 1;
        break;  // 10.91 Hz
    case 8:
        renderOrder<< 0 << 0 << 0 << 1 << 1 << 1;
        break;  // 10 Hz
    case 9:
        renderOrder<< 0 << 0 << 0 << 1 << 1 << 1 << 0 << 0 << 0 << 1 << 1 << 1 << 1;
        break;  // 9.23 Hz
    case 10:
        renderOrder<< 0 << 0 << 0 << 1 << 1 << 1 << 1;
        break;  // 8.57 Hz
    case 11:
        renderOrder<< 0 << 0 << 0 << 1 << 1 << 1 << 1 << 0 << 0 << 0 << 0 << 1 << 1 << 1 << 1;
        break;  // 8 Hz
    case 12:
        renderOrder<< 0 << 0 << 0 << 0 << 1 << 1 << 1 << 1;
        break;  // 7.5 Hz
    case 13:
        renderOrder<< 0 << 0 << 0 << 0 << 1 << 1 << 1 << 1 << 0 << 0 << 0 << 0 << 1 << 1 << 1 << 1 << 1;
        break;  // 7.05 Hz
    case 14:
        renderOrder<< 0 << 0 << 0 << 0 << 1 << 1 << 1 << 1 << 1;
        break;  // 6.66 Hz
    case 15:
        renderOrder<< 0 << 0 << 0 << 0 << 0 << 1 << 1 << 1 << 1 << 1;
        break;  // 6 Hz
    default:
        renderOrder<< 0 << 1 ;
        qDebug()<< "SSVEPBCI-SETUP: Could not set up rendering order. 30 Hz have been chosen instead for flicker frequency!";
        break;
    }
    item.setRenderOrder(renderOrder, freqKey);

    // signal for changing frequency list of ssvepBCI class
    emit frequencyChanged();
}


//*************************************************************************************************************

QList<double> SsvepBciSetupStimulusWidget::getFrequencies()
{
    // get list of frequencies from the item-list beholding the ssvepBCIScreen class
    QList<double> freqList;
    foreach(SsvepBciFlickeringItem item, m_pSsvepBciScreen->m_Items){
        freqList << m_idFreqMap.value(item.getFreqKey());
    }
    return freqList;
}


//*************************************************************************************************************

void SsvepBciSetupStimulusWidget::on_m_lineEdit_BCISpeller_textChanged(const QString &arg1)
{
    emit settledPhrase(arg1);
}


//*************************************************************************************************************

QSharedPointer<ScreenKeyboard> SsvepBciSetupStimulusWidget::getScreenKeyboardSPtr()
{
    return m_pSsvepBciScreen->m_pScreenKeyboard;
}
