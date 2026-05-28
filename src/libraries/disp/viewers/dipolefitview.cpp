//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2020-2026 MNE-CPP Authors
 *
 * @file     dipolefitview.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>
 * @author   Andreas Griesshammer <ag@fieldlineinc.com>
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.8
 * @date     November 2020
 * @brief    Implementation of the DipoleFitView dipole-fit input / result panel.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "dipolefitview.h"
#include "ui_dipolefitview.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDateTime>
#include <QtDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

DipoleFitView::DipoleFitView(QWidget *parent,
                             Qt::WindowFlags)
: AbstractView(parent)
, m_pUi(new Ui::DipoleFitViewWidget)
{
    m_pUi->setupUi(this);
    this->setMinimumWidth(330);

    initGui();
}

//=============================================================================================================

void DipoleFitView::saveSettings()
{

}

//=============================================================================================================

void DipoleFitView::loadSettings()
{

}

//=============================================================================================================

void DipoleFitView::updateGuiMode(GuiMode mode)
{
    switch(mode) {
        case GuiMode::Clinical:
            break;
        default: // default is research mode
            break;
    }
}

//=============================================================================================================

void DipoleFitView::updateProcessingMode(ProcessingMode mode)
{
    switch(mode) {
        case ProcessingMode::Offline:
            break;
        default: // default is realtime mode
            break;
    }
}

//=============================================================================================================

void DipoleFitView::addBem(const QString &sFileName)
{
    m_pUi->comboBox_bem->addItem(sFileName);
}

//=============================================================================================================

void DipoleFitView::addMri(const QString &sFileName)
{
    m_pUi->comboBox_mri->addItem(sFileName);
}

//=============================================================================================================

void DipoleFitView::addNoise(const QString &sFileName)
{
    m_pUi->comboBox_noise->addItem(sFileName);
}

//=============================================================================================================

void DipoleFitView::requestParams()
{
    emit timeChanged(m_pUi->spinBox_tmin->value(),
                     m_pUi->spinBox_tmax->value(),
                     m_pUi->spinBox_tstep->value(),
                     m_pUi->spinBox_tint->value());

    emit modalityChanged(m_pUi->checkBox_EEG->isChecked(), m_pUi->checkBox_MEG->isChecked());

    emit fittingChanged(m_pUi->doubleSpinBox_dist->value(),
                        m_pUi->doubleSpinBox_grid->value());

    emit baselineChanged(m_pUi->spinBox_bmax->value(),
                         m_pUi->spinBox_bmax->value());

    emit noiseChanged(m_pUi->doubleSpinBox_gradnoise->value(),
                      m_pUi->doubleSpinBox_magnoise->value(),
                      m_pUi->doubleSpinBox_eegnoise->value());

    emit regChanged(m_pUi->doubleSpinBox_gradreg->value(),
                    m_pUi->doubleSpinBox_magreg->value(),
                    m_pUi->doubleSpinBox_eegreg->value());

    emit sphereChanged(m_pUi->doubleSpinBox_orgx->value(),
                       m_pUi->doubleSpinBox_orgy->value(),
                       m_pUi->doubleSpinBox_orgz->value(),
                       m_pUi->doubleSpinBox_rad->value());
}

//=============================================================================================================

void DipoleFitView::initGui()
{
    //Init Combo boxes
    m_pUi->comboBox_bem->addItem("None");
    m_pUi->comboBox_noise->addItem("None");
    m_pUi->comboBox_mri->addItem("None");
    m_pUi->comboBox_meas->addItem("None");

    //Perform Fit
    connect(m_pUi->pushButton_fit, &QPushButton::clicked, [=, this] {
            emit performDipoleFit(m_pUi->lineEdit_name->text());
            });

    connect(m_pUi->spinBox_set, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &DipoleFitView::setChanged, Qt::UniqueConnection);

    //Time settings
    connect(m_pUi->spinBox_tmin, QOverload<int>::of(&QSpinBox::valueChanged), [=, this](int iValue){
                emit timeChanged(iValue,
                                 m_pUi->spinBox_tmax->value(),
                                 m_pUi->spinBox_tstep->value(),
                                 m_pUi->spinBox_tint->value());
            });
    connect(m_pUi->spinBox_tmax, QOverload<int>::of(&QSpinBox::valueChanged), [=, this](int iValue){
                emit timeChanged(m_pUi->spinBox_tmin->value(),
                                 iValue,
                                 m_pUi->spinBox_tstep->value(),
                                 m_pUi->spinBox_tint->value());
            });
    connect(m_pUi->spinBox_tstep, QOverload<int>::of(&QSpinBox::valueChanged), [=, this](int iValue){
                emit timeChanged(m_pUi->spinBox_tmin->value(),
                                 m_pUi->spinBox_tmax->value(),
                                 iValue,
                                 m_pUi->spinBox_tint->value());
            });
    connect(m_pUi->spinBox_tint, QOverload<int>::of(&QSpinBox::valueChanged), [=, this](int iValue){
                emit timeChanged(m_pUi->spinBox_tmin->value(),
                                 m_pUi->spinBox_tmax->value(),
                                 m_pUi->spinBox_tstep->value(),
                                 iValue);
            });

    //Baseline
    connect(m_pUi->spinBox_bmin, QOverload<int>::of(&QSpinBox::valueChanged), [=, this](int iValue){
                if (iValue != m_pUi->spinBox_bmax->value()){
                    emit baselineChanged(iValue,
                                         m_pUi->spinBox_bmax->value());
                } else {
                    emit baselineChanged(1e6, 1e6);
                }
            });
    connect(m_pUi->spinBox_bmax, QOverload<int>::of(&QSpinBox::valueChanged), [=, this](int iValue){
                if (iValue != m_pUi->spinBox_bmin->value()){
                    emit baselineChanged(m_pUi->spinBox_bmin->value(),
                                         iValue);
                } else {
                    emit baselineChanged(1e6, 1e6);
                }
            });

    //Modality
    connect(m_pUi->checkBox_EEG, &QCheckBox::toggled, [=, this](bool bChecked){
                emit modalityChanged(bChecked, m_pUi->checkBox_MEG->isChecked());
            });
    connect(m_pUi->checkBox_MEG, &QCheckBox::toggled, [=, this](bool bChecked){
                emit modalityChanged(m_pUi->checkBox_EEG->isChecked(), bChecked);
            });

    //Fittings
    connect(m_pUi->doubleSpinBox_dist, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [=, this](double dValue){
                emit fittingChanged(dValue,
                                    m_pUi->doubleSpinBox_grid->value());
            });
    connect(m_pUi->doubleSpinBox_grid, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [=, this](double dValue){
                emit fittingChanged(m_pUi->doubleSpinBox_dist->value(),
                                    dValue);
            });

    //Noise
    connect(m_pUi->doubleSpinBox_gradnoise, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [=, this](double dValue){
                emit noiseChanged(dValue,
                                  m_pUi->doubleSpinBox_magnoise->value(),
                                  m_pUi->doubleSpinBox_eegnoise->value());
            });
    connect(m_pUi->doubleSpinBox_magnoise, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [=, this](double dValue){
                emit noiseChanged(m_pUi->doubleSpinBox_gradnoise->value(),
                                  dValue,
                                  m_pUi->doubleSpinBox_eegnoise->value());
            });
    connect(m_pUi->doubleSpinBox_eegnoise, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [=, this](double dValue){
                emit noiseChanged(m_pUi->doubleSpinBox_gradnoise->value(),
                                  m_pUi->doubleSpinBox_magnoise->value(),
                                  dValue);
            });

    //Reg
    connect(m_pUi->doubleSpinBox_gradreg, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [=, this](double dValue){
                emit regChanged(dValue,
                                m_pUi->doubleSpinBox_magreg->value(),
                                m_pUi->doubleSpinBox_eegreg->value());
            });
    connect(m_pUi->doubleSpinBox_magreg, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [=, this](double dValue){
                emit regChanged(m_pUi->doubleSpinBox_gradreg->value(),
                                dValue,
                                m_pUi->doubleSpinBox_eegreg->value());
            });
    connect(m_pUi->doubleSpinBox_eegreg, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [=, this](double dValue){
                emit regChanged(m_pUi->doubleSpinBox_gradreg->value(),
                                m_pUi->doubleSpinBox_magreg->value(),
                                dValue);
            });

    //Sphere model
    connect(m_pUi->doubleSpinBox_orgx, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [=, this](double dValue){
                emit sphereChanged(dValue,
                                   m_pUi->doubleSpinBox_orgy->value(),
                                   m_pUi->doubleSpinBox_orgz->value(),
                                   m_pUi->doubleSpinBox_rad->value());
            });
    connect(m_pUi->doubleSpinBox_orgy, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [=, this](double dValue){
                emit sphereChanged(m_pUi->doubleSpinBox_orgx->value(),
                                   dValue,
                                   m_pUi->doubleSpinBox_orgz->value(),
                                   m_pUi->doubleSpinBox_rad->value());
            });
    connect(m_pUi->doubleSpinBox_orgz, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [=, this](double dValue){
                emit sphereChanged(m_pUi->doubleSpinBox_orgx->value(),
                                   m_pUi->doubleSpinBox_orgy->value(),
                                   dValue,
                                   m_pUi->doubleSpinBox_rad->value());
            });
    connect(m_pUi->doubleSpinBox_rad, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [=, this](double dValue){
                emit sphereChanged(m_pUi->doubleSpinBox_orgx->value(),
                                   m_pUi->doubleSpinBox_orgy->value(),
                                   m_pUi->doubleSpinBox_orgz->value(),
                                   dValue);
            });

    //Models
    connect(m_pUi->comboBox_bem, &QComboBox::currentTextChanged,
            this, &DipoleFitView::selectedBem, Qt::UniqueConnection);
    connect(m_pUi->comboBox_noise, &QComboBox::currentTextChanged,
            this, &DipoleFitView::selectedNoise, Qt::UniqueConnection);
    connect(m_pUi->comboBox_mri, &QComboBox::currentTextChanged,
            this, &DipoleFitView::selectedMri, Qt::UniqueConnection);
    connect(m_pUi->comboBox_meas, &QComboBox::currentTextChanged, [=, this](const QString& sFileName){
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
            auto skip = QString::SkipEmptyParts;
#else
            auto skip = Qt::SkipEmptyParts;
#endif
            QString sName = sFileName.split(".",skip).at(0);
            if(sName.endsWith("-ave") || sName.endsWith("_ave") || sName.endsWith("-raw") || sName.endsWith("_raw")){
                sName.chop(4);
            }
            m_pUi->lineEdit_name->setText("Dipole Fit - " + sName + " - " + QDateTime::currentDateTime().toString("MMMM d yyyy hh:mm:ss"));
            emit selectedMeas(sFileName);
    });

}

//=============================================================================================================

void DipoleFitView::addMeas(const QString &sFileName)
{
    m_pUi->comboBox_meas->addItem(sFileName);

//    QString sName = sFileName.split(".",QString::SkipEmptyParts).at(0);
//    if(sName.endsWith("-ave") || sName.endsWith("_ave") || sName.endsWith("-raw") || sName.endsWith("_raw")){
//        sName.chop(4);
//    }

//    m_pUi->lineEdit_name->setText("Dipole Fit - " + sName + " - " + QDateTime::currentDateTime().toString("MMMM d yyyy hh:mm:ss"));
}

//=============================================================================================================

void DipoleFitView::clearView()
{
    m_pUi->comboBox_meas->clear();
    m_pUi->comboBox_bem->clear();
    m_pUi->comboBox_mri->clear();
    m_pUi->comboBox_noise->clear();
}

//=============================================================================================================

void DipoleFitView::removeModel(const QString &sModelName, int iType)
{
    switch (iType){
    case 1: {
        int iIndex = m_pUi->comboBox_meas->findText(sModelName);
        if(iIndex != -1){
            m_pUi->comboBox_meas->removeItem(iIndex);
        }
        break;
    }
    case 2: {
        int iIndex = m_pUi->comboBox_bem->findText(sModelName);
        if(iIndex != -1){
            m_pUi->comboBox_bem->removeItem(iIndex);
        }
        break;
    }
    case 3: {
        int iIndex = m_pUi->comboBox_mri->findText(sModelName);
        if(iIndex != -1){
            m_pUi->comboBox_mri->removeItem(iIndex);
        }
        break;
    }
    case 4: {
        int iIndex = m_pUi->comboBox_noise->findText(sModelName);
        if(iIndex != -1){
            m_pUi->comboBox_noise->removeItem(iIndex);
        }
        break;
    }
    default:{
        qWarning() << "[DipoleFitView::removeModel] Model type not recognized";
        break;
    }
    }
}
