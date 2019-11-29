//=============================================================================================================
/**
* @file     enhancededitorwindow.cpp
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
* @brief    Definition of EnhancedEditorWindow class.
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "enhancededitorwindow.h"
#include "ui_enhancededitorwindow.h"

#include "deletemessagebox.h"
#include "ui_deletemessagebox.h"

#include "formulaeditor.h"

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include "QtGui"
#include <QWidget>
#include <QMessageBox>

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================



//*************************************************************************************************************
//=============================================================================================================
// MAIN
//=============================================================================================================

qint32 _atom_count = 1;
qint32 _sample_count = 64;
QList<qreal> value_a_list;
QList<qreal> value_b_list;
QList<qreal> value_c_list;
QList<qreal> value_d_list;
QList<qreal> value_e_list;
QList<qreal> value_f_list;
QList<qreal> value_g_list;
QList<qreal> value_h_list;
QStringList _names_list;

//***********************************************************************************************************************

//constructor
Enhancededitorwindow::Enhancededitorwindow(QWidget *parent): QWidget(parent), ui(new Ui::Enhancededitorwindow)
{
    this->setAccessibleName("formel");
    ui->setupUi(this);

    QSettings settings;
    move(settings.value("pos_enhanced_editor", QPoint(200, 200)).toPoint());

    m_strStandardFunction.append("");
    m_strStandardFunction.append("PI");
    m_strStandardFunction.append("ABS");
    m_strStandardFunction.append("SQRT");
    m_strStandardFunction.append("SINH");
    m_strStandardFunction.append("COSH");
    m_strStandardFunction.append("TANH");
    m_strStandardFunction.append("ARCTAN");
    m_strStandardFunction.append("LN");
    m_strStandardFunction.append("LOG");
    m_strStandardFunction.append("EXP");
    m_strStandardFunction.append("SIN"); //must be addended behind SINH!!!
    m_strStandardFunction.append("COS");
    m_strStandardFunction.append("TAN");
    m_strStandardFunction.append("COT");
    m_strStandardFunction.append("ARCSIN");
    m_strStandardFunction.append("ARCCOS");
    m_strStandardFunction.append("INT");
    m_strStandardFunction.append("RAD");
    m_strStandardFunction.append("DEG");
    m_strStandardFunction.append("SIGN");
    m_strStandardFunction.append("ARSINH");
    m_strStandardFunction.append("ARCOSH");
    m_strStandardFunction.append("ARTANH");
    m_strStandardFunction.append("KGV");
    m_strStandardFunction.append("GGT");

    read_formula();
}

//***********************************************************************************************************************

Enhancededitorwindow::~Enhancededitorwindow()
{
    delete ui;
}

//*************************************************************************************************************************************

void Enhancededitorwindow::closeEvent(QCloseEvent * event)
{
    Q_UNUSED(event);
    QSettings settings;
    if(!this->isMaximized())
    {
        settings.setValue("pos_enhanced_editor", pos());
    }
}

//*************************************************************************************************************************************


void Enhancededitorwindow::on_formula_saved()
{
    read_formula();
}

//*************************************************************************************************************************************


void Enhancededitorwindow::read_formula()
{
    _names_list.clear();
    ui->cb_AtomFormula->clear();

    QString contents;
    QFile formula_file(QDir::homePath() + "/" + "Matching-Pursuit-Toolbox/user.fml");

    if (formula_file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        while(!formula_file.atEnd())
        {
            contents = formula_file.readLine(0).constData();
            ui->cb_AtomFormula->addItem(QIcon(":/images/icons/formel.png"), contents.trimmed());
        }
    }
    formula_file.close();

    QDir dictDir = QDir(QDir::homePath() + "/" + "Matching-Pursuit-Toolbox");

    QStringList filterList;
    filterList.append("*.dict");
    filterList.append("*.pdict");

    QFileInfoList fileList = dictDir.entryInfoList(filterList);
    for(int i = 0; i < fileList.length(); i++)
        _names_list.append(fileList.at(i).baseName());

    if(ui->cb_AtomFormula->count() != 0)
        ui->cb_AtomFormula->setCurrentIndex(0);
}

//***********************************************************************************************************************

// when "all combined" toogelt
void Enhancededitorwindow::on_chb_allCombined_toggled(bool checked)
{
    if(checked)
    {
        ui->sb_Atomcount->setEnabled(false);

        ui->dsb_StepVauleA->setEnabled(true);
        ui->dsb_StepVauleB->setEnabled(true);
        ui->dsb_StepVauleC->setEnabled(true);
        ui->dsb_StepVauleD->setEnabled(true);
        ui->dsb_StepVauleE->setEnabled(true);
        ui->dsb_StepVauleF->setEnabled(true);
        ui->dsb_StepVauleG->setEnabled(true);
        ui->dsb_StepVauleH->setEnabled(true);

        ui->dsb_EndValueA->setEnabled(true);
        ui->dsb_EndValueB->setEnabled(true);
        ui->dsb_EndValueC->setEnabled(true);
        ui->dsb_EndValueD->setEnabled(true);
        ui->dsb_EndValueE->setEnabled(true);
        ui->dsb_EndValueF->setEnabled(true);
        ui->dsb_EndValueG->setEnabled(true);
        ui->dsb_EndValueH->setEnabled(true);

        calc_atom_count_all_combined();
    }
    else
    {
        ui->sb_Atomcount->setEnabled(true);
        ui->dsb_EndValueA->setEnabled(false);
        ui->dsb_EndValueB->setEnabled(false);
        ui->dsb_EndValueC->setEnabled(false);
        ui->dsb_EndValueD->setEnabled(false);
        ui->dsb_EndValueE->setEnabled(false);
        ui->dsb_EndValueF->setEnabled(false);
        ui->dsb_EndValueG->setEnabled(false);
        ui->dsb_EndValueH->setEnabled(false);
    }
}

//***********************************************************************************************************************

// when number of atoms is changing
void Enhancededitorwindow::on_sb_Atomcount_valueChanged(int arg1)
{
    if(arg1 <= 1)
    {
        ui->dsb_StepVauleA->setEnabled(false);
        ui->dsb_StepVauleB->setEnabled(false);
        ui->dsb_StepVauleC->setEnabled(false);
        ui->dsb_StepVauleD->setEnabled(false);
        ui->dsb_StepVauleE->setEnabled(false);
        ui->dsb_StepVauleF->setEnabled(false);
        ui->dsb_StepVauleG->setEnabled(false);
        ui->dsb_StepVauleH->setEnabled(false);
    }
    else
    {
        ui->dsb_StepVauleA->setEnabled(true);
        ui->dsb_StepVauleB->setEnabled(true);
        ui->dsb_StepVauleC->setEnabled(true);
        ui->dsb_StepVauleD->setEnabled(true);
        ui->dsb_StepVauleE->setEnabled(true);
        ui->dsb_StepVauleF->setEnabled(true);
        ui->dsb_StepVauleG->setEnabled(true);
        ui->dsb_StepVauleH->setEnabled(true);
    }

    if(!ui->chb_allCombined->isChecked())
    {
        if(ui->lb_A->isVisible()) ui->dsb_EndValueA->setValue(calc_end_value(ui->dsb_StartValueA->value(), ui->dsb_StepVauleA->value()));
        if(ui->lb_B->isVisible()) ui->dsb_EndValueB->setValue(calc_end_value(ui->dsb_StartValueB->value(), ui->dsb_StepVauleB->value()));
        if(ui->lb_C->isVisible()) ui->dsb_EndValueC->setValue(calc_end_value(ui->dsb_StartValueC->value(), ui->dsb_StepVauleC->value()));
        if(ui->lb_D->isVisible()) ui->dsb_EndValueD->setValue(calc_end_value(ui->dsb_StartValueD->value(), ui->dsb_StepVauleD->value()));
        if(ui->lb_E->isVisible()) ui->dsb_EndValueE->setValue(calc_end_value(ui->dsb_StartValueE->value(), ui->dsb_StepVauleE->value()));
        if(ui->lb_F->isVisible()) ui->dsb_EndValueF->setValue(calc_end_value(ui->dsb_StartValueF->value(), ui->dsb_StepVauleF->value()));
        if(ui->lb_G->isVisible()) ui->dsb_EndValueG->setValue(calc_end_value(ui->dsb_StartValueG->value(), ui->dsb_StepVauleG->value()));
        if(ui->lb_H->isVisible()) ui->dsb_EndValueH->setValue(calc_end_value(ui->dsb_StartValueH->value(), ui->dsb_StepVauleH->value()));
    }
    _atom_count = arg1;
}

//***********************************************************************************************************************

void Enhancededitorwindow::on_sb_SampleCount_editingFinished()
{
    _sample_count = ui->sb_SampleCount->value();
}

//***********************************************************************************************************************

// when formula is changed
void Enhancededitorwindow::on_cb_AtomFormula_currentIndexChanged(const QString &arg1)
{
    QList<QChar> foundChar;
    foundChar.clear();
    for(qint32 i = 0; i < arg1.length(); i++)
    {
        bool beforeFound = false;
        bool nextfound = false;
        QChar upperChar = arg1.at(i).toUpper();
        if((upperChar >= 'A' && upperChar <= 'H') || upperChar == 'X')
        {
            if(i != 0)
            {
                QChar beforeUpperChar = arg1.at(i - 1).toUpper();
                if(beforeUpperChar < 'A' || (beforeUpperChar > 'Z' && beforeUpperChar < 126))   beforeFound = true;
            }
            else    beforeFound = true;

            if(i < arg1.length() - 1)
            {
                QChar nextUpperChar = arg1.at(i+1).toUpper();
                if(nextUpperChar < 'A' || (nextUpperChar > 'Z' && nextUpperChar < 126))     nextfound = true;
            }
            else    nextfound = true;


            if(beforeFound && nextfound)
            {
                if(upperChar == 'A')        foundChar.append(upperChar);
                else if(upperChar == 'B')   foundChar.append(upperChar);
                else if(upperChar == 'C')   foundChar.append(upperChar);
                else if(upperChar == 'D')   foundChar.append(upperChar);
                else if(upperChar == 'E')   foundChar.append(upperChar);
                else if(upperChar == 'F')   foundChar.append(upperChar);
                else if(upperChar == 'G')   foundChar.append(upperChar);
                else if(upperChar == 'H')   foundChar.append(upperChar);
                else if(upperChar == 'X')   foundChar.append(upperChar);
            }
        }
    }

    ui->gb_A->setHidden(true);
    ui->gb_B->setHidden(true);
    ui->gb_C->setHidden(true);
    ui->gb_D->setHidden(true);
    ui->gb_E->setHidden(true);
    ui->gb_F->setHidden(true);
    ui->gb_G->setHidden(true);
    ui->gb_H->setHidden(true);

    for(qint32 j = 0; j < foundChar.length(); j++)
    {
        if(foundChar.at(j) =='A') ui->gb_A->setHidden(false);
        else if(foundChar.at(j) =='B') ui->gb_B->setHidden(false);
        else if(foundChar.at(j) =='C') ui->gb_C->setHidden(false);
        else if(foundChar.at(j) =='D') ui->gb_D->setHidden(false);
        else if(foundChar.at(j) =='E') ui->gb_E->setHidden(false);
        else if(foundChar.at(j) =='F') ui->gb_F->setHidden(false);
        else if(foundChar.at(j) =='G') ui->gb_G->setHidden(false);
        else if(foundChar.at(j) =='H') ui->gb_H->setHidden(false);
    }

    resize(minimumSize());
    setFixedHeight(sizeHint().height()); // no user access
}

//***********************************************************************************************************************

void Enhancededitorwindow::on_btt_DeleteFormula_clicked()
{
    QString contents;
    QSettings settings;
    if(settings.value("show_warnings", true).toBool())
    {
        DeleteMessageBox* msgBox = new DeleteMessageBox(this);
        msgBox->setModal(true);
        qint32 result = msgBox->exec();

        if(result == 0)
        {
            msgBox->close();
            return;
        }
        msgBox->close();
    }

    QFile formel_file(QDir::homePath() + "/" + "Matching-Pursuit-Toolbox/user.fml");
    QFile formel_temp_file(QDir::homePath() + "/" + "Matching-Pursuit-Toolbox/user.temp");

    if(!formel_temp_file.exists())
    {
        if (formel_temp_file.open(QIODevice::ReadWrite | QIODevice::Text))
        formel_temp_file.close();
    }

    QTextStream stream( &formel_temp_file );
    if (formel_file.open(QIODevice::ReadWrite | QIODevice::Text))
    {
        if (formel_temp_file.open (QIODevice::WriteOnly| QIODevice::Append))
        {
            while(!formel_file.atEnd())
            {
                contents = formel_file.readLine(0).constData();
                if(!QString::compare(ui->cb_AtomFormula->currentText() + "\n", contents) == 0)
                    stream << contents;
            }
        }
    }

    formel_file.close();
    formel_temp_file.close();

    formel_file.remove();
    formel_temp_file.rename(QDir::homePath() + "/" + "Matching-Pursuit-Toolbox/user.fml");

    ui->cb_AtomFormula->removeItem(ui->cb_AtomFormula->currentIndex());
}

//***********************************************************************************************************************

// calculates number of atoms if "combine all"
void Enhancededitorwindow::calc_atom_count_all_combined()
{
    qint32 count = 0;
    qint32 count_a = 1;
    qint32 count_b = 1;
    qint32 count_c = 1;
    qint32 count_d = 1;
    qint32 count_e = 1;
    qint32 count_f = 1;
    qint32 count_g = 1;
    qint32 count_h = 1;

    if(value_a_list.length() != 0) count_a = value_a_list.length();
    if(value_b_list.length() != 0) count_b = value_b_list.length();
    if(value_c_list.length() != 0) count_c = value_c_list.length();
    if(value_d_list.length() != 0) count_d = value_d_list.length();
    if(value_e_list.length() != 0) count_e = value_e_list.length();
    if(value_f_list.length() != 0) count_f = value_f_list.length();
    if(value_g_list.length() != 0) count_g = value_g_list.length();
    if(value_h_list.length() != 0) count_h = value_h_list.length();

    count = count_a * count_b * count_c * count_d * count_e * count_f * count_g * count_h;

    if(count > 10000000)
    {
        QMessageBox::warning(this, tr("Error"),
        tr("The number of atoms is too large."));
        //return;
    }
    ui->sb_Atomcount->setValue(count);
}

//***********************************************************************************************************************

QList<qreal> Enhancededitorwindow::calc_value_list(qreal start_value, qreal line_step_value, qreal end_value)
{
    qreal atom_count = _atom_count;
    QList<qreal> resultList;
    if(ui->chb_allCombined->isChecked())
        atom_count = (end_value - start_value + 1) / line_step_value;

    for(qint32 i = 0; i < atom_count; i++)
        resultList.append(start_value + (i * line_step_value));

    return resultList;
}

//***********************************************************************************************************************

qreal Enhancededitorwindow::calc_end_value(qreal startValue, qreal linStepValue)
{
    if(ui->sb_Atomcount->value() == 1)
        return startValue;
    return startValue + (_atom_count * linStepValue);
}

//***********************************************************************************************************************

void Enhancededitorwindow::on_dsb_StepVauleA_valueChanged(double arg1)
{
    value_a_list = calc_value_list(ui->dsb_StartValueA->value(), arg1, ui->dsb_EndValueA->value());
    if(!ui->chb_allCombined->isChecked()) ui->dsb_EndValueA->setValue(calc_end_value(ui->dsb_StartValueA->value(), arg1));
    if(ui->chb_allCombined->isChecked())
    {
        ui->dsb_EndValueA->setMinimum(ui->dsb_StartValueA->value() + arg1);
        calc_atom_count_all_combined();
    }
}

void Enhancededitorwindow::on_dsb_StepVauleB_valueChanged(double arg1)
{
    value_b_list = calc_value_list(ui->dsb_StartValueB->value(), arg1, ui->dsb_EndValueB->value());
    if(!ui->chb_allCombined->isChecked()) ui->dsb_EndValueB->setValue(calc_end_value(ui->dsb_StartValueB->value(), arg1));
    if(ui->chb_allCombined->isChecked())
    {
        ui->dsb_EndValueB->setMinimum(ui->dsb_StartValueB->value() + arg1);
        calc_atom_count_all_combined();
    }
}

void Enhancededitorwindow::on_dsb_StepVauleC_valueChanged(double arg1)
{
    value_c_list = calc_value_list(ui->dsb_StartValueC->value(), arg1, ui->dsb_EndValueC->value());
    if(!ui->chb_allCombined->isChecked()) ui->dsb_EndValueC->setValue(calc_end_value(ui->dsb_StartValueC->value(), arg1));
    if(ui->chb_allCombined->isChecked())
    {
        ui->dsb_EndValueC->setMinimum(ui->dsb_StartValueC->value() + arg1);
        calc_atom_count_all_combined();
    }
}

void Enhancededitorwindow::on_dsb_StepVauleD_valueChanged(double arg1)
{
    value_d_list = calc_value_list(ui->dsb_StartValueD->value(), arg1, ui->dsb_EndValueD->value());
    if(!ui->chb_allCombined->isChecked()) ui->dsb_EndValueD->setValue(calc_end_value(ui->dsb_StartValueD->value(), arg1));
    if(ui->chb_allCombined->isChecked())
    {
        ui->dsb_EndValueD->setMinimum(ui->dsb_StartValueD->value() + arg1);
        calc_atom_count_all_combined();
    }
}

void Enhancededitorwindow::on_dsb_StepVauleE_valueChanged(double arg1)
{
    value_e_list = calc_value_list(ui->dsb_StartValueE->value(), arg1, ui->dsb_EndValueE->value());
    if(!ui->chb_allCombined->isChecked()) ui->dsb_EndValueE->setValue(calc_end_value(ui->dsb_StartValueE->value(), arg1));
    if(ui->chb_allCombined->isChecked())
    {
        ui->dsb_EndValueE->setMinimum(ui->dsb_StartValueE->value() + arg1);
        calc_atom_count_all_combined();
    }
}

void Enhancededitorwindow::on_dsb_StepVauleF_valueChanged(double arg1)
{
    value_f_list = calc_value_list(ui->dsb_StartValueF->value(), arg1, ui->dsb_EndValueF->value());
    if(!ui->chb_allCombined->isChecked()) ui->dsb_EndValueF->setValue(calc_end_value(ui->dsb_StartValueF->value(), arg1));
    if(ui->chb_allCombined->isChecked())
    {
        ui->dsb_EndValueF->setMinimum(ui->dsb_StartValueF->value() + arg1);
        calc_atom_count_all_combined();
    }
}

void Enhancededitorwindow::on_dsb_StepVauleG_valueChanged(double arg1)
{
    value_g_list = calc_value_list(ui->dsb_StartValueG->value(), arg1, ui->dsb_EndValueG->value());
    if(!ui->chb_allCombined->isChecked()) ui->dsb_EndValueG->setValue(calc_end_value(ui->dsb_StartValueG->value(), arg1));
    if(ui->chb_allCombined->isChecked())
    {
        ui->dsb_EndValueG->setMinimum(ui->dsb_StartValueG->value() + arg1);
        calc_atom_count_all_combined();
    }
}

void Enhancededitorwindow::on_dsb_StepVauleH_valueChanged(double arg1)
{
    value_h_list = calc_value_list(ui->dsb_StartValueH->value(), arg1, ui->dsb_EndValueH->value());
    if(!ui->chb_allCombined->isChecked()) ui->dsb_EndValueH->setValue(calc_end_value(ui->dsb_StartValueH->value(), arg1));
    if(ui->chb_allCombined->isChecked())
    {
        ui->dsb_EndValueH->setMinimum(ui->dsb_StartValueH->value() + arg1);
        calc_atom_count_all_combined();
    }
}

//***********************************************************************************************************************


void Enhancededitorwindow::on_dsb_StartValueA_valueChanged(double arg1)
{
    value_a_list = calc_value_list(arg1, ui->dsb_StepVauleA->value(), ui->dsb_EndValueA->value());
    if(!ui->chb_allCombined->isChecked()) ui->dsb_EndValueA->setValue(calc_end_value(arg1, ui->dsb_StepVauleA->value()));
    if(ui->chb_allCombined->isChecked())
    {
        ui->dsb_EndValueA->setMinimum(arg1 + ui->dsb_StepVauleA->value());
        calc_atom_count_all_combined();
    }
}

void Enhancededitorwindow::on_dsb_StartValueB_valueChanged(double arg1)
{
    value_b_list = calc_value_list(arg1, ui->dsb_StepVauleB->value(), ui->dsb_EndValueB->value());
    if(!ui->chb_allCombined->isChecked()) ui->dsb_EndValueB->setValue(calc_end_value(arg1, ui->dsb_StepVauleB->value()));
    if(ui->chb_allCombined->isChecked())
    {
        ui->dsb_EndValueA->setMinimum(arg1 + ui->dsb_StepVauleB->value());
        calc_atom_count_all_combined();
    }
}

void Enhancededitorwindow::on_dsb_StartValueC_valueChanged(double arg1)
{
    value_c_list = calc_value_list(arg1, ui->dsb_StepVauleC->value(), ui->dsb_EndValueC->value());
    if(!ui->chb_allCombined->isChecked()) ui->dsb_EndValueC->setValue(calc_end_value(arg1, ui->dsb_StepVauleC->value()));
    if(ui->chb_allCombined->isChecked())
    {
        ui->dsb_EndValueA->setMinimum(arg1 + ui->dsb_StepVauleC->value());
        calc_atom_count_all_combined();
    }
}

void Enhancededitorwindow::on_dsb_StartValueD_valueChanged(double arg1)
{
    value_d_list = calc_value_list(arg1, ui->dsb_StepVauleD->value(), ui->dsb_EndValueD->value());
    if(!ui->chb_allCombined->isChecked()) ui->dsb_EndValueD->setValue(calc_end_value(arg1, ui->dsb_StepVauleD->value()));
    if(ui->chb_allCombined->isChecked())
    {
        ui->dsb_EndValueA->setMinimum(arg1 + ui->dsb_StepVauleD->value());
        calc_atom_count_all_combined();
    }
}

void Enhancededitorwindow::on_dsb_StartValueE_valueChanged(double arg1)
{
    value_e_list = calc_value_list(arg1, ui->dsb_StepVauleE->value(), ui->dsb_EndValueE->value());
    if(!ui->chb_allCombined->isChecked()) ui->dsb_EndValueE->setValue(calc_end_value(arg1, ui->dsb_StepVauleE->value()));
    if(ui->chb_allCombined->isChecked())
    {
        ui->dsb_EndValueA->setMinimum(arg1 + ui->dsb_StepVauleE->value());
        calc_atom_count_all_combined();
    }
}

void Enhancededitorwindow::on_dsb_StartValueF_valueChanged(double arg1)
{
    value_f_list = calc_value_list(arg1, ui->dsb_StepVauleF->value(), ui->dsb_EndValueF->value());
    if(!ui->chb_allCombined->isChecked()) ui->dsb_EndValueF->setValue(calc_end_value(arg1, ui->dsb_StepVauleF->value()));
    if(ui->chb_allCombined->isChecked())
    {
        ui->dsb_EndValueA->setMinimum(arg1 + ui->dsb_StepVauleF->value());
        calc_atom_count_all_combined();
    }
}

void Enhancededitorwindow::on_dsb_StartValueG_valueChanged(double arg1)
{
    value_g_list = calc_value_list(arg1, ui->dsb_StepVauleG->value(), ui->dsb_EndValueG->value());
    if(!ui->chb_allCombined->isChecked()) ui->dsb_EndValueG->setValue(calc_end_value(arg1, ui->dsb_StepVauleG->value()));
    if(ui->chb_allCombined->isChecked())
    {
        ui->dsb_EndValueA->setMinimum(arg1 + ui->dsb_StepVauleG->value());
        calc_atom_count_all_combined();
    }
}

void Enhancededitorwindow::on_dsb_StartValueH_valueChanged(double arg1)
{
    value_h_list = calc_value_list(arg1, ui->dsb_StepVauleH->value(), ui->dsb_EndValueH->value());
    if(!ui->chb_allCombined->isChecked()) ui->dsb_EndValueH->setValue(calc_end_value(arg1, ui->dsb_StepVauleH->value()));
    if(ui->chb_allCombined->isChecked())
    {
        ui->dsb_EndValueA->setMinimum(arg1 + ui->dsb_StepVauleH->value());
        calc_atom_count_all_combined();
    }
}

//***********************************************************************************************************************

void Enhancededitorwindow::on_dsb_EndValueA_valueChanged(double arg1)
{
    value_a_list = calc_value_list(ui->dsb_StartValueA->value(), ui->dsb_StepVauleA->value(), arg1);
    if(ui->chb_allCombined->isChecked()) calc_atom_count_all_combined();
}

void Enhancededitorwindow::on_dsb_EndValueB_valueChanged(double arg1)
{
    value_b_list = calc_value_list(ui->dsb_StartValueB->value(), ui->dsb_StepVauleB->value(), arg1);
    if(ui->chb_allCombined->isChecked()) calc_atom_count_all_combined();
}

void Enhancededitorwindow::on_dsb_EndValueC_valueChanged(double arg1)
{
    value_c_list = calc_value_list(ui->dsb_StartValueC->value(), ui->dsb_StepVauleC->value(), arg1);
    if(ui->chb_allCombined->isChecked()) calc_atom_count_all_combined();
}

void Enhancededitorwindow::on_dsb_EndValueD_valueChanged(double arg1)
{
    value_d_list = calc_value_list(ui->dsb_StartValueD->value(), ui->dsb_StepVauleD->value(), arg1);
    if(ui->chb_allCombined->isChecked()) calc_atom_count_all_combined();
}

void Enhancededitorwindow::on_dsb_EndValueE_valueChanged(double arg1)
{
    value_e_list = calc_value_list(ui->dsb_StartValueE->value(), ui->dsb_StepVauleE->value(), arg1);
    if(ui->chb_allCombined->isChecked()) calc_atom_count_all_combined();
}

void Enhancededitorwindow::on_dsb_EndValueF_valueChanged(double arg1)
{
    value_f_list = calc_value_list(ui->dsb_StartValueF->value(), ui->dsb_StepVauleF->value(), arg1);
    if(ui->chb_allCombined->isChecked()) calc_atom_count_all_combined();
}

void Enhancededitorwindow::on_dsb_EndValueG_valueChanged(double arg1)
{
    value_g_list = calc_value_list(ui->dsb_StartValueG->value(), ui->dsb_StepVauleG->value(), arg1);
    if(ui->chb_allCombined->isChecked()) calc_atom_count_all_combined();
}

void Enhancededitorwindow::on_dsb_EndValueH_valueChanged(double arg1)
{
    value_h_list = calc_value_list(ui->dsb_StartValueH->value(), ui->dsb_StepVauleH->value(), arg1);
    if(ui->chb_allCombined->isChecked()) calc_atom_count_all_combined();
}

//***********************************************************************************************************************

// calc all atoms with choosen parameters and save to list and to drive
void Enhancededitorwindow::on_pushButton_clicked()
{
    QStringList results_list;
    results_list.clear();

    QString savePath = QString(QDir::homePath() + "/" + "Matching-Pursuit-Toolbox/%1.pdict").arg(ui->tb_atom_name->text());
    QFile dict(savePath);

    if(ui->tb_atom_name->text().isEmpty())
    {
        QMessageBox::warning(this, tr("Error"),
        tr("There was no name assigned."));
        ui->tb_atom_name->setFocus();
        return;
    }

    QStringList filterList;
    filterList.append("*.pdict");
    QDir dictDir = QDir(QDir::homePath() + "/" + "Matching-Pursuit-Toolbox");
    QFileInfoList fileList = dictDir.entryInfoList(filterList);

    for(qint32 i = 0; i < fileList.length(); i++)
    {
        QFileInfo fileInfo = fileList.at(i);
        if(QString::compare(fileInfo.baseName(), ui->tb_atom_name->text()) == 0)
        {
                QMessageBox::warning(this, tr("Error"),
                tr("The name for the dictionary is already taken."));
                ui->tb_atom_name->setFocus();
                ui->tb_atom_name->selectAll();
                return;
        }
    }

    QString save_path_xml = QString(QDir::homePath() + "/" + "Matching-Pursuit-Toolbox/%1_xml.pdict").arg(ui->tb_atom_name->text());
    QFile xml_file(save_path_xml);

    if(xml_file.open(QIODevice::WriteOnly))
    {
        QXmlStreamWriter xmlWriter(&xml_file);
        xmlWriter.setAutoFormatting(true);
        xmlWriter.writeStartDocument();

        xmlWriter.writeStartElement("COUNT");
        xmlWriter.writeAttribute("of_atoms", QString::number(ui->sb_Atomcount->value()));
        xmlWriter.writeStartElement("built_Atoms");

        xmlWriter.writeAttribute("formula", ui->cb_AtomFormula->currentText());
        xmlWriter.writeAttribute("sample_count", QString::number(ui->sb_SampleCount->value()));
        xmlWriter.writeAttribute("atom_count", QString::number(ui->sb_Atomcount->value()));
        xmlWriter.writeAttribute("source_dict", ui->tb_atom_name->text());

        if (dict.open (QIODevice::WriteOnly| QIODevice::Append))
        {
            QTextStream stream( &dict );
            stream << QString("atomcount = %1 ").arg(_atom_count) << "\n";
            if(ui->chb_allCombined->isChecked())
            {
                qint32 atomIndex = 0;
                qint32 max_a = 1;
                qint32 max_b = 1;
                qint32 max_c = 1;
                qint32 max_d = 1;
                qint32 max_e = 1;
                qint32 max_f = 1;
                qint32 max_g = 1;
                qint32 max_h = 1;

                if(value_a_list.length() != 0) max_a = value_a_list.length();
                if(value_b_list.length() != 0) max_b = value_b_list.length();
                if(value_c_list.length() != 0) max_c = value_c_list.length();
                if(value_d_list.length() != 0) max_d = value_d_list.length();
                if(value_e_list.length() != 0) max_e = value_e_list.length();
                if(value_f_list.length() != 0) max_f = value_f_list.length();
                if(value_g_list.length() != 0) max_g = value_g_list.length();
                if(value_h_list.length() != 0) max_h = value_h_list.length();

                qint32 count_h = 0;
                while(count_h < max_h)
                {
                    qint32 count_g = 0;
                    while(count_g < max_g)
                    {
                        qint32 count_f = 0;
                        while(count_f < max_f)
                        {
                            qint32 count_e = 0;
                            while(count_e < max_e)
                            {
                                qint32 count_d = 0;
                                while(count_d < max_d)
                                {
                                    qint32 count_c = 0;
                                    while(count_c < max_c)
                                    {
                                        qint32 count_b = 0;
                                        while(count_b < max_b)
                                        {
                                            qint32 count_a = 0;
                                            while(count_a < max_a)
                                            {
                                                qreal temp_a = ui->dsb_StartValueA->value();
                                                qreal temp_b = ui->dsb_StartValueB->value();
                                                qreal temp_c = ui->dsb_StartValueC->value();
                                                qreal temp_d = ui->dsb_StartValueD->value();
                                                qreal temp_e = ui->dsb_StartValueE->value();
                                                qreal temp_f = ui->dsb_StartValueF->value();
                                                qreal temp_g = ui->dsb_StartValueG->value();
                                                qreal temp_h = ui->dsb_StartValueH->value();
                                                if(value_a_list.length() > 0) temp_a = value_a_list.at(count_a);
                                                if(value_b_list.length() > 0) temp_b = value_b_list.at(count_b);
                                                if(value_c_list.length() > 0) temp_c = value_c_list.at(count_c);
                                                if(value_d_list.length() > 0) temp_d = value_d_list.at(count_d);
                                                if(value_e_list.length() > 0) temp_e = value_e_list.at(count_e);
                                                if(value_f_list.length() > 0) temp_f = value_f_list.at(count_f);
                                                if(value_g_list.length() > 0) temp_g = value_g_list.at(count_g);
                                                if(value_h_list.length() > 0) temp_h = value_h_list.at(count_h);


                                                Formulaeditor formula_parser;

                                                formula_parser.set_funct_const(0, temp_a);
                                                formula_parser.set_funct_const(1, temp_b);
                                                formula_parser.set_funct_const(2, temp_c);
                                                formula_parser.set_funct_const(3, temp_d);
                                                formula_parser.set_funct_const(4, temp_e);
                                                formula_parser.set_funct_const(5, temp_f);
                                                formula_parser.set_funct_const(6, temp_g);
                                                formula_parser.set_funct_const(7, temp_h);

                                                results_list.clear();
                                                VectorXd formel_vec = VectorXd::Zero(_sample_count);
                                                qreal norm = 0;
                                                for(qint32 i = 0; i < _sample_count; i++)
                                                {
                                                    formel_vec[i] = formula_parser.calculation(ui->cb_AtomFormula->currentText(), i);

                                                    //normalization
                                                    norm = formel_vec.norm();
                                                    if(norm != 0) formel_vec /= norm;

                                                    results_list.append(QString::number(formel_vec[i]));
                                                }
                                                stream << QString("%1_ATOM_%2 \n %3: a: %4 b: %5 c: %6 d: %7 e: %8 f: %9 g: %10 h: %11")
                                                          .arg(ui->tb_atom_name->text())
                                                          .arg(atomIndex)
                                                          .arg(ui->cb_AtomFormula->currentText())
                                                          .arg(temp_a)
                                                          .arg(temp_b)
                                                          .arg(temp_c)
                                                          .arg(temp_d)
                                                          .arg(temp_e)
                                                          .arg(temp_f)
                                                          .arg(temp_g)
                                                          .arg(temp_h) << "\n";

                                                for (QStringList::Iterator it = results_list.begin(); it != results_list.end(); it++)
                                                     stream << *it << "\n";

                                                xmlWriter.writeStartElement("ATOM");
                                                xmlWriter.writeAttribute("ID", QString::number(atomIndex));
                                                xmlWriter.writeAttribute("a", QString::number(temp_a));
                                                xmlWriter.writeAttribute("b", QString::number(temp_b));
                                                xmlWriter.writeAttribute("c", QString::number(temp_c));
                                                xmlWriter.writeAttribute("d", QString::number(temp_d));
                                                xmlWriter.writeAttribute("e", QString::number(temp_e));
                                                xmlWriter.writeAttribute("f", QString::number(temp_f));
                                                xmlWriter.writeAttribute("g", QString::number(temp_g));
                                                xmlWriter.writeAttribute("h", QString::number(temp_h));

                                                xmlWriter.writeStartElement("samples");
                                                QString samples_to_xml;
                                                for (qint32 it = 0; it < results_list.length(); it++)
                                                {
                                                    samples_to_xml.append(results_list.at(it));
                                                    samples_to_xml.append(":");
                                                }
                                                xmlWriter.writeAttribute("samples", samples_to_xml);
                                                xmlWriter.writeEndElement();

                                                xmlWriter.writeEndElement();


                                               atomIndex++;
                                               count_a++;
                                            }
                                            count_b++;
                                        }
                                        count_c++;
                                    }
                                    count_d++;
                                }
                                count_e++;
                            }
                            count_f++;
                        }
                        count_g++;
                    }
                    count_h++;
                }
            }
            else // not all params combined
            {
                qint32 atom_index = 0;
                while (atom_index < _atom_count)
                {
                    qreal temp_a = ui->dsb_StartValueA->value();
                    qreal temp_b = ui->dsb_StartValueB->value();
                    qreal temp_c = ui->dsb_StartValueC->value();
                    qreal temp_d = ui->dsb_StartValueD->value();
                    qreal temp_e = ui->dsb_StartValueE->value();
                    qreal temp_f = ui->dsb_StartValueF->value();
                    qreal temp_g = ui->dsb_StartValueG->value();
                    qreal temp_h = ui->dsb_StartValueH->value();
                    if(value_a_list.length() > 0 && atom_index < value_a_list.length()) temp_a = value_a_list.at(atom_index);
                    if(value_b_list.length() > 0 && atom_index < value_b_list.length()) temp_b = value_b_list.at(atom_index);
                    if(value_c_list.length() > 0 && atom_index < value_c_list.length()) temp_c = value_c_list.at(atom_index);
                    if(value_d_list.length() > 0 && atom_index < value_d_list.length()) temp_d = value_d_list.at(atom_index);
                    if(value_e_list.length() > 0 && atom_index < value_e_list.length()) temp_e = value_e_list.at(atom_index);
                    if(value_f_list.length() > 0 && atom_index < value_f_list.length()) temp_f = value_f_list.at(atom_index);
                    if(value_g_list.length() > 0 && atom_index < value_g_list.length()) temp_g = value_g_list.at(atom_index);
                    if(value_h_list.length() > 0 && atom_index < value_h_list.length()) temp_h = value_h_list.at(atom_index);

                    Formulaeditor formula_parser;
                    formula_parser.set_funct_const(0, temp_a);
                    formula_parser.set_funct_const(1, temp_b);
                    formula_parser.set_funct_const(2, temp_c);
                    formula_parser.set_funct_const(3, temp_d);
                    formula_parser.set_funct_const(4, temp_e);
                    formula_parser.set_funct_const(5, temp_f);
                    formula_parser.set_funct_const(6, temp_g);
                    formula_parser.set_funct_const(7, temp_h);

                    results_list.clear();
                    for(qint32 i = 0; i < _sample_count; i++)
                        results_list.append(QString::number(formula_parser.calculation(ui->cb_AtomFormula->currentText(), i)));

                    stream << QString("%1_ATOM_%2 \n %3: a: %4 b: %5 c: %6 d: %7 e: %8 f: %9 g: %10 h: %11")
                              .arg(ui->tb_atom_name->text())
                              .arg(atom_index)
                              .arg(ui->cb_AtomFormula->currentText())
                              .arg(temp_a)
                              .arg(temp_b)
                              .arg(temp_c)
                              .arg(temp_d)
                              .arg(temp_e)
                              .arg(temp_f)
                              .arg(temp_g)
                              .arg(temp_h) << "\n";

                   for (QStringList::Iterator it = results_list.begin(); it != results_list.end(); it++)
                        stream << *it << "\n";

                   xmlWriter.writeStartElement("ATOM");
                   xmlWriter.writeAttribute("ID", QString::number(atom_index));
                   xmlWriter.writeAttribute("a", QString::number(temp_a));
                   xmlWriter.writeAttribute("b", QString::number(temp_b));
                   xmlWriter.writeAttribute("c", QString::number(temp_c));
                   xmlWriter.writeAttribute("d", QString::number(temp_d));
                   xmlWriter.writeAttribute("e", QString::number(temp_e));
                   xmlWriter.writeAttribute("f", QString::number(temp_f));
                   xmlWriter.writeAttribute("g", QString::number(temp_g));
                   xmlWriter.writeAttribute("h", QString::number(temp_h));

                   xmlWriter.writeStartElement("samples");
                   QString samples_to_xml;
                   for (qint32 it = 0; it < results_list.length(); it++)
                   {
                       samples_to_xml.append(results_list.at(it));
                       samples_to_xml.append(":");
                   }
                   xmlWriter.writeAttribute("samples", samples_to_xml);
                   xmlWriter.writeEndElement();

                   xmlWriter.writeEndElement();


                   atom_index++;
                }
            }
            dict.close();
        }
        xmlWriter.writeEndElement();
        xmlWriter.writeEndElement();
        xmlWriter.writeEndDocument();

        xml_file.close();

        emit dict_saved();
    }
    else
    {
        QMessageBox::warning(this, tr("Error"),
        tr(".xml file not found"));
    }
}
