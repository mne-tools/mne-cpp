//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2014-2026 MNE-CPP Authors
 *
 * @file     eegosportsimpedancewidget.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Johannes Vorwerk <johannes.vorwerk@umit.at>
 * @since    0.1.0
 * @date     July, 2014
 * @brief    Contains the declaration of the EEGoSportsImpedanceWidget class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "eegosportsimpedancewidget.h"
#include <fstream>
#include <memory>
#include "ui_eegosportsimpedancewidget.h"

#include <base/string_utils.h>
#include <channels/layoutloader.h>
#include <channels/layoutmaker.h>

#include "../eegosports.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QFileDialog>
#include <QStandardPaths>
#include <QString>
#include <QDate>
#include <QMessageBox>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace EEGOSPORTSPLUGIN;

//=============================================================================================================
// STATIC HELPERS
//=============================================================================================================

namespace
{

QString layoutDirectory()
{
    return QCoreApplication::applicationDirPath() + "/../resources/general/3DLayouts/";
}

} // anonymous namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

EEGoSportsImpedanceWidget::EEGoSportsImpedanceWidget(EEGoSports* pEEGoSports, QWidget* parent)
: QWidget(parent)
, m_pEEGoSports(pEEGoSports)
, m_pUi(std::make_unique<Ui::EEGoSportsImpedanceWidget>())
, m_dMaxImpedance(20000)
{
    m_pUi->setupUi(this);

    // Init colormap object
    m_cbColorMap = std::make_shared<ColorMap>();

    // Init GUI stuff
    m_qGScene = new EEGoSportsImpedanceScene(m_pUi->m_graphicsView_impedanceView);
    m_pUi->m_graphicsView_impedanceView->setScene(m_qGScene);
    m_pUi->m_graphicsView_impedanceView->show();

    m_pUi->m_pushButton_stop->setEnabled(false);

    // Connects of this widget
    connect(m_pUi->m_pushButton_stop, &QPushButton::released, this, &EEGoSportsImpedanceWidget::stopImpedanceMeasurement);
    connect(m_pUi->m_pushButton_start, &QPushButton::released, this, &EEGoSportsImpedanceWidget::startImpedanceMeasurement);
    connect(m_pUi->m_pushButton_takeScreenshot, &QPushButton::released, this, &EEGoSportsImpedanceWidget::takeScreenshot);
    connect(m_pUi->m_pushButton_loadLayout, &QPushButton::released, this, &EEGoSportsImpedanceWidget::loadLayout);
    connect(m_pUi->m_pushButton_saveValues, &QPushButton::released, this, &EEGoSportsImpedanceWidget::saveToFile);
    connect(m_pUi->m_pushButton_Help, &QPushButton::released, this, &EEGoSportsImpedanceWidget::helpDialog);
    //connect(m_pUi->m_verticalSlider_manualImpedanceValue, &QSlider::valueChanged, this, &EEGoSportsImpedanceWidget::setIm);
}

//=============================================================================================================

EEGoSportsImpedanceWidget::~EEGoSportsImpedanceWidget()
{
    if (m_pEEGoSports->m_bCheckImpedances)
        stopImpedanceMeasurement();
}

//=============================================================================================================

void EEGoSportsImpedanceWidget::updateGraphicScene(const VectorXd& matValue)
{
    const double threshold = m_pUi->m_doubleSpinBox_manualImpedanceThreshold->value();
    const double colormapmax = m_pUi->m_doubleSpinBox_manualImpedanceColormapMax->value();

    // Electrodes of the layout beyond the amplifier's inputs keep their colour
    for (QGraphicsItem* graphicsItem : m_qGScene->items()) {
        auto* item = static_cast<EEGoSportsElectrodeItem*>(graphicsItem);
        const int matIndex = m_qmElectrodeNameIndex.value(item->getElectrodeName(), -1);
        if (matIndex < 0 || matIndex >= matValue.size()) {
            continue;
        }
        double impedanceValue = matValue[matIndex];

        // set new color and impedance value. Clip received impedance value if > predefined max impedance value
        if (impedanceValue > 3 * threshold || impedanceValue < 0)
            impedanceValue = -1.0;
        //double scale = 0.000053;
        //cout << scale <<endl;
        //double valueScaledNormalized = (scale*impedanceValue)/(scale*impedanceValue+1);

        if (m_pUi->m_radioButton_Threshold->isChecked()) {
            QColor thresholdColor;
            if (impedanceValue > 2 * threshold) {
                thresholdColor.setRed(255);
                thresholdColor.setGreen(140);
            } else if (impedanceValue > threshold) {
                thresholdColor.setRed(255);
                thresholdColor.setGreen(255);
            } else if (impedanceValue > 0)
                thresholdColor.setGreen(255);
            else
                thresholdColor.setRed(255);
            item->setColor(thresholdColor);
        } else if (m_pUi->m_radioButton_Colormap->isChecked())
            item->setColor(m_cbColorMap->valueToJet(std::min(colormapmax, impedanceValue / colormapmax)));

        item->setImpedanceValue(impedanceValue);
    }

    m_qGScene->update(m_qGScene->itemsBoundingRect());
}

//=============================================================================================================

void EEGoSportsImpedanceWidget::initGraphicScene()
{
    // Clear all items from scene
    m_qGScene->clear();

    // Load standard layout file
    const auto layout = mne::LayoutLoader::read_elc(mne::pathFromUtf8((layoutDirectory() + "standard_waveguard66.elc").toStdString()));
    if (!layout) {
        qDebug() << "Error: Reading elc file.";
        return;
    }
    addElectrodeItems(*layout);

    m_pUi->m_graphicsView_impedanceView->fitInView(m_qGScene->itemsBoundingRect(), Qt::KeepAspectRatio);
}

//=============================================================================================================

void EEGoSportsImpedanceWidget::addElectrodeItem(const QString& electrodeName, const QVector2D& position)
{
    EEGoSportsElectrodeItem* item = new EEGoSportsElectrodeItem(electrodeName, QPointF(position.x(), position.y()), QColor(m_cbColorMap->valueToJet(1)), m_qmElectrodeNameIndex[electrodeName]);
    item->setPos(QPointF(position.x(), position.y()));
    m_qGScene->addItem(item);
}

//=============================================================================================================

void EEGoSportsImpedanceWidget::addElectrodeItems(const mne::ElectrodeLayout& layout)
{
    for (std::size_t i = 0; i < layout.positions_2d.size(); ++i)
        m_qmElectrodeNameIndex.insert(QString::fromStdString(layout.names[i]), static_cast<int>(i));

    // swap x y to rotate 90°, multiply to mirror by x and y axis, multiply by to scale
    for (std::size_t i = 0; i < layout.positions_2d.size(); ++i) {
        const Eigen::Vector2d& position = layout.positions_2d[i];
        addElectrodeItem(QString::fromStdString(layout.names[i]), QVector2D(static_cast<float>(position.y() * -4.5), static_cast<float>(position.x() * -4.5)));
    }
}

//=============================================================================================================

void EEGoSportsImpedanceWidget::startImpedanceMeasurement()
{
    m_pEEGoSports->m_bCheckImpedances = true;

    if (m_pEEGoSports->start()) {
        m_pUi->m_pushButton_stop->setEnabled(true);
        m_pUi->m_pushButton_start->setEnabled(false);
    } else {
        m_pEEGoSports->m_bCheckImpedances = false;
        QMessageBox::information(this, tr("MNE Scan - Start"), tr("Not able to start the impedance measurement.\nSwitch the amplifier on and check that the eego SDK is installed."), QMessageBox::Ok);
        return;
    }
}

//=============================================================================================================

void EEGoSportsImpedanceWidget::stopImpedanceMeasurement()
{
    m_pEEGoSports->m_bCheckImpedances = false;

    if (m_pEEGoSports->stop()) {
        m_pUi->m_pushButton_stop->setEnabled(false);
        m_pUi->m_pushButton_start->setEnabled(true);
    } else
        m_pEEGoSports->m_bCheckImpedances = true;
}

//=============================================================================================================

void EEGoSportsImpedanceWidget::takeScreenshot()
{
    // Open file dialog
    QDate date;
    QString fileName = QFileDialog::getSaveFileName(this,
                                                    "Save Screenshot",
                                                    QString("%1/%2_%3_%4_Impedances").arg(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation)).arg(date.currentDate().year()).arg(date.currentDate().month()).arg(date.currentDate().day()),
                                                    tr("Vector graphic(*.svg);;Images (*.png)"));

    if (!fileName.isEmpty()) {
        // Generate screenshot
        if (fileName.contains(".svg")) {
            QSvgGenerator svgGen;

            svgGen.setFileName(fileName);
            QRectF rect = m_qGScene->itemsBoundingRect();
            svgGen.setSize(QSize(rect.width(), rect.height()));
            //svgGen.setViewBox(QRect(0, 0, rect.width(), rect.height()));

            QPainter painter(&svgGen);
            m_qGScene->render(&painter);
        }

        if (fileName.contains(".png")) {
            m_qGScene->setSceneRect(m_qGScene->itemsBoundingRect());                     // Re-shrink the scene to it's bounding contents
            QImage image(m_qGScene->sceneRect().size().toSize(), QImage::Format_ARGB32); // Create the image with the exact size of the shrunk scene
            image.fill(Qt::transparent);                                                 // Start all pixels transparent

            QPainter painter(&image);
            m_qGScene->render(&painter);
            image.save(fileName);
        }
    }
}

//=============================================================================================================

void EEGoSportsImpedanceWidget::loadLayout()
{
    QString sElcFilePath = QFileDialog::getOpenFileName(this,
                                                        tr("Open Layout"),
                                                        layoutDirectory(),
                                                        tr("ELC layout file (*.elc)"));

    // Load standard layout file
    const auto layout = mne::LayoutLoader::read_elc(mne::pathFromUtf8(sElcFilePath.toStdString()));
    if (!layout) {
        qDebug() << "Error: Reading elc file.";
        return;
    }
    m_qGScene->clear();
    m_qmElectrodeNameIndex.clear();
    addElectrodeItems(*layout);

    m_pUi->m_graphicsView_impedanceView->fitInView(m_qGScene->itemsBoundingRect(), Qt::KeepAspectRatio);
}

//=============================================================================================================

void EEGoSportsImpedanceWidget::closeEvent(QCloseEvent* event)
{
    Q_UNUSED(event);

    // On window close event -> stop impedance measurement
    if (m_pEEGoSports->isRunning()) {
        stopImpedanceMeasurement();
    }
}

//=============================================================================================================

bool EEGoSportsImpedanceWidget::compareChannelIndex(EEGoSportsElectrodeItem* a, EEGoSportsElectrodeItem* b)
{
    return a->getChannelIndex() < b->getChannelIndex();
}

//=============================================================================================================

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

    QList<QGraphicsItem*> itemList = m_qGScene->items();

    // Convert to QList with EEGoSportsElectrodeItem's
    QList<EEGoSportsElectrodeItem*> itemListNew;
    for (int i = 0; i < itemList.size(); i++)
        itemListNew.append((EEGoSportsElectrodeItem*)itemList.at(i));

    // Sort list corresponding to the channelIndex
    std::sort(itemListNew.begin(), itemListNew.end(), EEGoSportsImpedanceWidget::compareChannelIndex);

    // Update position
    for (int i = 0; i < itemListNew.size(); i++) {
        EEGoSportsElectrodeItem* item = itemListNew.at(i);
        outputFileStream << i << " " << item->getElectrodeName().toStdString() << " " << item->getImpedanceValue() << std::endl;
    }

    outputFileStream.close();
}

//=============================================================================================================

void EEGoSportsImpedanceWidget::helpDialog()
{
    QMessageBox msgBox;
    msgBox.setText("- Use mouse wheel to zoom.\n- Hold and move right mouse button to scale the electrode positions in the scene.\n- Double click to fit the scene into the view.");
    msgBox.exec();
}
