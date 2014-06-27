//=============================================================================================================
/**
* @file     tmsiimpedancewidget.cpp
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     June, 2014
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
* @brief    Contains the declaration of the TmsiImpedanceWidget class.
*
*/

#include "tmsiimpedancewidget.h"
#include "ui_tmsiimpedancewidget.h"

#include "../tmsi.h"

#include <QFileDialog>
#include <QString>

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace TMSIPlugin;

//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

TmsiImpedanceWidget::TmsiImpedanceWidget(TMSI* p_pTMSI, QWidget *parent)
: m_pTMSI(p_pTMSI)
, QWidget(parent)
, ui(new Ui::TmsiImpedanceWidget)
{
    ui->setupUi(this);

    ui->m_graphicsView_impedanceView->setScene(&m_scene);

    ui->m_graphicsView_impedanceView->show();

    // Connect buttons
    connect(ui->m_pushButton_stop, &QPushButton::released, this, &TmsiImpedanceWidget::stopImpedanceMeasurement);
    connect(ui->m_pushButton_start, &QPushButton::released, this, &TmsiImpedanceWidget::startImpedanceMeasurement);
    connect(ui->m_pushButton_takeScreenshot, &QPushButton::released, this, &TmsiImpedanceWidget::takeScreenshot);
    connect(ui->m_pushButton_loadLayout, &QPushButton::released, this, &TmsiImpedanceWidget::loadLayout);
}

//*************************************************************************************************************

TmsiImpedanceWidget::~TmsiImpedanceWidget()
{
    delete ui;
}

//*************************************************************************************************************

void TmsiImpedanceWidget::updateGraphicScene(MatrixXf &matValue)
{
    for(int i = 0; i<m_qmElectrodeIndex.size(); i++)
    {
        // Find item in scene
        QVector2D position = m_qmElectrodePositions[m_qmElectrodeIndex[i]];
        //QList<QGraphicsItem *> itemList = m_scene.itemAt(QPointF(position.x(), position.y()));

        cout<<position.x()<<" "<<position.y()<<endl;

        // Repaint item depending on the current impedance value
//        if(!itemList.isEmpty())
//        {
            QGraphicsEllipseItem *item = (QGraphicsEllipseItem *)m_scene.itemAt(QPointF(position.x(), position.y()), QTransform());
            item->setBrush(QBrush(Qt::cyan));
//        }
    }
}

//*************************************************************************************************************

void TmsiImpedanceWidget::initGraphicScene()
{
    //m_scene.addText("Hello, world!");

    // Load standard layout file
    AsAElc *asaObject = new AsAElc();
    QVector< QVector<double> > elcLocation3D;
    QVector< QVector<double> > elcLocation2D;
    QString unit;
    QStringList elcChannelNames;
    QString sElcFilePath = QString("./mne_x_plugins/resources/tmsi/loc_files/standard_waveguard128.elc");

    if(!asaObject->readElcFile(sElcFilePath, elcChannelNames, elcLocation3D, elcLocation2D, unit))
        qDebug() << "Error: Reading elc file.";

    // Transform to QMap
    for(int i = 0; i<elcLocation2D.size(); i++)
    {
        m_qmElectrodePositions.insert(elcChannelNames.at(i), QVector2D(elcLocation2D[i][0], elcLocation2D[i][1]));
        m_qmElectrodeIndex.insert(i, elcChannelNames.at(i));
    }

    // Add electrodes to scene
    for(int i = 0; i<elcLocation2D.size(); i++)
    {
        QVector2D position(elcLocation2D[i][0],elcLocation2D[i][1]);
        addElectrodeItem("Test", position);
    }
}

//*************************************************************************************************************

void TmsiImpedanceWidget::addElectrodeItem(QString electrodeName, QVector2D position)
{
    TmsiElectrodeItem *item = new TmsiElectrodeItem(electrodeName, QPointF(position.x(), position.y()), QColor(qrand() % 256, qrand() % 256, qrand() % 256));
    m_scene.addItem(item);
    //m_scene.addEllipse(position.x(), position.y(), 10, 10, QPen(), QBrush(Qt::cyan));
}

//*************************************************************************************************************

void TmsiImpedanceWidget::startImpedanceMeasurement()
{
    m_pTMSI->m_bCheckImpedances = true;

    m_pTMSI->start();
}

//*************************************************************************************************************

void TmsiImpedanceWidget::stopImpedanceMeasurement()
{
    m_pTMSI->m_bCheckImpedances = false;

    m_pTMSI->stop();
}

//*************************************************************************************************************

void TmsiImpedanceWidget::takeScreenshot()
{

}

//*************************************************************************************************************

void TmsiImpedanceWidget::loadLayout()
{

}

