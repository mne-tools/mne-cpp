//=============================================================================================================
/**
* @file     frequencyspectrumwidget.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Limin Sun <liminsun@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     July, 2014
*
* @section  LICENSE
*
* Copyright (C) 2014, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Definition of the FrequencySpectrumWidget Class.
*
*/

//ToDo Paint to render area

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "frequencyspectrumwidget.h"
//#include "annotationwindow.h"

#include <scMeas/frequencyspectrum.h>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <math.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QVBoxLayout>
#include <QLabel>
#include <QHeaderView>
#include <QSettings>
#include <QTableView>

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace SCDISPLIB;
using namespace SCMEASLIB;


//=============================================================================================================
/**
* Tool enumeration.
*/
enum Tool
{
    Freeze     = 0,     /**< Freezing tool. */
    Annotation = 1      /**< Annotation tool. */
};


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FrequencySpectrumWidget::FrequencySpectrumWidget(QSharedPointer<FrequencySpectrum> pFS, QSharedPointer<QTime> &pTime, QWidget* parent)
: MeasurementWidget(parent)
, m_pFSModel(Q_NULLPTR)
, m_pFSDelegate(Q_NULLPTR)
, m_pTableView(Q_NULLPTR)
, m_pFS(pFS)
, m_fLowerFrqBound(0)
, m_fUpperFrqBound(300)
, m_bInitialized(false)
{
    Q_UNUSED(pTime)


    m_pActionFrequencySettings = new QAction(QIcon(":/images/frqResolution.png"), tr("Shows the frequency spectrum settings widget (F12)"),this);
    m_pActionFrequencySettings->setShortcut(tr("F12"));
    m_pActionFrequencySettings->setStatusTip(tr("Shows the frequency spectrum settings widget (F12)"));
    connect(m_pActionFrequencySettings, &QAction::triggered, this, &FrequencySpectrumWidget::showFrequencySpectrumSettingsWidget);
    addDisplayAction(m_pActionFrequencySettings);

    m_pActionFrequencySettings->setVisible(false);

    if(m_pTableView)
        delete m_pTableView;
    m_pTableView = new QTableView;

    m_pTableView->setSelectionMode( QAbstractItemView::ExtendedSelection );
    m_pTableView->setMouseTracking( true );
    m_pTableView->viewport()->installEventFilter( this );

    //set vertical layout
    QVBoxLayout *neLayout = new QVBoxLayout(this);

//    QLabel *t_pLabelFrequencySpectrum = new QLabel;
//    t_pLabelFrequencySpectrum->setText("Noise Estimation Widget");
//    neLayout->addWidget(t_pLabelFrequencySpectrum);

    neLayout->addWidget(m_pTableView);

    //set layouts
    this->setLayout(neLayout);

    getData();
}


//*************************************************************************************************************

FrequencySpectrumWidget::~FrequencySpectrumWidget()
{
    //
    // Store Settings
    //
    if(!m_pFS->getName().isEmpty())
    {
        QString t_sFSName = m_pFS->getName();

        QSettings settings;

        settings.setValue(QString("FSW/%1/lowerFrqBound").arg(t_sFSName), m_fLowerFrqBound);
        settings.setValue(QString("FSW/%1/upperFrqBound").arg(t_sFSName), m_fUpperFrqBound);
    }
}


//*************************************************************************************************************

void FrequencySpectrumWidget::update(SCMEASLIB::NewMeasurement::SPtr)
{
    getData();
}


//*************************************************************************************************************

void FrequencySpectrumWidget::getData()
{
//    qDebug() << "get Data" << m_pNE->getValue()(0,1) << "Cols" << m_pNE->getValue().cols();

    if(!m_bInitialized)
    {
        if(m_pFS->isInit())
        {
            init();

            m_pFSModel->addData(m_pFS->getValue());

            initSettingsWidget();
        }
    }
    else
        m_pFSModel->addData(m_pFS->getValue());
}


//*************************************************************************************************************

void FrequencySpectrumWidget::init()
{
    if(m_pFS->getFiffInfo())
    {
        QSettings settings;
        if(!m_pFS->getName().isEmpty())
        {
            QString t_sFSName = m_pFS->getName();
            m_fLowerFrqBound = settings.value(QString("FSW/%1/lowerFrqBound").arg(t_sFSName), 0).toFloat();
            m_fUpperFrqBound = settings.value(QString("FSW/%1/upperFrqBound").arg(t_sFSName), 300).toFloat();
        }

        m_pActionFrequencySettings->setVisible(true);

        if(m_pFSModel)
            delete m_pFSModel;
        m_pFSModel = new FrequencySpectrumModel(this);

        m_pFSModel->setInfo(m_pFS->getFiffInfo());
        m_pFSModel->setScaleType(m_pFS->getScaleType()); /*Added by Limin; 10/19/2014 for passing the scale type to the model*/

        if(m_pFSDelegate)
            delete m_pFSDelegate;
//        m_pFSDelegate = new FrequencySpectrumDelegate(this);
        m_pFSDelegate = new FrequencySpectrumDelegate(m_pTableView,this);
        m_pFSDelegate->setScaleType(m_pFS->getScaleType()); /*Added by Limin; 10/19/2014 for passing the scale type to the delegate*/


        connect(m_pTableView, &QTableView::doubleClicked, m_pFSModel, &FrequencySpectrumModel::toggleFreeze);

        // add a connection for sending mouse location to the delegate; Dr. -Ing. Limin Sun 8/21/14
        connect(this,&FrequencySpectrumWidget::sendMouseLoc,
                m_pFSDelegate, &FrequencySpectrumDelegate::rcvMouseLoc);


        m_pTableView->setModel(m_pFSModel);
        m_pTableView->setItemDelegate(m_pFSDelegate);

        //set some size settings for m_pTableView
        m_pTableView->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);

        m_pTableView->setShowGrid(false);

        m_pTableView->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch); //Stretch 2 column to maximal width
        m_pTableView->horizontalHeader()->hide();
        m_pTableView->verticalHeader()->setDefaultSectionSize(140);//m_fZoomFactor*m_fDefaultSectionSize);//Row Height

        m_pTableView->setAutoScroll(false);
        m_pTableView->setColumnHidden(0,true); //because content is plotted jointly with column=1

        m_pTableView->resizeColumnsToContents();

        m_pTableView->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);

        //set context menu
        m_pTableView->setContextMenuPolicy(Qt::CustomContextMenu);
        //connect(m_pTableView,SIGNAL(customContextMenuRequested(QPoint)),this,SLOT(channelContextMenu(QPoint)));

        m_bInitialized = true;



    }
}


//*************************************************************************************************************

void FrequencySpectrumWidget::initSettingsWidget()
{
    if(!m_pFrequencySpectrumSettingsWidget)
    {
        m_pFrequencySpectrumSettingsWidget = QSharedPointer<FrequencySpectrumSettingsWidget>(new FrequencySpectrumSettingsWidget(this));

        m_pFrequencySpectrumSettingsWidget->setWindowTitle("Frequency Spectrum Settings");

        connect(m_pFrequencySpectrumSettingsWidget.data(), &FrequencySpectrumSettingsWidget::settingsChanged, this, &FrequencySpectrumWidget::broadcastSettings);
    }

    if(m_pFS->isInit() && m_pFS->getFiffInfo())
    {
        m_fUpperFrqBound = m_fLowerFrqBound < m_fUpperFrqBound ? m_fUpperFrqBound : m_fLowerFrqBound;
        m_pFrequencySpectrumSettingsWidget->m_pSliderLowerBound->setMinimum(0);
        m_pFrequencySpectrumSettingsWidget->m_pSliderLowerBound->setMaximum((qint32)(m_pFS->getFiffInfo()->sfreq/2)*1000);
        m_pFrequencySpectrumSettingsWidget->m_pSliderLowerBound->setValue((qint32)(m_fLowerFrqBound*1000));

        m_pFrequencySpectrumSettingsWidget->m_pSliderUpperBound->setMinimum(0);
        m_pFrequencySpectrumSettingsWidget->m_pSliderUpperBound->setMaximum((qint32)(m_pFS->getFiffInfo()->sfreq/2)*1000);
        m_pFrequencySpectrumSettingsWidget->m_pSliderUpperBound->setValue((qint32)(m_fUpperFrqBound*1000));
    }

}


//*************************************************************************************************************

void FrequencySpectrumWidget::broadcastSettings()
{
    if(m_pFrequencySpectrumSettingsWidget)
    {
        m_fLowerFrqBound = m_pFrequencySpectrumSettingsWidget->m_pSliderLowerBound->value()/1000.0f;
        m_fUpperFrqBound = m_pFrequencySpectrumSettingsWidget->m_pSliderUpperBound->value()/1000.0f;
        m_pFSModel->setBoundaries(m_fLowerFrqBound,m_fUpperFrqBound);
    }
}


//*************************************************************************************************************

void FrequencySpectrumWidget::showFrequencySpectrumSettingsWidget()
{
    initSettingsWidget();
    m_pFrequencySpectrumSettingsWidget->show();
}


//*************************************************************************************************************

bool FrequencySpectrumWidget::eventFilter(QObject * watched, QEvent * event)
{
  if(event->type() == QEvent::MouseMove){
      QMouseEvent *mouseEvent = static_cast <QMouseEvent*>( event );
      //qDebug()<<"MouseMove event!@"<<mouseEvent->x()<<":"<<mouseEvent->y();

      int currentRow = m_pTableView->rowAt(mouseEvent->y());
      m_pTableView->selectRow(currentRow);

      QModelIndex item = m_pTableView->currentIndex();

      emit sendMouseLoc(item.row(), mouseEvent->x(), mouseEvent->y(),m_pTableView->visualRect(item) );

      return true;
  }
  else
  {
      return QWidget::eventFilter(watched, event);
  }

}
