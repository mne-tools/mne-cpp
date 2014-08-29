//=============================================================================================================
/**
* @file     enhancededitorwindow.h
* @author   Martin Henfling <martin.henfling@tu-ilmenau.de>;
*           Daniel Knobl <daniel.knobl@tu-ilmenau.de>;
*           Sebastian Krause <sebastian.krause@tu-ilmenau.de>
* @version  1.0
* @date     July, 2014
*
* @section  LICENSE
*
* Copyright (C) 2014, Martin Henfling, Daniel Knobl and Sebastian Krause. All rights reserved.
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
* @brief    EnhancedEditorWindow class declaration which enables the adaption of parameters for stored
*           formulas.
*
*/
#ifndef ENHANCEDEDITORWINDOW_H
#define ENHANCEDEDITORWINDOW_H
//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "formulaeditor.h"

//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QWidget>

//=============================================================================================================
// USED NAMESPACES

namespace Ui
{

//=============================================================================================================

    class Enhancededitorwindow;
}

class Enhancededitorwindow : public QWidget
{
    Q_OBJECT
    
public:    
    explicit Enhancededitorwindow(QWidget *parent = 0);
    ~Enhancededitorwindow();
    
private slots:    
    void on_chb_allCombined_toggled(bool checked);    
    void on_cb_AtomFormula_currentIndexChanged(const QString &arg1);
    void on_sb_Atomcount_valueChanged(int arg1);
    void on_btt_DeleteFormula_clicked();
    void on_dsb_StepVauleA_editingFinished();
    void on_dsb_StepVauleB_editingFinished();
    void on_dsb_StepVauleC_editingFinished();
    void on_dsb_StepVauleD_editingFinished();
    void on_dsb_StepVauleE_editingFinished();
    void on_dsb_StepVauleF_editingFinished();
    void on_dsb_StepVauleG_editingFinished();
    void on_dsb_StepVauleH_editingFinished();
    void on_dsb_StartValueA_editingFinished();
    void on_dsb_StartValueB_editingFinished();
    void on_dsb_StartValueC_editingFinished();
    void on_dsb_StartValueD_editingFinished();
    void on_dsb_StartValueE_editingFinished();
    void on_dsb_StartValueF_editingFinished();
    void on_dsb_StartValueG_editingFinished();
    void on_dsb_StartValueH_editingFinished();
    void on_sb_SampleCount_editingFinished();
    void on_dsb_StepVauleA_valueChanged(double arg1);
    void on_dsb_StepVauleB_valueChanged(double arg1);
    void on_dsb_StepVauleC_valueChanged(double arg1);
    void on_dsb_StepVauleD_valueChanged(double arg1);
    void on_dsb_StepVauleE_valueChanged(double arg1);
    void on_dsb_StepVauleF_valueChanged(double arg1);
    void on_dsb_StepVauleG_valueChanged(double arg1);
    void on_dsb_StepVauleH_valueChanged(double arg1);
    void on_dsb_StartValueA_valueChanged(double arg1);
    void on_dsb_StartValueB_valueChanged(double arg1);
    void on_dsb_StartValueC_valueChanged(double arg1);
    void on_dsb_StartValueD_valueChanged(double arg1);
    void on_dsb_StartValueE_valueChanged(double arg1);
    void on_dsb_StartValueF_valueChanged(double arg1);
    void on_dsb_StartValueG_valueChanged(double arg1);
    void on_dsb_StartValueH_valueChanged(double arg1);
    void on_pushButton_clicked();

private:
    Ui::Enhancededitorwindow *ui;
    QStringList m_strStandardFunction;
    QList<qreal> calc_value_list(qreal startValue, qreal linStepValue);
    qreal calc_end_value(qreal startValue, qreal linStepValue);
};

#endif // ENHANCEDEDITORWINDOW_H
