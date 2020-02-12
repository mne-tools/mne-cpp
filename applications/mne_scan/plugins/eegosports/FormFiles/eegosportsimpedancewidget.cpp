//=============================================================================================================
/**
* @file     eegosportsimpedancewidget.cpp
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
* @brief    Contains the declaration of the EEGoSportsImpedanceWidget class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "eegosportsimpedancewidget.h"
#include "ui_eegosportsimpedancewidget.h"

#include <utils/layoutloader.h>
#include <utils/layoutmaker.h>

#include "../eegosports.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QFileDialog>
#include <QStandardPaths>
#include <QString>
#include <QDate>
#include <QMessageBox>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace EEGOSPORTSPLUGIN;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

EEGoSportsImpedanceWidget::EEGoSportsImpedanceWidget(EEGoSports* pEEGoSports, QWidget *parent)
: m_pEEGoSports(pEEGoSports)
, QWidget(parent)
, ui(new Ui::EEGoSportsImpedanceWidget)
, m_dMaxImpedance(20000)
{
    ui->setupUi(this);

    // Init colormap object
    m_cbColorMap = QSharedPointer<ColorMap>(new ColorMap());

    // Init GUI stuff
    m_qGScene = new EEGoSportsImpedanceScene(ui->m_graphicsView_impedanceView);
    ui->m_graphicsView_impedanceView->setScene(m_qGScene);
    ui->m_graphicsView_impedanceView->show();

    ui->m_pushButton_stop->setEnabled(false);

    // Connects of this widget
    connect(ui->m_pushButton_stop, &QPushButton::released, this, &EEGoSportsImpedanceWidget::stopImpedanceMeasurement);
    connect(ui->m_pushButton_start, &QPushButton::released, this, &EEGoSportsImpedanceWidget::startImpedanceMeasurement);
    connect(ui->m_pushButton_takeScreenshot, &QPushButton::released, this, &EEGoSportsImpedanceWidget::takeScreenshot);
    connect(ui->m_pushButton_loadLayout, &QPushButton::released, this, &EEGoSportsImpedanceWidget::loadLayout);
    connect(ui->m_pushButton_saveValues, &QPushButton::released, this, &EEGoSportsImpedanceWidget::saveToFile);
    connect(ui->m_pushButton_Help, &QPushButton::released, this, &EEGoSportsImpedanceWidget::helpDialog);
    //connect(ui->m_verticalSlider_manualImpedanceValue, &QSlider::valueChanged, this, &EEGoSportsImpedanceWidget::setIm);

}

//*************************************************************************************************************

EEGoSportsImpedanceWidget::~EEGoSportsImpedanceWidget()
{
    delete ui;
}

//*************************************************************************************************************

void EEGoSportsImpedanceWidget::updateGraphicScene(VectorXd matValue)
{
    // Get scene items
    QList<QGraphicsItem *> itemList = m_qGScene->items();

    // Update color and impedance values for each electrode item
    int matIndex = 0;
    double impedanceValue = 0.0;
    int numberItems = itemList.size();

    if(itemList.size()>matValue.rows())
    {
        qDebug()<<"EEGoSportsImpedanceWidget - ERROR - There were more items in the scene than samples received from the device - Check the current layout! Only available channels will be displayed!"<<endl;
   //     qDebug()<<"itemlist.size() " << itemList.size() << " matValue.rows() " << matValue.rows() << endl;
        numberItems = matValue.rows();
        return;
    }

    // For testing purposes only!
    //impedanceValue = ui->m_verticalSlider_manualImpedanceValue->value();

    double threshold = ui->m_doubleSpinBox_manualImpedanceThreshold->value();
    double colormapmax = ui->m_doubleSpinBox_manualImpedanceColormapMax->value();

    for(int i = 0; i<numberItems; i++)
    {
        EEGoSportsElectrodeItem *item = (EEGoSportsElectrodeItem *) itemList.at(i);

        // find matrix index for given electrode name
        matIndex = m_qmElectrodeNameIndex[item->getElectrodeName()];
        impedanceValue = matValue[matIndex];

        // set new color and impedance value. Clip received impedance value if > predefined max impedance value
        if(impedanceValue>3*threshold || impedanceValue<0)
            impedanceValue = -1.0;
        //double scale = 0.000053;
        //cout << scale <<endl;
        //double valueScaledNormalized = (scale*impedanceValue)/(scale*impedanceValue+1);

        if(ui->m_radioButton_Threshold->isChecked())
        {
            QColor thresholdColor;
            if(impedanceValue > 2*threshold)
            {
                thresholdColor.setRed(255);
                thresholdColor.setGreen(140);
            }
            else if(impedanceValue > threshold)
            {
                thresholdColor.setRed(255);
                thresholdColor.setGreen(255);
            }
            else if(impedanceValue > 0)
                thresholdColor.setGreen(255);
            else
                thresholdColor.setRed(255);
            item->setColor(thresholdColor);
        }
        else if(ui->m_radioButton_Colormap->isChecked())
            item->setColor(m_cbColorMap->valueToJet(std::min(colormapmax,impedanceValue/colormapmax)));

        item->setImpedanceValue(impedanceValue);
    }

    m_qGScene->update(m_qGScene->itemsBoundingRect());
}

//*************************************************************************************************************

void EEGoSportsImpedanceWidget::initGraphicScene()
{
    // Clear all items from scene
    m_qGScene->clear();

    // Load standard layout file
//    LayoutLoader *asaObject = new LayoutLoader();
    QList<QVector<float> > elcLocation3D;
    QList<QVector<float> > elcLocation2D;
    QString unit;
    QStringList elcChannelNames;
    QString sElcFilePath = QString("../resources/general/3DLayouts/standard_waveguard66.elc");

    if(!LayoutLoader::readAsaElcFile(sElcFilePath, elcChannelNames, elcLocation3D, elcLocation2D, unit))
    {
        qDebug() << "Error: Reading elc file.";
        return;
    }

    // Generate lookup table for channel names to corresponding matrix/vector index
    for(int i = 0; i<elcLocation2D.size(); i++)
        m_qmElectrodeNameIndex.insert(elcChannelNames.at(i), i);

    // Add electrodes to scene
    for(int i = 0; i<elcLocation2D.size(); i++)
    {
        QVector2D position(elcLocation2D[i][1]*-4.5,elcLocation2D[i][0]*-4.5); // swap x y to rotate 90°, multiply to mirror by x and y axis, multiply by to scale
        addElectrodeItem(elcChannelNames.at(i), position);
    }

    ui->m_graphicsView_impedanceView->fitInView(m_qGScene->itemsBoundingRect(), Qt::KeepAspectRatio);
}

//*************************************************************************************************************

void EEGoSportsImpedanceWidget::addElectrodeItem(QString electrodeName, QVector2D position)
{
    EEGoSportsElectrodeItem *item = new EEGoSportsElectrodeItem(electrodeName, QPointF(position.x(), position.y()), QColor(m_cbColorMap->valueToJet(1)), m_qmElectrodeNameIndex[electrodeName]);
    item->setPos(QPointF(position.x(), position.y()));
    m_qGScene->addItem(item);
}

//*************************************************************************************************************

void EEGoSportsImpedanceWidget::startImpedanceMeasurement()
{
    m_pEEGoSports->m_bCheckImpedances = true;

    if(m_pEEGoSports->start())
    {
        ui->m_pushButton_stop->setEnabled(true);
        ui->m_pushButton_start->setEnabled(false);
    }
    else
        m_pEEGoSports->m_bCheckImpedances = false;
}

//*************************************************************************************************************

void EEGoSportsImpedanceWidget::stopImpedanceMeasurement()
{
    m_pEEGoSports->m_bCheckImpedances = false;

    if(m_pEEGoSports->stop())
    {
        ui->m_pushButton_stop->setEnabled(false);
        ui->m_pushButton_start->setEnabled(true);
    }
    else
        m_pEEGoSports->m_bCheckImpedances = true;
}

//*************************************************************************************************************

void EEGoSportsImpedanceWidget::takeScreenshot()
{
    // Open file dialog
    QDate date;
    QString fileName = QFileDialog::getSaveFileName(this,
                                                    "Save Screenshot",
                                                    QString("%1/%2_%3_%4_Impedances").arg(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation)).arg(date.currentDate().year()).arg(date.currentDate().month()).arg(date.currentDate().day()),
                                                    tr("Vector graphic(*.svg);;Images (*.png)"));

    if(!fileName.isEmpty())
    {
        // Generate screenshot
        if(fileName.contains(".svg"))
        {
            QSvgGenerator svgGen;

            svgGen.setFileName(fileName);
            QRectF rect = m_qGScene->itemsBoundingRect();
            svgGen.setSize(QSize(rect.width(), rect.height()));
            //svgGen.setViewBox(QRect(0, 0, rect.width(), rect.height()));

            QPainter painter(&svgGen);
            m_qGScene->render(&painter);
        }

        if(fileName.contains(".png"))
        {
            m_qGScene->setSceneRect(m_qGScene->itemsBoundingRect());                          // Re-shrink the scene to it's bounding contents
            QImage image(m_qGScene->sceneRect().size().toSize(), QImage::Format_ARGB32);  // Create the image with the exact size of the shrunk scene
            image.fill(Qt::transparent);                                              // Start all pixels transparent

            QPainter painter(&image);
            m_qGScene->render(&painter);
            image.save(fileName);
        }
    }
}

//*************************************************************************************************************

void EEGoSportsImpedanceWidget::loadLayout()
{
    QString sElcFilePath = QFileDialog::getOpenFileName(this,
                                                        tr("Open Layout"),
                                                        "./resources/mne_scan/plugins/eegosports/loc_files/",
                                                        tr("ELC layout file (*.elc)"));

    // Load standard layout file
//    LayoutLoader *asaObject = new LayoutLoader();
    QList<QVector<float> > elcLocation3D;
    QList<QVector<float> > elcLocation2D;
    QString unit;
    QStringList elcChannelNames;

    if(!LayoutLoader::readAsaElcFile(sElcFilePath, elcChannelNames, elcLocation3D, elcLocation2D, unit))
        qDebug() << "Error: Reading elc file.";
    else
        m_qGScene->clear();

    // Clean old map -> Generate lookup table for channel names and corresponding index
    m_qmElectrodeNameIndex.clear();

    for(int i = 0; i<elcLocation2D.size(); i++)
        m_qmElectrodeNameIndex.insert(elcChannelNames.at(i), i);

    // Add electrodes to scene
    for(int i = 0; i<elcLocation2D.size(); i++)
    {
        QVector2D position(elcLocation2D[i][1]*-4.5,elcLocation2D[i][0]*-4.5); // swap x y to rotate 90°, multiply to mirror by x and y axis, multiply by to scale

        addElectrodeItem(elcChannelNames.at(i), position);
    }

    ui->m_graphicsView_impedanceView->fitInView(m_qGScene->itemsBoundingRect(), Qt::KeepAspectRatio);
}

//*************************************************************************************************************

void EEGoSportsImpedanceWidget::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event);

    // On window close event -> stop impedance measurement
    if(m_pEEGoSports->m_bIsRunning)
        stopImpedanceMeasurement();
}

//*************************************************************************************************************

// This function is needed to sort the QList
bool compareChannelIndex(EEGoSportsElectrodeItem* a, EEGoSportsElectrodeItem* b)
{
    return a->getChannelIndex() < b->getChannelIndex();
}

void EEGoSportsImpedanceWidget::saveToFile()
{
    // Open file dialog
    QDate date;
    QString fileName = QFileDialog::getSaveFileName(this,
                                                    "Save impedance values",
                                                    QString("%1/%2_%3_%4_Impedances").arg(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation)).arg(date.currentDate().year()).arg(date.currentDate().month()).arg(date.currentDate().day()),
                                                    tr("Text file (*.txt)"));

    std::ofstream outputFileStream;
    outputFileStream.open(fileName.toStdString(), std::ios::trunc); //ios::trunc deletes old file data

    QList<QGraphicsItem *> itemList = m_qGScene->items();

    // Convert to QList with EEGoSportsElectrodeItem's
    QList<EEGoSportsElectrodeItem *> itemListNew;
    for(int i = 0; i<itemList.size(); i++)
        itemListNew.append((EEGoSportsElectrodeItem *)itemList.at(i));

    // Sort list corresponding to the channelIndex
    std::sort(itemListNew.begin(), itemListNew.end(), compareChannelIndex);

    // Update position
    for(int i = 0; i<itemListNew.size(); i++)
    {
        EEGoSportsElectrodeItem *item = itemListNew.at(i);
        outputFileStream << i << " " << item->getElectrodeName().toStdString() << " " << item->getImpedanceValue() << endl;
    }

    outputFileStream.close();
}

//*************************************************************************************************************

void EEGoSportsImpedanceWidget::helpDialog()
{
    QMessageBox msgBox;
    msgBox.setText("- Use mouse wheel to zoom.\n- Hold and move right mouse button to scale the electrode positions in the scene.\n- Double click to fit the scene into the view.");
    msgBox.exec();
}
