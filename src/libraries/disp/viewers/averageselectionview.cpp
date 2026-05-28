//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *
 * @file     averageselectionview.cpp
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     July 2018
 * @brief    Implementation of the AverageSelectionView picker.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "averageselectionview.h"

#include <fiff/fiff_evoked_set.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QGridLayout>
#include <QCheckBox>
#include <QColorDialog>
#include <QPalette>
#include <QPushButton>
#include <QDebug>
#include <QSettings>
#include <QPointer>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include<Eigen/Core>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

AverageSelectionView::AverageSelectionView(const QString& sSettingsPath,
                                           QWidget *parent,
                                           Qt::WindowFlags f)
: AbstractView(parent, f)
, m_iMaxNumAverages(10)
, m_qMapAverageColor(QSharedPointer<QMap<QString, QColor> >::create())
, m_qMapAverageActivation(QSharedPointer<QMap<QString, bool> >::create())
{
    m_sSettingsPath = sSettingsPath;
    this->setWindowTitle("Average Selection");
    this->setMinimumWidth(330);
    this->setMaximumWidth(330);

    loadSettings();
    redrawGUI();
}

//=============================================================================================================

AverageSelectionView::~AverageSelectionView()
{
    saveSettings();
}

//=============================================================================================================

QSharedPointer<QMap<QString, QColor> > AverageSelectionView::getAverageColor() const
{
    return m_qMapAverageColor;
}

//=============================================================================================================

QSharedPointer<QMap<QString, bool> > AverageSelectionView::getAverageActivation() const
{
    return m_qMapAverageActivation;
}

//=============================================================================================================

void AverageSelectionView::setAverageColor(const QSharedPointer<QMap<QString, QColor> > qMapAverageColor)
{
    m_qMapAverageColor = qMapAverageColor;
    redrawGUI();
}

//=============================================================================================================

void AverageSelectionView::setAverageActivation(const QSharedPointer<QMap<QString, bool> > qMapAverageActivation)
{
    m_qMapAverageActivation = qMapAverageActivation;
    redrawGUI();
}

//=============================================================================================================

void AverageSelectionView::saveSettings()
{
    if(m_sSettingsPath.isEmpty()) {
        return;
    }

    QSettings settings("MNECPP");

    settings.beginGroup(m_sSettingsPath + QString("/AverageSelectionView/averageColorMap"));
    QMap<QString, QColor>::const_iterator iColor = m_qMapAverageColor->constBegin();
    while (iColor != m_qMapAverageColor->constEnd()) {
         settings.setValue(iColor.key(), iColor.value());
         ++iColor;
    }
    settings.endGroup();

    settings.beginGroup(m_sSettingsPath + QString("/AverageSelectionView/averageActivationMap"));
    QMap<QString, bool>::const_iterator iActivation = m_qMapAverageActivation->constBegin();
    while (iActivation != m_qMapAverageActivation->constEnd()) {
         settings.setValue(iActivation.key(), iActivation.value());
         ++iActivation;
    }
    settings.endGroup();
}

//=============================================================================================================

void AverageSelectionView::loadSettings()
{
    if(m_sSettingsPath.isEmpty()) {
        return;
    }

    QSettings settings("MNECPP");

    settings.beginGroup(m_sSettingsPath + QString("/AverageSelectionView/averageColorMap"));
    QStringList keys = settings.childKeys();
    foreach (QString key, keys) {
         m_qMapAverageColor->insert(key, settings.value(key).value<QColor>());
    }
    settings.endGroup();

    settings.beginGroup(m_sSettingsPath + QString("/AverageSelectionView/averageActivationMap"));
    keys = settings.childKeys();
    foreach (QString key, keys) {
         m_qMapAverageActivation->insert(key, settings.value(key).toBool());
    }
    settings.endGroup();
}

//=============================================================================================================

void AverageSelectionView::updateGuiMode(GuiMode mode)
{
    switch(mode) {
        case GuiMode::Clinical:
            break;
        default: // default is research mode
            break;
    }
}

//=============================================================================================================

void AverageSelectionView::updateProcessingMode(ProcessingMode mode)
{
    switch(mode) {
        case ProcessingMode::Offline:
            break;
        default: // default is realtime mode
            break;
    }
}

//=============================================================================================================

void AverageSelectionView::redrawGUI()
{
    if(m_qMapAverageColor->size() != m_qMapAverageActivation->size()) {
        qDebug() << "AverageSelectionView::update - m_qMapAverageColor and m_qMapAverageActivation do not match in size. Returning.";
        return;
    }

    //Delete all widgets in the averages layout
    QGridLayout* topLayout = static_cast<QGridLayout*>(this->layout());
    if(!topLayout) {
       topLayout = new QGridLayout();
    }

    QLayoutItem *child;
    while ((child = topLayout->takeAt(0)) != 0) {
        delete child->widget();
        delete child;
    }

    // Create new GUI elements
    QMapIterator<QString, QColor> itr(*m_qMapAverageColor);
    int count = 0;
    while(itr.hasNext()) {
        if(count >= m_iMaxNumAverages) {
            break;
        }

        itr.next();

        //Create average active checkbox
        QPointer<QCheckBox> pCheckBox = new QCheckBox(itr.key());
        pCheckBox->setChecked(m_qMapAverageActivation->value(itr.key()));
        pCheckBox->setObjectName(itr.key());
        topLayout->addWidget(pCheckBox, count, 0);
        connect(pCheckBox.data(), &QCheckBox::clicked,
                this, &AverageSelectionView::onAverageSelectionColorChanged);

        //Create average color pushbutton
        QColor color = itr.value();
        QPointer<QPushButton> pButton = new QPushButton("Click to change");
        pButton->setObjectName(itr.key());
        pButton->setStyleSheet(QString("background-color: rgb(%1, %2, %3);").arg(color.red()).arg(color.green()).arg(color.blue()));
        topLayout->addWidget(pButton, count, 1);
        connect(pButton.data(), &QPushButton::clicked,
                this, &AverageSelectionView::onAverageSelectionColorChanged);

        ++count;
    }

    this->setLayout(topLayout);
}

//=============================================================================================================

void AverageSelectionView::onAverageSelectionColorChanged()
{
    //Change color for average
    if(QPointer<QPushButton> button = qobject_cast<QPushButton*>(sender())) {
        QString sObjectName = button->objectName();

        QColor color = QColorDialog::getColor(m_qMapAverageColor->value(sObjectName), this, "Set average color");

        if(button) {
            QPalette palette(QPalette::Button,color);
            button->setPalette(palette);
            button->update();

            //Set color of button new new scene color
            button->setStyleSheet(QString("background-color: rgb(%1, %2, %3);").arg(color.red()).arg(color.green()).arg(color.blue()));
        }

        m_qMapAverageColor->insert(sObjectName, color);

        emit newAverageColorMap(m_qMapAverageColor);
    }

    //Change color for average
    if(QPointer<QCheckBox> checkBox = qobject_cast<QCheckBox*>(sender())) {
        QString sObjectName = checkBox->objectName();

        m_qMapAverageActivation->insert(sObjectName, checkBox->isChecked());

        emit newAverageActivationMap(m_qMapAverageActivation);
    }

    saveSettings();
}

//=============================================================================================================

void AverageSelectionView::clearView()
{

}
