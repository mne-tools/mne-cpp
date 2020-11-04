//=============================================================================================================
/**
 * @file     dipolefitview.cpp
 * @author   Gabriel Motta <gbmotta@mgh.harvard.edu>
 * @since    0.1.5
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

    connect(m_pUi->pushButton_fit, &QPushButton::clicked,
            this, &DipoleFitView::performDipoleFit);
    connect(m_pUi->spinBox_tmin, QOverload<int>::of(&QSpinBox::valueChanged), [=](int iValue){
                emit timeChanged(iValue,
                                 m_pUi->spinBox_tmax->value(),
                                 m_pUi->spinBox_tstep->value());
            });
    connect(m_pUi->spinBox_tmax, QOverload<int>::of(&QSpinBox::valueChanged), [=](int iValue){
                emit timeChanged(m_pUi->spinBox_tmin->value(),
                                 iValue,
                                 m_pUi->spinBox_tstep->value());
            });
    connect(m_pUi->spinBox_tstep, QOverload<int>::of(&QSpinBox::valueChanged), [=](int iValue){
                emit timeChanged(m_pUi->spinBox_tmin->value(),
                                 m_pUi->spinBox_tmax->value(),
                                 iValue);
            });
    connect(m_pUi->radioButton_EEG, &QRadioButton::toggled, [=](bool bChecked){
                emit modalityChanged(bChecked ? 1 : 0);
            });
    connect(m_pUi->doubleSpinBox_dist,  QOverload<double>::of(&QDoubleSpinBox::valueChanged), [=](double dValue){
                emit fittingChanged(dValue,
                                    m_pUi->doubleSpinBox_grid->value());
            });
    connect(m_pUi->doubleSpinBox_grid, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [=](double dValue){
                emit fittingChanged(m_pUi->doubleSpinBox_dist->value(),
                                    dValue);
            });
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

void DipoleFitView::setBEM(const QString &sFileName)
{

}

//=============================================================================================================

void DipoleFitView::setMRI(const QString &sFileName)
{

}

//=============================================================================================================

void DipoleFitView::setNoise(const QString &sFileName)
{

}

//=============================================================================================================

void DipoleFitView::requestParams()
{

}
