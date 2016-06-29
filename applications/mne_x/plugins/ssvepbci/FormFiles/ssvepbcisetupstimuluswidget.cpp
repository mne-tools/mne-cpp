//=============================================================================================================
/**
* @file     ssvepBCISetupStimulusWidget.cpp
* @author   Viktor Klüber <viktor.klueber@tu-ilmenauz.de>;
*           Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     April 2016
*
* @section  LICENSE
*
* Copyright (C) 2014, Lorenz Esch, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Contains the implementation of the ssvepBCISetupStimulusWidget class.
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

using namespace ssvepBCIPlugin;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ssvepBCISetupStimulusWidget::ssvepBCISetupStimulusWidget(ssvepBCI* pssvepBCI, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ssvepBCISetupStimulusWidget),
     m_pssvepBCI(pssvepBCI),
     m_bIsRunning(false),
     m_bReadFreq(false)
{
    ui->setupUi(this);

    //setup the test screen and initialize screen for subject
    m_pssvepBCIScreen = new ssvepBCIScreen;
    QScreen *screen  = QGuiApplication::screens()[1]; // specify which screen to use
    m_pssvepBCIScreen->move(screen->geometry().x(), screen->geometry().y());
    m_pssvepBCIScreen->showFullScreen();

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
    foreach(int i, m_idFreqMap.keys())
        ui->comboBox_2->addItem(QString().number(m_idFreqMap[i]));

    //getting refreshrate of the subject's screen and add it to the setupWidget
    ui->label_6->setText(QString().number(screen->refreshRate()));
    delete screen;
}

//*************************************************************************************************************

ssvepBCISetupStimulusWidget::~ssvepBCISetupStimulusWidget()
{
    delete ui;
    delete m_pssvepBCIScreen;
}


//*************************************************************************************************************

void ssvepBCISetupStimulusWidget::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event)
    m_pssvepBCIScreen->close();
}

//*************************************************************************************************************

void ssvepBCISetupStimulusWidget::clear()
{
    m_bIsRunning = false;
    m_pssvepBCIScreen->m_Items.clear();
}


//*************************************************************************************************************

void ssvepBCISetupStimulusWidget::changeComboBox()
{

    //clear ComboBox
    ui->comboBox->clear();

    //create new comobBox list
    for(int i = 1; i <= m_pssvepBCIScreen->m_Items.size(); i++ )
        ui->comboBox->addItem(QString().number(i));

    m_bIsRunning = true;
    on_comboBox_currentIndexChanged(0);
}

//*************************************************************************************************************

void ssvepBCISetupStimulusWidget::on_pushButton_clicked()
{
    m_pssvepBCIScreen->showFullScreen();
}

//*************************************************************************************************************

void ssvepBCISetupStimulusWidget::on_pushButton_2_clicked()
{
    clear();
}

//*************************************************************************************************************

void ssvepBCISetupStimulusWidget::on_pushButton_3_clicked()
{
    m_pssvepBCIScreen->close();
}

//*************************************************************************************************************

void ssvepBCISetupStimulusWidget::on_pushButton_4_clicked()
{
    // TEST 3
    //clear  Items from screen
    clear();

    ssvepBCIFlickeringItem item1;
    ssvepBCIFlickeringItem item2;
    ssvepBCIFlickeringItem item3;
    ssvepBCIFlickeringItem item4;

    //set Frequencies
    setFreq(item1,1);
    setFreq(item2,5);
    setFreq(item3,9);
    setFreq(item4,13);
    //set dimensions and positions
    item1.setDim(0.2,0.2);
    item2.setDim(0.2,0.2);
    item3.setDim(0.2,0.2);
    item4.setDim(0.2,0.2);
    item1.setPos(0.2,0.2);
    item2.setPos(1-0.4,0.2);
    item3.setPos(1-0.4,1-0.4);
    item4.setPos(0.2,1-0.4);

    m_pssvepBCIScreen->m_Items <<item1<<item2<<item3<<item4 ;
    changeComboBox();
}

//*************************************************************************************************************

void ssvepBCISetupStimulusWidget::on_pushButton_5_clicked()
{
    // TEST 1
    //clear  Items from screen
    clear();

    ssvepBCIFlickeringItem item1;
    //whole screen with 15 Hz
    setFreq(item1,11);
    item1.setDim(1,1);
    m_pssvepBCIScreen->m_Items <<item1 ;

    changeComboBox();
}

//*************************************************************************************************************

void ssvepBCISetupStimulusWidget::on_pushButton_6_clicked()
{
    // TEST 2

    //clear  Items from screen
    clear();

    ssvepBCIFlickeringItem item1;
    ssvepBCIFlickeringItem item2;
    ssvepBCIFlickeringItem item3;
    ssvepBCIFlickeringItem item4;

    //set frequencies
    setFreq(item1,0);
    setFreq(item2,3);
    setFreq(item3,7);
    setFreq(item4,11);
    //set dimensions and positions
    item1.setDim(0.2,0.2);
    item2.setDim(0.2,0.2);
    item3.setDim(0.2,0.2);
    item4.setDim(0.2,0.2);
    item1.setPos(0.2,0.2);
    item2.setPos(1-0.4,0.2);
    item3.setPos(1-0.4,1-0.4);
    item4.setPos(0.2,1-0.4);
    //add items to List
    m_pssvepBCIScreen->m_Items <<item1<<item2<<item3<<item4 ;

    changeComboBox();

}

//*************************************************************************************************************

void ssvepBCIPlugin::ssvepBCISetupStimulusWidget::on_pushButton_7_clicked()
{
    // TEST 4
    //clear  Items from screen
    clear();

    ssvepBCIFlickeringItem item1;
    //whole screen with 15 Hz
    setFreq(item1,3);
    item1.setDim(1,1);
    m_pssvepBCIScreen->m_Items <<item1 ;

    changeComboBox();

}


//*************************************************************************************************************

void ssvepBCISetupStimulusWidget::on_comboBox_currentIndexChanged(int index)
{

    if(m_bIsRunning){
        m_bReadFreq = true;
        ui->comboBox_2->setCurrentIndex(m_pssvepBCIScreen->m_Items.at(index).m_iFreqKey);//get key of frequency
    }
}

//*************************************************************************************************************

void ssvepBCIPlugin::ssvepBCISetupStimulusWidget::on_comboBox_2_currentIndexChanged(int index)
{
    if(m_bIsRunning){
        if(m_bReadFreq)
            m_bReadFreq=false;
        else{

            //get selected Item from comboBox
            int ItemSelect = ui->comboBox->currentIndex();
            //adjust the rendering order of the selected Plugin
            setFreq(m_pssvepBCIScreen->m_Items[ItemSelect],index);
        }
    }

}

//*************************************************************************************************************

void ssvepBCISetupStimulusWidget::setFreq(ssvepBCIFlickeringItem &item, int freqKey)
{

    QList<bool> renderOrder;

    //choose the rendereing orders according to evoked flickerfrequency (being valid for a 60 Hz monitor)
    switch(freqKey){
    case 0:
        renderOrder<< 0 << 1;                                                                                break;  // 30 Hz
    case 1:
        renderOrder<< 0 << 1 << 0 << 1 << 1;                                                                 break;  // 24 Hz
    case 2:
        renderOrder<< 0 << 1 << 1;                                                                           break;  // 20 Hz
    case 3:
        renderOrder<< 0 << 0 << 1 << 1 << 0 << 1 << 1;                                                       break;  // 17.14 Hz
    case 4:
        renderOrder<< 0 << 0 << 1 << 1;                                                                      break;  // 15 Hz
    case 5:
        renderOrder<< 0 << 0 << 1 << 1 << 0 << 0 << 1 << 1 << 1;                                             break;  // 13.33 Hz
    case 6:
        renderOrder<< 0 << 0 << 1 << 1 << 1;                                                                 break;  // 12 Hz
    case 7:
        renderOrder<< 0 << 0 << 1 << 1 << 1 << 0 << 0 << 0 << 1 << 1 << 1;                                   break;  // 10.91 Hz
    case 8:
        renderOrder<< 0 << 0 << 0 << 1 << 1 << 1;                                                            break;  // 10 Hz
    case 9:
        renderOrder<< 0 << 0 << 0 << 1 << 1 << 1 << 0 << 0 << 0 << 1 << 1 << 1 << 1;                         break;  // 9.23 Hz
    case 10:
        renderOrder<< 0 << 0 << 0 << 1 << 1 << 1 << 1;                                                       break;  // 8.57 Hz
    case 11:
        renderOrder<< 0 << 0 << 0 << 1 << 1 << 1 << 1 << 0 << 0 << 0 << 0 << 1 << 1 << 1 << 1;               break;  // 8 Hz
    case 12:
        renderOrder<< 0 << 0 << 0 << 0 << 1 << 1 << 1 << 1;                                                  break;  // 7.5 Hz
    case 13:
        renderOrder<< 0 << 0 << 0 << 0 << 1 << 1 << 1 << 1 << 0 << 0 << 0 << 0 << 1 << 1 << 1 << 1 << 1;     break;  // 7.05 Hz
    case 14:
        renderOrder<< 0 << 0 << 0 << 0 << 1 << 1 << 1 << 1 << 1;                                             break;  // 6.66 Hz
    case 15:
        renderOrder<< 0 << 0 << 0 << 0 << 0 << 1 << 1 << 1 << 1 << 1;                                        break;  // 6 Hz
    default:{
        renderOrder<< 0 << 1 ;
        qDebug()<< "SSVEPBCI-SETUP: Could not set up rendering order. 30 Hz have been chosen instead for flicker frequency!";break;
    }
    }
    item.setRenderOrder(renderOrder, freqKey);
}

