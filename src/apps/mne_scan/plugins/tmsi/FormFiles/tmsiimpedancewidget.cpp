//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2014-2026 MNE-CPP Authors
 *
 * @file     tmsiimpedancewidget.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     June, 2014
 * @brief    Contains the declaration of the TMSIImpedanceWidget class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/base/log.h>
#include "tmsiimpedancewidget.h"

#include "ui_tmsiimpedancewidget.h"
#include "../tmsi.h"
#include <mne/base/string_utils.h>
#include <mne/viz/backends/qt/plots/helpers/svgwriter.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QFileDialog>
#include <QPainter>
#include <QString>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <fstream>
#include <memory>

//=============================================================================================================
// STATIC HELPERS
//=============================================================================================================

namespace
{

QString layoutDirectory()
{
    return QCoreApplication::applicationDirPath() + "/../resources/mne_scan/plugins/tmsi/loc_files/";
}

} // anonymous namespace

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace TMSIPLUGIN;
using namespace Eigen;
using namespace mne::viz;
using namespace mne::viz::qt;
using namespace std;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

TMSIImpedanceWidget::TMSIImpedanceWidget(TMSI* pTMSI, QWidget* parent)
: QWidget(parent)
, m_pTMSI(pTMSI)
, ui(std::make_unique<Ui::TMSIImpedanceWidget>())
, m_dMaxImpedance(200000)
{
    ui->setupUi(this);

    // Init colormap object
    m_cbColorMap = std::make_shared<ColorMap>();

    // Init GUI stuff
    m_qGScene = new TMSIImpedanceScene(ui->m_graphicsView_impedanceView);
    ui->m_graphicsView_impedanceView->setScene(m_qGScene);
    ui->m_graphicsView_impedanceView->show();

    ui->m_pushButton_stop->setEnabled(false);

    // Connects of this widget
    connect(ui->m_pushButton_stop, &QPushButton::released,
            this, &TMSIImpedanceWidget::stopImpedanceMeasurement);
    connect(ui->m_pushButton_start, &QPushButton::released,
            this, &TMSIImpedanceWidget::startImpedanceMeasurement);
    connect(ui->m_pushButton_takeScreenshot, &QPushButton::released,
            this, &TMSIImpedanceWidget::takeScreenshot);
    connect(ui->m_pushButton_loadLayout, &QPushButton::released,
            this, &TMSIImpedanceWidget::loadLayout);
    connect(ui->m_pushButton_saveValues, &QPushButton::released,
            this, &TMSIImpedanceWidget::saveToFile);
    connect(ui->m_pushButton_Help, &QPushButton::released,
            this, &TMSIImpedanceWidget::helpDialog);
    //connect(ui->m_verticalSlider_manualImpedanceValue, &QSlider::valueChanged, this, &TMSIImpedanceWidget::setIm);
}

//=============================================================================================================

TMSIImpedanceWidget::~TMSIImpedanceWidget()
{
}

//=============================================================================================================

void TMSIImpedanceWidget::updateGraphicScene(const VectorXd& matValue)
{
    // Electrodes of the layout beyond the amplifier's inputs keep their colour
    const double scale = ui->m_doubleSpinBox_manualImpedanceValueScale->value();
    for (QGraphicsItem* graphicsItem : m_qGScene->items()) {
        auto* item = static_cast<TMSIElectrodeItem*>(graphicsItem);
        const int matIndex = m_qmElectrodeNameIndex.value(item->getElectrodeName(), -1);
        if (matIndex < 0 || matIndex >= matValue.size()) {
            continue;
        }
        const double impedanceValue = matValue[matIndex];
        const double valueScaledNormalized = (scale * impedanceValue) / (scale * impedanceValue + 1);

        item->setColor(m_cbColorMap->valueToJet(valueScaledNormalized));
        item->setImpedanceValue(impedanceValue);
    }

    m_qGScene->update(m_qGScene->itemsBoundingRect());
}

//=============================================================================================================

void TMSIImpedanceWidget::initGraphicScene()
{
    // Clear all items from scene
    m_qGScene->clear();

    const auto layout = mne::LayoutLoader::read_elc(mne::path_from_utf8((layoutDirectory() + "standard_waveguard128.elc").toStdString()));
    if (!layout) {
        mne::Log::debug("Error: Reading elc file.");
        return;
    }
    addElectrodeItems(*layout);

    ui->m_graphicsView_impedanceView->fitInView(m_qGScene->itemsBoundingRect(), Qt::KeepAspectRatio);
}

//=============================================================================================================

void TMSIImpedanceWidget::addElectrodeItem(const QString& electrodeName, QVector2D position)
{
    TMSIElectrodeItem* item = new TMSIElectrodeItem(electrodeName, QPointF(position.x(), position.y()), QColor(m_cbColorMap->valueToJet(1)), m_qmElectrodeNameIndex[electrodeName]);
    item->setPos(QPointF(position.x(), position.y()));
    m_qGScene->addItem(item);
}

//=============================================================================================================

void TMSIImpedanceWidget::addElectrodeItems(const mne::ElectrodeLayout& layout)
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

void TMSIImpedanceWidget::startImpedanceMeasurement()
{
    m_pTMSI->m_bCheckImpedances = true;

    if (m_pTMSI->start()) {
        ui->m_pushButton_stop->setEnabled(true);
        ui->m_pushButton_start->setEnabled(false);
    } else
        m_pTMSI->m_bCheckImpedances = false;
}

//=============================================================================================================

void TMSIImpedanceWidget::stopImpedanceMeasurement()
{
    m_pTMSI->m_bCheckImpedances = false;

    if (m_pTMSI->stop()) {
        ui->m_pushButton_stop->setEnabled(false);
        ui->m_pushButton_start->setEnabled(true);
    } else
        m_pTMSI->m_bCheckImpedances = true;
}

//=============================================================================================================

void TMSIImpedanceWidget::takeScreenshot()
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
            mne::viz::qt::SvgWriter svgGen;

            svgGen.setFileName(fileName.toStdString());
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

void TMSIImpedanceWidget::loadLayout()
{
    QString sElcFilePath = QFileDialog::getOpenFileName(this,
                                                        tr("Open Layout"),
                                                        layoutDirectory(),
                                                        tr("ELC layout file (*.elc)"));

    const auto layout = mne::LayoutLoader::read_elc(mne::path_from_utf8(sElcFilePath.toStdString()));
    if (!layout) {
        mne::Log::debug("Error: Reading elc file.");
        return;
    }
    m_qGScene->clear();
    m_qmElectrodeNameIndex.clear();
    addElectrodeItems(*layout);

    ui->m_graphicsView_impedanceView->fitInView(m_qGScene->itemsBoundingRect(), Qt::KeepAspectRatio);
}

//=============================================================================================================

void TMSIImpedanceWidget::closeEvent(QCloseEvent* event)
{
    Q_UNUSED(event);

    // On window close event -> stop impedance measurement
    if (m_pTMSI->isRunning()) {
        stopImpedanceMeasurement();
    }
}

//=============================================================================================================

// This function is needed to sort the QList
bool compareChannelIndex(TMSIElectrodeItem* a, TMSIElectrodeItem* b)
{
    return a->getChannelIndex() < b->getChannelIndex();
}

void TMSIImpedanceWidget::saveToFile()
{
    // Open file dialog
    QDate date;
    QString fileName = QFileDialog::getSaveFileName(this,
                                                    "Save impedance values",
                                                    QString("%1/%2_%3_%4_Impedances").arg(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation)).arg(date.currentDate().year()).arg(date.currentDate().month()).arg(date.currentDate().day()),
                                                    tr("Text file (*.txt)"));

    ofstream outputFileStream;
    outputFileStream.open(fileName.toStdString(), ios::trunc); //ios::trunc deletes old file data

    QList<QGraphicsItem*> itemList = m_qGScene->items();

    // Convert to QList with TMSIElectrodeItem's
    QList<TMSIElectrodeItem*> itemListNew;
    for (int i = 0; i < itemList.size(); i++)
        itemListNew.append((TMSIElectrodeItem*)itemList.at(i));

    // Sort list corresponding to the channelIndex
    sort(itemListNew.begin(), itemListNew.end(), compareChannelIndex);

    // Update position
    for (int i = 0; i < itemListNew.size(); i++) {
        TMSIElectrodeItem* item = itemListNew.at(i);
        outputFileStream << i << " " << item->getElectrodeName().toStdString() << " " << item->getImpedanceValue() << endl;
    }

    outputFileStream.close();
}

//=============================================================================================================

void TMSIImpedanceWidget::helpDialog()
{
    QMessageBox msgBox;
    msgBox.setText("- Use mouse wheel to zoom.\n- Hold and move right mouse button to scale the electrode positions in the scene.\n- Double click to fit the scene into the view.");
    msgBox.exec();
}
