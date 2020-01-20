//=============================================================================================================
/**
 * @file     enhancededitorwindow.h
 * @author   Daniel Knobl <Daniel.Knobl@tu-ilmenau.de>
 * @version  1.0
 * @date     July, 2014
 *
 * @section  LICENSE
 *
 * Copyright (C) 2014, Daniel Knobl. All rights reserved.
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
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/SparseCore>
#include <unsupported/Eigen/FFT>

//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QWidget>

//=============================================================================================================
// USED NAMESPACES


using namespace Eigen;
namespace Ui
{
    class Enhancededitorwindow;
}

class Enhancededitorwindow : public QWidget
{
    Q_OBJECT
    
public:    
    //*********************************************************************************************************
    //constructor
    explicit Enhancededitorwindow(QWidget *parent = 0);
    //*********************************************************************************************************

    ~Enhancededitorwindow();
    
private slots:    
    void on_chb_allCombined_toggled(bool checked);    
    void on_cb_AtomFormula_currentIndexChanged(const QString &arg1);
    void on_sb_Atomcount_valueChanged(int arg1);
    void on_btt_DeleteFormula_clicked(); 
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
    void on_dsb_EndValueA_valueChanged(double arg1);
    void on_dsb_EndValueB_valueChanged(double arg1);
    void on_dsb_EndValueC_valueChanged(double arg1);
    void on_dsb_EndValueD_valueChanged(double arg1);
    void on_dsb_EndValueE_valueChanged(double arg1);
    void on_dsb_EndValueF_valueChanged(double arg1);
    void on_dsb_EndValueG_valueChanged(double arg1);
    void on_dsb_EndValueH_valueChanged(double arg1);
    void on_pushButton_clicked();
    void on_formula_saved();

signals:
    void dict_saved();

private:

    Ui::Enhancededitorwindow *ui;
    QStringList m_strStandardFunction;

    //=======================================================================================================
    /**
    * EnhancedEditorWindow_close_event
    *
    * ### MP toolbox EnhancedEditorWindow function ###
    *
    * close event
    *
    * @param
    *
    * @return void
    */
    void closeEvent(QCloseEvent * event);

    //=======================================================================================================
    /**
    * EnhancedEditorWindow_read_formula
    *
    * ### MP toolbox EnhancedEditorWindow function ###
    *
    * read in formula
    *
    * @return void
    */
    void read_formula();

    //=======================================================================================================
    /**
    * EnhancedEditorWindow_calc_atom_count_all_combined
    *
    * ### MP toolbox EnhancedEditorWindow function ###
    *
    * calculates atom count when all combined is selected
    *
    * @return void
    */
    void calc_atom_count_all_combined();

    //=======================================================================================================
    /**
    * EnhancedEditorWindow_calc_value_list
    *
    * ### MP toolbox EnhancedEditorWindow function ###
    *
    * calculates value list of parameter(s)
    *
    * @return   QList<real> value list of parameter(s)
    */
    QList<qreal> calc_value_list(qreal start_value, qreal line_step_value, qreal end_value);

    //=======================================================================================================
    /**
    * EnhancedEditorWindow_calc_end_value
    *
    * ### MP toolbox EnhancedEditorWindow function ###
    *
    * calculates end value of parameter
    *
    * @param    startValue      start value of paramter
    * @param    linStepValue    step width
    *
    * @return   qreal   end value of parameter
    */
    qreal calc_end_value(qreal startValue, qreal linStepValue);
    //=======================================================================================================
};

#endif // ENHANCEDEDITORWINDOW_H
