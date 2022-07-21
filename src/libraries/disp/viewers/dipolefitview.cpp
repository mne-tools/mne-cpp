//=============================================================================================================
/**
 * @file     dipolefitview.cpp
 * @author   Gabriel Motta <gbmotta@mgh.harvard.edu>
 * @since    0.1.7
 * @date     Novemeber, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020, Gabriel Motta. All rights reserved.
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
 * @brief    Definition of the DipoleFitView Class.
 *
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
    connect(m_pUi->pushButton_fit, &QPushButton::clicked, [=] {
            emit performDipoleFit(m_pUi->lineEdit_name->text());
            });

    connect(m_pUi->spinBox_set, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &DipoleFitView::setChanged, Qt::UniqueConnection);

    //Time settings
    connect(m_pUi->spinBox_tmin, QOverload<int>::of(&QSpinBox::valueChanged), [=](int iValue){
                emit timeChanged(iValue,
                                 m_pUi->spinBox_tmax->value(),
                                 m_pUi->spinBox_tstep->value(),
                                 m_pUi->spinBox_tint->value());
            });
    connect(m_pUi->spinBox_tmax, QOverload<int>::of(&QSpinBox::valueChanged), [=](int iValue){
                emit timeChanged(m_pUi->spinBox_tmin->value(),
                                 iValue,
                                 m_pUi->spinBox_tstep->value(),
                                 m_pUi->spinBox_tint->value());
            });
    connect(m_pUi->spinBox_tstep, QOverload<int>::of(&QSpinBox::valueChanged), [=](int iValue){
                emit timeChanged(m_pUi->spinBox_tmin->value(),
                                 m_pUi->spinBox_tmax->value(),
                                 iValue,
                                 m_pUi->spinBox_tint->value());
            });
    connect(m_pUi->spinBox_tint, QOverload<int>::of(&QSpinBox::valueChanged), [=](int iValue){
                emit timeChanged(m_pUi->spinBox_tmin->value(),
                                 m_pUi->spinBox_tmax->value(),
                                 m_pUi->spinBox_tstep->value(),
                                 iValue);
            });

    //Baseline
    connect(m_pUi->spinBox_bmin, QOverload<int>::of(&QSpinBox::valueChanged), [=](int iValue){
                if (iValue != m_pUi->spinBox_bmax->value()){
                    emit baselineChanged(iValue,
                                         m_pUi->spinBox_bmax->value());
                } else {
                    emit baselineChanged(1e6, 1e6);
                }
            });
    connect(m_pUi->spinBox_bmax, QOverload<int>::of(&QSpinBox::valueChanged), [=](int iValue){
                if (iValue != m_pUi->spinBox_bmin->value()){
                    emit baselineChanged(m_pUi->spinBox_bmin->value(),
                                         iValue);
                } else {
                    emit baselineChanged(1e6, 1e6);
                }
            });

    //Modality
    connect(m_pUi->checkBox_EEG, &QCheckBox::toggled, [=](bool bChecked){
                emit modalityChanged(bChecked, m_pUi->checkBox_MEG->isChecked());
            });
    connect(m_pUi->checkBox_MEG, &QCheckBox::toggled, [=](bool bChecked){
                emit modalityChanged(m_pUi->checkBox_EEG->isChecked(), bChecked);
            });

    //Fittings
    connect(m_pUi->doubleSpinBox_dist, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [=](double dValue){
                emit fittingChanged(dValue,
                                    m_pUi->doubleSpinBox_grid->value());
            });
    connect(m_pUi->doubleSpinBox_grid, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [=](double dValue){
                emit fittingChanged(m_pUi->doubleSpinBox_dist->value(),
                                    dValue);
            });

    //Noise
    connect(m_pUi->doubleSpinBox_gradnoise, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [=](double dValue){
                emit noiseChanged(dValue,
                                  m_pUi->doubleSpinBox_magnoise->value(),
                                  m_pUi->doubleSpinBox_eegnoise->value());
            });
    connect(m_pUi->doubleSpinBox_magnoise, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [=](double dValue){
                emit noiseChanged(m_pUi->doubleSpinBox_gradnoise->value(),
                                  dValue,
                                  m_pUi->doubleSpinBox_eegnoise->value());
            });
    connect(m_pUi->doubleSpinBox_eegnoise, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [=](double dValue){
                emit noiseChanged(m_pUi->doubleSpinBox_gradnoise->value(),
                                  m_pUi->doubleSpinBox_magnoise->value(),
                                  dValue);
            });

    //Reg
    connect(m_pUi->doubleSpinBox_gradreg, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [=](double dValue){
                emit regChanged(dValue,
                                m_pUi->doubleSpinBox_magreg->value(),
                                m_pUi->doubleSpinBox_eegreg->value());
            });
    connect(m_pUi->doubleSpinBox_magreg, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [=](double dValue){
                emit regChanged(m_pUi->doubleSpinBox_gradreg->value(),
                                dValue,
                                m_pUi->doubleSpinBox_eegreg->value());
            });
    connect(m_pUi->doubleSpinBox_eegreg, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [=](double dValue){
                emit regChanged(m_pUi->doubleSpinBox_gradreg->value(),
                                m_pUi->doubleSpinBox_magreg->value(),
                                dValue);
            });

    //Sphere model
    connect(m_pUi->doubleSpinBox_orgx, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [=](double dValue){
                emit sphereChanged(dValue,
                                   m_pUi->doubleSpinBox_orgy->value(),
                                   m_pUi->doubleSpinBox_orgz->value(),
                                   m_pUi->doubleSpinBox_rad->value());
            });
    connect(m_pUi->doubleSpinBox_orgy, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [=](double dValue){
                emit sphereChanged(m_pUi->doubleSpinBox_orgx->value(),
                                   dValue,
                                   m_pUi->doubleSpinBox_orgz->value(),
                                   m_pUi->doubleSpinBox_rad->value());
            });
    connect(m_pUi->doubleSpinBox_orgz, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [=](double dValue){
                emit sphereChanged(m_pUi->doubleSpinBox_orgx->value(),
                                   m_pUi->doubleSpinBox_orgy->value(),
                                   dValue,
                                   m_pUi->doubleSpinBox_rad->value());
            });
    connect(m_pUi->doubleSpinBox_rad, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [=](double dValue){
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
    connect(m_pUi->comboBox_meas, &QComboBox::currentTextChanged, [=](const QString& sFileName){
            QString sName = sFileName.split(".",Qt::SkipEmptyParts).at(0);
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
