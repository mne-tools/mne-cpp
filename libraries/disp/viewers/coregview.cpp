//=============================================================================================================
/**
 * @file     coregview.cpp
 * @author   Ruben Dörfel <doerfelruben@aol.com>
 * @since    0.1.5
 * @date     August, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020, Ruben Dörfel. All rights reserved.
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
 * @brief    CoregView class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "coregview.h"
#include "ui_coregview.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

CoregView::CoregView(const QString& sSettingsPath,
                     QWidget *parent,
                     Qt::WindowFlags f)
: AbstractView(parent, f)
, m_pUi(new Ui::CoregViewWidget)
{
    m_sSettingsPath = sSettingsPath;
    m_pUi->setupUi(this);

    // Connect Gui elemnts
    QGroupBox *m_qGroupBox_MriSubject;
    QPushButton *m_qPushButton_BemFileDialog;
    QLineEdit *m_qLineEdit_BemFileName;
    QPushButton *m_qPushButton_FidFileDialog;
    QLineEdit *m_qLineEdit_FidFileName;
    QPushButton *m_qPushButton_PickLPA;
    QPushButton *m_qPushButton_PickNas;
    QPushButton *m_qPushButton_PickRPA;
    QPushButton *m_qPushButton_FidStoreFileDialog;
    QLineEdit *m_qLineEdit_FidStoreFileName;
    QPushButton *m_qPushButton_DigFileDialog;
    QLineEdit *m_qLineEdit_DigFileName;
    QSpinBox *m_qSpinBox_Discard;
    QPushButton *m_qPushButton_Omit;
    QLabel *m_qLabel_NOmitted;
    QCheckBox *checkBox;
    QWidget *m_qWidget_IcpIterations;
    QSpinBox *spinBox;
    QDoubleSpinBox *m_qDoubleSpinBox_Converge;
    QLineEdit *m_qLineEdit_LpaWeight;
    QLineEdit *m_qLineEdit_NasWeight;
    QLineEdit *m_qLineEdit_RpaWeight;
    QPushButton *m_qPushButton_FitFiducials;
    QPushButton *m_qPushButton_FitICP;
    QSpinBox *m_qSpinBox_X;
    QSpinBox *m_qSpinBox_Y;
    QSpinBox *m_qSpinBox_Z;
    QComboBox *m_qComboBox_ScalingMode;
    QPushButton *m_qPushButton_ApplyScaling;
    QLineEdit *m_qLineEdit_TransX;
    QLineEdit *m_qLineEdit_RotX;
    QLineEdit *m_qLineEdit_TransY;
    QLineEdit *m_qLineEdit_RotY;
    QLineEdit *m_qLineEdit_TransZ;
    QLineEdit *m_qLineEdit_RotZ;
    QLineEdit *m_qLineEdit_TransFileStore;
    QPushButton *m_qPushButton_TransFileStoreDialaog;

}

//=============================================================================================================

CoregView::~CoregView()
{
    delete m_pUi;
}
