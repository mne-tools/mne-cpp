//=============================================================================================================
/**
 * @file     invmnecontrol.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @version  dev
 * @date     February, 2017
 *
 * @section  LICENSE
 *
 * Copyright (C) 2017, Christoph Dinh. All rights reserved.
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
 * @brief    Contains the declaration of the InvMNEControl class.
 *
 */

#ifndef INVMNECONTROL_H
#define INVMNECONTROL_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <QWidget>


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace Ui {
    class InvMNEControl;
}


//=============================================================================================================
/**
 * InvMNEControl Extension Control
 *
 * @brief The InvMNEControl class provides the extension control.
 */
class InvMNEControl : public QWidget
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
    * Constructs the MNE Control.
    *
    * @param[in] parent     If parent is not NULL the QWidget becomes a child of QWidget inside parent.
    */
    explicit InvMNEControl(QWidget *parent = 0);

    //=========================================================================================================
    /**
    * Destroys the MNE Control.
    */
    ~InvMNEControl();

signals:
    //=========================================================================================================
    /**
    * Emmitted when the calculation button is pressed.
    */
    void calculate_signal();

private slots:
    //=========================================================================================================
    /**
    * Called when fiff evoked path editing finished
    */
    void on_m_qLineEditEvoked_editingFinished();

    //=========================================================================================================
    /**
    * Called when event selection changed
    *
    * @param [in] arg1      the selected event
    */
    void on_m_qComboBoxEventType_currentIndexChanged(const QString &arg1);

    //=========================================================================================================
    /**
    * Called when inverse operator path editing finished
    */
    void on_m_qLineEditInv_editingFinished();

    //=========================================================================================================
    /**
    * Called when double spin box editing finished
    *
    * @param [in] arg1      the selected value
    */
    void on_doubleSpinBox_valueChanged(double arg1);

    //=========================================================================================================
    /**
    * Called when method spin box editing finished
    *
    * @param [in] arg1      the selected method
    */
    void on_m_qComboBoxMethod_currentIndexChanged(const QString &arg1);

    //=========================================================================================================
    /**
    * Called when STC path editing finished
    */
    void on_m_qLineEditSTC_editingFinished();

    //=========================================================================================================
    /**
    * Called when calculating button was pressed.
    */
    void on_m_qPushButtonCalculate_released();

private:
    Ui::InvMNEControl *ui;  /**< The user interface */
};

#endif // INVMNECONTROL_H
