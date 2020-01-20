//=============================================================================================================
/**
 * @file     formulaeditor.h
 * @author   Daniel Knobl <Daniel.Knobl@tu-ilmenau.de>
 * @version  dev
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
 * @brief    FormulaEditor class declaration which allows the definition of individual atomformulas for the
 *           usage in FixDictMp-Algorithm.
 */

#ifndef FORMULAEDITOR_H
#define FORMULAEDITOR_H
//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================



//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QWidget>
#include <QtGui>

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================



//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

#define ANZFUNKTKONST 10

class AtomPaintWindow;

namespace Ui
{
    class Formulaeditor;
}

class Formulaeditor : public QWidget
{

    Q_OBJECT
    
public:

    //*********************************************************************************************************
    //constructor
    explicit Formulaeditor(QWidget *parent = 0);
    ~Formulaeditor();
    //*********************************************************************************************************

    //========================================================================================================
    /**
    * FormlaEditor_get_formula
    *
    * ### MP toolbox FormulaEditor function ###
    *
    * gets formula
    *
    * @return QString   formula
    */
    QString get_formula();

    //========================================================================================================
    /**
    * FormlaEditor_strip_formula
    *
    * ### MP toolbox FormulaEditor function ###
    *
    * adapts formula for calculation
    *
    * @param QString    strformula
    *
    * @return void
    */
    void strip_formula(QString& strFormula);

    //========================================================================================================
    /**
    * FormlaEditor_set_formula
    *
    * ### MP toolbox FormulaEditor function ###
    *
    * sets formula
    *
    * @param QString      Formula
    *
    * @return void
    */
    void set_formula(QString Formula);

    //========================================================================================================
    /**
    * FormlaEditor_set_funct_const
    *
    * ### MP toolbox FormulaEditor function ###
    *
    * sets formula
    *
    * @param    int index       index of paramters
    * @param    double val      value of paramters
    *
    * @return void
    */
    void set_funct_const(int index, double val);

    //========================================================================================================
    /**
    * FormlaEditor_calculation
    *
    * ### MP toolbox FormulaEditor function ###
    *
    * calculation of function values
    *
    * @param    QString strFormula      formula
    * @param    qreal xValue            value of paramters
    * @param    bool strip              read in correct formula (true)
    *
    * @return   double calculation      function value
    */
    double calculation(QString strFormula, qreal xValue, bool strip  =true);
    //========================================================================================================
    
private slots:
    void on_tb_A_textChanged(const QString &arg1);
    void on_tb_B_textChanged(const QString &arg1);
    void on_tb_C_textChanged(const QString &arg1);
    void on_tb_D_textChanged(const QString &arg1);
    void on_tb_E_textChanged(const QString &arg1);
    void on_tb_F_textChanged(const QString &arg1);
    void on_tb_G_textChanged(const QString &arg1);
    void on_tb_H_textChanged(const QString &arg1);
    void on_tb_Formula_textChanged(const QString &arg1);
    void on_btt_Test_clicked();
    void on_btt_Save_clicked();
    void on_dsb_StartValue_editingFinished();
    void on_dsb_StepWidth_editingFinished();

signals:
    void formula_saved();

private:    
    Ui::Formulaeditor *ui;
    AtomPaintWindow *callAtomPaintWindow;

    //========================================================================================================
    // formula methods    Copyright: 2004, Ralf Wirtz
    QString m_strFormula;
    QString m_strFunction;
    QString m_strErrortext;
    QStringList m_strStandardFunction;
    static QString g_strF;
    double m_dFktValue;
    double m_dFunctionConstant[ANZFUNKTKONST];    

    qreal sign_factor(qint32 &nPosition, QString& strCharacter);
    double expression(int& nPosition, QString& strCharacter);
    double simple_expression(int& nPosition, QString& strCharacter);
    double term(int& nPosition, QString& strCharacter);
    double factor(qint32 &nPosition, QString& strCharacter);
    double char_n(int& nPosition, QString& strCharacter);
    QString str_char(QString DecimalZahl);

    QString get_next_token(QString& strSrc, const QString strDelim);
    double SINQ(double Winkel_grad);
    double COSQ(double Winkel_grad);
    double DEG(double x /* rad */) ;
    double RAD(double x /* grad */);
    double cot(double x);
    long double signl(long double x);
    double ArSinh(double x);
    double ArCosh(double x);
    double ArTanh(double x);
    double ArCoth(double x);
    double sqr(double x);
    void closeEvent(QCloseEvent * event);
    // end formula methods    Copyright: 2004, Ralf Wirtz
    //========================================================================================================

};

class AtomPaintWindow : public QWidget
{
    Q_OBJECT

protected:    
   void paintEvent(QPaintEvent *event);

public:   
   void paint_signal(QList<qreal> valueList, QSize windowSize);

};

#endif // FORMULAEDITOR_H

