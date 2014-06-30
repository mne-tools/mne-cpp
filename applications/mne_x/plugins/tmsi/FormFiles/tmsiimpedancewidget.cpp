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
* @brief    Contains the declaration of the TMSIImpedanceWidget class.
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

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

TMSIImpedanceWidget::TMSIImpedanceWidget(TMSI* p_pTMSI, QWidget *parent)
: m_pTMSI(p_pTMSI)
, QWidget(parent)
, ui(new Ui::TMSIImpedanceWidget)
{
    ui->setupUi(this);

    // Init GUI stuff
    ui->m_graphicsView_impedanceView->setScene(&m_scene);
    ui->m_graphicsView_impedanceView->show();

    ui->m_pushButton_stop->setEnabled(false);

    // Connect buttons of this widget
    connect(ui->m_pushButton_stop, &QPushButton::released, this, &TMSIImpedanceWidget::stopImpedanceMeasurement);
    connect(ui->m_pushButton_start, &QPushButton::released, this, &TMSIImpedanceWidget::startImpedanceMeasurement);
    connect(ui->m_pushButton_takeScreenshot, &QPushButton::released, this, &TMSIImpedanceWidget::takeScreenshot);
    connect(ui->m_pushButton_loadLayout, &QPushButton::released, this, &TMSIImpedanceWidget::loadLayout);
}

//*************************************************************************************************************

TMSIImpedanceWidget::~TMSIImpedanceWidget()
{
    delete ui;
}

//*************************************************************************************************************

void TMSIImpedanceWidget::updateGraphicScene(VectorXd matValue)
{
    // Get scene items
    QList<QGraphicsItem *> itemList = m_scene.items();

    // Update color and impedance values for each electrode item
    int matIndex = 0;
    double impedanceValue = 0.0;

    for(int i = 0; i<itemList.size(); i++)
    {
        TMSIElectrodeItem *item = (TMSIElectrodeItem *) itemList.at(i);

        // find matrix index for given electrode name
        matIndex = m_qmElectrodeNameIndex[item->getElectrodeName()];
        if(matIndex<matValue.rows())
        {
            impedanceValue = matValue[matIndex];

            // set new color and impedance value
            item->setColor(QColor(qrand() % 256, qrand() % 256, qrand() % 256));
            item->setImpedanceValue(impedanceValue);
        }
        else
            qDebug()<<"TmsiImpedanceWidget - ERROR - There were more items in the scene than samples received from the device - Check the current layout!"<<endl;
    }

    m_scene.update(m_scene.sceneRect());
}

//*************************************************************************************************************

void TMSIImpedanceWidget::initGraphicScene()
{
    // Clear all items from scene
    m_scene.clear();

    // Load standard layout file
    AsAElc *asaObject = new AsAElc();
    QVector< QVector<double> > elcLocation3D;
    QVector< QVector<double> > elcLocation2D;
    QString unit;
    QStringList elcChannelNames;
    QString sElcFilePath = QString("./mne_x_plugins/resources/tmsi/loc_files/standard_waveguard256.elc");

    if(!asaObject->readElcFile(sElcFilePath, elcChannelNames, elcLocation3D, elcLocation2D, unit))
    {
        qDebug() << "Error: Reading elc file.";
        return;
    }

    // Generate lookup table for channel names and index and corresponding index
    for(int i = 0; i<elcLocation2D.size(); i++)
        m_qmElectrodeNameIndex.insert(elcChannelNames.at(i), i);

    // Add electrodes to scene
    for(int i = 0; i<elcLocation2D.size(); i++)
    {
        QVector2D position(elcLocation2D[i][0],elcLocation2D[i][1]);
        addElectrodeItem(elcChannelNames.at(i), position, QColor(qrand() % 256, qrand() % 256, qrand() % 256));
    }
}

//*************************************************************************************************************

void TMSIImpedanceWidget::addElectrodeItem(QString electrodeName, QVector2D position, QColor color)
{
    TMSIElectrodeItem *item = new TMSIElectrodeItem(electrodeName, QPointF(position.x(), position.y()), color);
    m_scene.addItem(item);
}

//*************************************************************************************************************

void TMSIImpedanceWidget::startImpedanceMeasurement()
{
    if(m_pTMSI->start())
    {
        m_pTMSI->m_bCheckImpedances = true;
        ui->m_pushButton_stop->setEnabled(true);
        ui->m_pushButton_start->setEnabled(false);
    }
}

//*************************************************************************************************************

void TMSIImpedanceWidget::stopImpedanceMeasurement()
{
    if(m_pTMSI->stop())
    {
        m_pTMSI->m_bCheckImpedances = false;
        ui->m_pushButton_stop->setEnabled(false);
        ui->m_pushButton_start->setEnabled(true);
    }
}

//*************************************************************************************************************

void TMSIImpedanceWidget::takeScreenshot()
{
    // Open file dialog
    QDate date;
    QString fileName = QFileDialog::getSaveFileName(this,
                                                    "Save Screenshot",
                                                    QString("%1/%2_%3_%4_Impedances").arg(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation)).arg(date.currentDate().year()).arg(date.currentDate().month()).arg(date.currentDate().day()),
                                                    tr("Vector graphic(*.svg);;Images (*.png)"));

    if(!fileName.isEmpty())
    {
        // scale view in a way that all items are visible for the screenshot, then transform back
        QTransform temp = ui->m_graphicsView_impedanceView->transform();
        ui->m_graphicsView_impedanceView->fitInView(m_scene.sceneRect(), Qt::KeepAspectRatio);

        // Generate screenshot
        if(fileName.contains(".svg"))
        {
            QSvgGenerator svgGen;

            svgGen.setFileName(fileName);
            QRectF rect = m_scene.sceneRect();
            svgGen.setSize(QSize(rect.height(), rect.width()));
            svgGen.setViewBox(QRect(0, 0, rect.height(), rect.width()));

            QPainter painter(&svgGen);
            m_scene.render(&painter);
        }

        if(fileName.contains(".png"))
        {
            QPixmap pixMap = QPixmap::grabWidget(ui->m_graphicsView_impedanceView);
            pixMap.save(fileName);
        }

        ui->m_graphicsView_impedanceView->setTransform(temp);
    }
}

//*************************************************************************************************************

void TMSIImpedanceWidget::loadLayout()
{
    QString sElcFilePath = QFileDialog::getOpenFileName(this,
                                                        tr("Open Layout"),
                                                        "./mne_x_plugins/resources/tmsi/loc_files/",
                                                        tr("Layout Files (*.elc)"));

    // Load standard layout file
    AsAElc *asaObject = new AsAElc();
    QVector< QVector<double> > elcLocation3D;
    QVector< QVector<double> > elcLocation2D;
    QString unit;
    QStringList elcChannelNames;

    if(!asaObject->readElcFile(sElcFilePath, elcChannelNames, elcLocation3D, elcLocation2D, unit))
        qDebug() << "Error: Reading elc file.";
    else
        m_scene.clear();

    // Clean old map -> Generate lookup table for channel names and corresponding index
    m_qmElectrodeNameIndex.clear();

    for(int i = 0; i<elcLocation2D.size(); i++)
        m_qmElectrodeNameIndex.insert(elcChannelNames.at(i), i);

    // Add electrodes to scene
    for(int i = 0; i<elcLocation2D.size(); i++)
    {
        QVector2D position(elcLocation2D[i][0],elcLocation2D[i][1]);
        addElectrodeItem(elcChannelNames.at(i), position, QColor(qrand() % 256, qrand() % 256, qrand() % 256));
    }
}

