//=============================================================================================================
/**
* @file     eegosportssetupstimuluswidget.cpp
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
* @brief    Contains the implementation of the EEGoSportsSetupStimulusWidget class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "eegosportssetupstimuluswidget.h"
#include "ui_eegosportssetupstimuluswidget.h"
#include "../flashobject.h"
#include "../eegosports.h"

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace EEGoSportsPlugin;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

EEGoSportsSetupStimulusWidget::EEGoSportsSetupStimulusWidget(EEGoSports* pEEGoSports, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::EEGoSportsSetupStimulusWidget),
     m_pEEGoSports(pEEGoSports),
     m_bIsRunning(false),
     m_bReadFreq(false)
{
    ui->setupUi(this);

    //create QGraphicsView with QGraphcisScene in a new Widget for the Test Screen
    m_pTestScreen   = new QWidget;
    m_pLayout       = new QVBoxLayout();
    m_pView         = new QGraphicsView();
    m_pScene        = new QGraphicsScene(m_pView);

    //setup the Widget for the test-screen
    m_pLayout->setSpacing(0);
    m_pLayout->setMargin(0);
    m_pLayout->setContentsMargins(0,0,0,0);
    m_pLayout->addWidget(m_pView);
    m_pTestScreen->setLayout(m_pLayout);
    m_pTestScreen->setStyleSheet("QWidget { border-style: none; }");

    //setting full-screen on second monitor
    QScreen *screen = QGuiApplication::screens()[1]; // specify which screen to use
    m_pTestScreen->move(screen->geometry().x(), screen->geometry().y());

    //connecting View with Scene
    m_pView->setScene(m_pScene);
    m_pView->setStyleSheet("QGraphicsView { border-style: none; }");

    //Setup a Timer for a continous refresh of the Test Screen (60 Hz)
    m_pTimer = new QTimer();
    connect(m_pTimer,SIGNAL(timeout()),this,SLOT(ScreenTrigger()));
    m_pTimer->setTimerType(Qt::PreciseTimer);
    m_pTimer->start(16.67);

    //connect the changedView slot to the Scene for refreshing function of the test screen view
    connect(m_pScene,SIGNAL(changed(QList<QRectF>)),this,SLOT(changeView()));
    m_pView->installEventFilter(this);

    //set Black as scene background color and show Test Screen
    m_pView->setBackgroundBrush(QBrush(Qt::black, Qt::SolidPattern));
    m_pTestScreen->showFullScreen();
}

//*************************************************************************************************************

EEGoSportsSetupStimulusWidget::~EEGoSportsSetupStimulusWidget()
{
    delete ui;

    delete m_pTestScreen;
    delete m_pLayout;
    delete m_pView;
    delete m_pScene;
}

//*************************************************************************************************************

void EEGoSportsSetupStimulusWidget::ScreenTrigger(){
    //m_pView->viewport()->repaint();
    m_pView->viewport()->update();
}

//*************************************************************************************************************

void EEGoSportsSetupStimulusWidget::changeView(){
    //Calculates and returns the bounding rect of all items on the scene.
    //This function works by iterating over all items, and because if this, it can be slow for large scenes.
    QRectF rect = m_pScene->itemsBoundingRect();
    m_pScene->setSceneRect(rect);
}

//*************************************************************************************************************

void EEGoSportsSetupStimulusWidget::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event)
    m_pTestScreen->close();
}

//*************************************************************************************************************

void EEGoSportsSetupStimulusWidget::clear()
{
    m_bIsRunning = false;

    QList <QGraphicsItem*> itemList = getTopLevelItems();
    while(!itemList.isEmpty()){
        delete itemList.first();
        itemList = m_pScene->items();
    }

}

//*************************************************************************************************************

QList<QGraphicsItem*> EEGoSportsSetupStimulusWidget::getTopLevelItems()
{
    int numItems, iItem;
    QList<QGraphicsItem*> topLevel;
    QList<QGraphicsItem*> itemList = m_pScene->items();

    numItems = itemList.size();
    for (iItem = 0; iItem < numItems; iItem++) {
        QGraphicsItem *item = itemList.at(iItem);
        if (item->parentItem() == NULL)
            topLevel.append(item);
    }
    return topLevel;
}

//*************************************************************************************************************

void EEGoSportsSetupStimulusWidget::changeComboBox(QList<QGraphicsItem*> &List)
{

    //clear ComboBox
    ui->comboBox->clear();

    //refresh Items on the QList
    m_pItems.clear();
    m_pItems = getTopLevelItems();

    //create new comobBox list
    for(int i = 1; i <= List.size(); i++ )
        ui->comboBox->addItem(QString().number(i));

    m_bIsRunning = true;
    on_comboBox_currentIndexChanged(0);
}

//*************************************************************************************************************

void EEGoSportsPlugin::EEGoSportsSetupStimulusWidget::on_pushButton_clicked()
{
    m_pTestScreen->showFullScreen();
}

//*************************************************************************************************************

void EEGoSportsPlugin::EEGoSportsSetupStimulusWidget::on_pushButton_2_clicked()
{
    clear();
}

//*************************************************************************************************************

void EEGoSportsPlugin::EEGoSportsSetupStimulusWidget::on_pushButton_3_clicked()
{
    m_pTestScreen->close();
}

//*************************************************************************************************************

void EEGoSportsPlugin::EEGoSportsSetupStimulusWidget::on_pushButton_4_clicked()
{
    //clear  Items from screen
    clear();
    m_pFlashList.clear();

    //define dimensions of items
    int dimx_x = (0.33*double(m_pView->width()));
    int dimx_y = int(0.2*double(m_pView->height()));
    int dimy_x = int(0.125*double(m_pView->width()));
    int dimy_y = int(0.4*double(m_pView->height()));

    //create new flashing objects
    QPointer<FlashObject> obj_1 = new FlashObject();
    QPointer<FlashObject> obj_2 = new FlashObject();
    QPointer<FlashObject> obj_3 = new FlashObject();
    QPointer<FlashObject> obj_4 = new FlashObject();
    QPointer<FlashObject> obj_5 = new FlashObject();
    //set starting-flashing frequencies
    obj_1->setFreq(6);
    obj_2->setFreq(7.5);
    obj_3->setFreq(10);
    obj_4->setFreq(15);
    obj_5->setFreq(30);
    //set item dimesnions
    obj_1->setDim(dimx_x,dimx_y);
    obj_2->setDim(dimy_x,dimy_y);
    obj_3->setDim(dimx_x,dimx_y);
    obj_4->setDim(dimy_x,dimy_y);
    obj_5->setDim(dimx_x,dimy_y);
    //set postitions of flashing object
    obj_1->setPos(0.5*m_pView->width() - 0.5*dimx_x , 0);
    obj_2->setPos(m_pView->width() - dimy_x         , +0.5*m_pView->height() - 0.5*dimy_y );
    obj_3->setPos(0.5*m_pView->width() - 0.5*dimx_x , +m_pView->height() - dimx_y);
    obj_4->setPos(0                                 , +0.5*m_pView->height() - 0.5*dimy_y);
    obj_5->setPos(0.5*m_pView->width() - 0.5*dimx_x , +0.5*m_pView->height() - 0.5*dimy_y);
    //add items to scene and to FlashList
    m_pScene->addItem(obj_1);m_pFlashList.append(obj_1);
    m_pScene->addItem(obj_2);m_pFlashList.append(obj_2);
    m_pScene->addItem(obj_3);m_pFlashList.append(obj_3);
    m_pScene->addItem(obj_4);m_pFlashList.append(obj_4);
    m_pScene->addItem(obj_5);m_pFlashList.append(obj_5);

    //add Items to ComboBox
    changeComboBox(m_pItems);
}

//*************************************************************************************************************

void EEGoSportsPlugin::EEGoSportsSetupStimulusWidget::on_pushButton_5_clicked()
{
    //clear  Items from screen
    clear();
    m_pFlashList.clear();


    QPointer<FlashObject> obj_1 = new FlashObject();
    obj_1->setFreq(1);
    obj_1->setDim(m_pView->width(),m_pView->height());
    m_pScene->addItem(obj_1);m_pFlashList.append(obj_1);

    obj_1->setFreq(15);

    //add Items to ComboBox
    changeComboBox(m_pItems);
}

//*************************************************************************************************************

void EEGoSportsPlugin::EEGoSportsSetupStimulusWidget::on_pushButton_6_clicked()
{
    //clear  Items from screen
    clear();
    m_pFlashList.clear();

    QPointer<FlashObject> obj_1 = new FlashObject;
    QPointer<FlashObject> obj_2 = new FlashObject;
    //set Frequencies
    obj_1->setFreq(3);
    obj_2->setFreq(3.75);
    //set dimensions
    obj_1->setDim(0.2*m_pView->width(),0.2*m_pView->height());
    obj_2->setDim(0.2*m_pView->width(),0.2*m_pView->height());
    //set positions
    obj_1->setPos(0.2*m_pView->width(),(0.5-0.2/2)*m_pView->height());
    obj_2->setPos((1-0.5)*m_pView->width(), (0.5-0.2/2)*m_pView->height());
    //ad Items to scene
    m_pScene->addItem(obj_1);m_pFlashList.append(obj_1);
    m_pScene->addItem(obj_2);m_pFlashList.append(obj_2);

    //add Items to ComboBox
    changeComboBox(m_pItems);
}

//*************************************************************************************************************

void EEGoSportsPlugin::EEGoSportsSetupStimulusWidget::on_doubleSpinBox_valueChanged(double arg)
{
    if(m_bReadFreq)
        m_bReadFreq=false;
    else{
        //get selected Item from comboBox
        int ItemSelect = ui->comboBox->currentIndex();
        //adjust the Frequency of the selected Plugin
        m_pFlashList.at(ItemSelect)->setFreq(arg);
    }
}

//*************************************************************************************************************

void EEGoSportsPlugin::EEGoSportsSetupStimulusWidget::on_comboBox_currentIndexChanged(int index)
{
    if(m_bIsRunning){
        m_bReadFreq = true;
        ui->doubleSpinBox->setValue(m_pFlashList.at(index)->getFreq());
    }
}
