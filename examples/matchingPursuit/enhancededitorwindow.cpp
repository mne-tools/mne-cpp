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
* @brief    Implementation of EnhancedEditorWindow class.
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

qint32 _atom_count = 0;
qint32 _sample_count = 0;
QList<qreal> value_a_list;
QList<qreal> value_b_list;
QList<qreal> value_c_list;
QList<qreal> value_d_list;
QList<qreal> value_e_list;
QList<qreal> value_f_list;
QList<qreal> value_g_list;
QList<qreal> value_h_list;
QStringList names_list;

//***********************************************************************************************************************

//constructor
Enhancededitorwindow::Enhancededitorwindow(QWidget *parent): QWidget(parent), ui(new Ui::Enhancededitorwindow)
{
    this->setAccessibleName("formel");
    ui->setupUi(this);

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

    QString contents;
    QFile formula_file("Matching-Pursuit-Toolbox/user.fml");

    if (formula_file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        while(!formula_file.atEnd())
        {
            contents = formula_file.readLine(0).constData();
            ui->cb_AtomFormula->addItem(QIcon(":/images/icons/formel.png"), contents.trimmed());
        }
    }
    formula_file.close();

    QDir dictDir = QDir("Matching-Pursuit-Toolbox");

    QStringList filterList;
    filterList.append("*.dict");
    filterList.append("*.pdict");

    QFileInfoList fileList = dictDir.entryInfoList(filterList);
    for(int i = 0; i < fileList.length(); i++)
        names_list.append(fileList.at(i).baseName());

    if(ui->cb_AtomFormula->count() != 0)
        ui->cb_AtomFormula->setCurrentIndex(0);
}

//***********************************************************************************************************************

Enhancededitorwindow::~Enhancededitorwindow()
{
    delete ui;
}

//***********************************************************************************************************************

// when "all combined" toogelt
void Enhancededitorwindow::on_chb_allCombined_toggled(bool checked)
{
    if(checked)
    {
        ui->sb_Atomcount->setEnabled(false);
        ui->dsb_EndValueA->setEnabled(true);
        ui->dsb_EndValueB->setEnabled(true);
        ui->dsb_EndValueC->setEnabled(true);
        ui->dsb_EndValueD->setEnabled(true);
        ui->dsb_EndValueE->setEnabled(true);
        ui->dsb_EndValueF->setEnabled(true);
        ui->dsb_EndValueG->setEnabled(true);
        ui->dsb_EndValueH->setEnabled(true);
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
    ui->dsb_EndValueA->setValue(calc_end_value(ui->dsb_StartValueA->value(), ui->dsb_StartValueA->value()));
    ui->dsb_EndValueB->setValue(calc_end_value(ui->dsb_StartValueB->value(), ui->dsb_StartValueB->value()));
    ui->dsb_EndValueC->setValue(calc_end_value(ui->dsb_StartValueC->value(), ui->dsb_StartValueC->value()));
    ui->dsb_EndValueD->setValue(calc_end_value(ui->dsb_StartValueD->value(), ui->dsb_StartValueD->value()));
    ui->dsb_EndValueE->setValue(calc_end_value(ui->dsb_StartValueE->value(), ui->dsb_StartValueE->value()));
    ui->dsb_EndValueF->setValue(calc_end_value(ui->dsb_StartValueF->value(), ui->dsb_StartValueF->value()));
    ui->dsb_EndValueG->setValue(calc_end_value(ui->dsb_StartValueG->value(), ui->dsb_StartValueG->value()));
    ui->dsb_EndValueH->setValue(calc_end_value(ui->dsb_StartValueH->value(), ui->dsb_StartValueH->value()));

    _atom_count = ui->sb_Atomcount->value();
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

    ui->fr_A->setHidden(true);
    ui->fr_B->setHidden(true);
    ui->fr_C->setHidden(true);
    ui->fr_D->setHidden(true);
    ui->fr_E->setHidden(true);
    ui->fr_F->setHidden(true);
    ui->fr_G->setHidden(true);
    ui->fr_H->setHidden(true);

    for(qint32 j = 0; j < foundChar.length(); j++)
    {
        if(foundChar.at(j) =='A') ui->fr_A->setHidden(false);
        else if(foundChar.at(j) =='B') ui->fr_B->setHidden(false);
        else if(foundChar.at(j) =='C') ui->fr_C->setHidden(false);
        else if(foundChar.at(j) =='D') ui->fr_D->setHidden(false);
        else if(foundChar.at(j) =='E') ui->fr_E->setHidden(false);
        else if(foundChar.at(j) =='F') ui->fr_F->setHidden(false);
        else if(foundChar.at(j) =='G') ui->fr_G->setHidden(false);
        else if(foundChar.at(j) =='H') ui->fr_H->setHidden(false);
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

    QFile formel_file("Matching-Pursuit-Toolbox/user.fml");
    QFile formel_temp_file("Matching-Pursuit-Toolbox/user.temp");

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
    formel_temp_file.rename("Matching-Pursuit-Toolbox/user.fml");

    ui->cb_AtomFormula->removeItem(ui->cb_AtomFormula->currentIndex());
}

//***********************************************************************************************************************

QList<qreal> Enhancededitorwindow::calc_value_list(qreal startValue, qreal linStepValue)
{
    QList<qreal> resultList;
    for(qint32 i = 0; i < _atom_count; i++)
        resultList.append(startValue + (i * linStepValue));

    return resultList;
}

//***********************************************************************************************************************

qreal Enhancededitorwindow::calc_end_value(qreal startValue, qreal linStepValue)
{
    return startValue + (_atom_count * linStepValue);
}


//***********************************************************************************************************************

void Enhancededitorwindow::on_dsb_StepVauleA_editingFinished()
{
    value_a_list = calc_value_list(ui->dsb_StartValueA->value(), ui->dsb_StepVauleA->value());
}

void Enhancededitorwindow::on_dsb_StepVauleB_editingFinished()
{
    value_b_list = calc_value_list(ui->dsb_StartValueB->value(), ui->dsb_StepVauleB->value());
}

void Enhancededitorwindow::on_dsb_StepVauleC_editingFinished()
{
    value_c_list = calc_value_list(ui->dsb_StartValueC->value(), ui->dsb_StepVauleC->value());
}

void Enhancededitorwindow::on_dsb_StepVauleD_editingFinished()
{
    value_d_list = calc_value_list(ui->dsb_StartValueD->value(), ui->dsb_StepVauleD->value());
}

void Enhancededitorwindow::on_dsb_StepVauleE_editingFinished()
{
    value_e_list = calc_value_list(ui->dsb_StartValueE->value(), ui->dsb_StepVauleE->value());
}

void Enhancededitorwindow::on_dsb_StepVauleF_editingFinished()
{
    value_f_list = calc_value_list(ui->dsb_StartValueF->value(), ui->dsb_StepVauleF->value());
}

void Enhancededitorwindow::on_dsb_StepVauleG_editingFinished()
{
    value_g_list = calc_value_list(ui->dsb_StartValueH->value(), ui->dsb_StepVauleG->value());
}

void Enhancededitorwindow::on_dsb_StepVauleH_editingFinished()
{
    value_h_list = calc_value_list(ui->dsb_StartValueG->value(), ui->dsb_StepVauleH->value());
}

//***********************************************************************************************************************

void Enhancededitorwindow::on_dsb_StartValueA_editingFinished()
{
   value_a_list = calc_value_list(ui->dsb_StartValueA->value(), ui->dsb_StepVauleA->value());
}

void Enhancededitorwindow::on_dsb_StartValueB_editingFinished()
{
    value_b_list = calc_value_list(ui->dsb_StartValueB->value(), ui->dsb_StepVauleB->value());
}

void Enhancededitorwindow::on_dsb_StartValueC_editingFinished()
{
    value_c_list = calc_value_list(ui->dsb_StartValueC->value(), ui->dsb_StepVauleC->value());
}

void Enhancededitorwindow::on_dsb_StartValueD_editingFinished()
{
    value_d_list = calc_value_list(ui->dsb_StartValueD->value(), ui->dsb_StepVauleD->value());
}

void Enhancededitorwindow::on_dsb_StartValueE_editingFinished()
{
    value_e_list = calc_value_list(ui->dsb_StartValueE->value(), ui->dsb_StepVauleE->value());
}

void Enhancededitorwindow::on_dsb_StartValueF_editingFinished()
{
    value_f_list = calc_value_list(ui->dsb_StartValueF->value(), ui->dsb_StepVauleF->value());
}

void Enhancededitorwindow::on_dsb_StartValueG_editingFinished()
{
    value_g_list = calc_value_list(ui->dsb_StartValueH->value(), ui->dsb_StepVauleG->value());
}

void Enhancededitorwindow::on_dsb_StartValueH_editingFinished()
{
    value_h_list = calc_value_list(ui->dsb_StartValueG->value(), ui->dsb_StepVauleH->value());
}

//***********************************************************************************************************************

void Enhancededitorwindow::on_dsb_StepVauleA_valueChanged(double arg1)
{
    ui->dsb_EndValueA->setValue(calc_end_value(ui->dsb_StartValueA->value(), arg1));
}

void Enhancededitorwindow::on_dsb_StepVauleB_valueChanged(double arg1)
{
    ui->dsb_EndValueB->setValue(calc_end_value(ui->dsb_StartValueB->value(), arg1));
}

void Enhancededitorwindow::on_dsb_StepVauleC_valueChanged(double arg1)
{
    ui->dsb_EndValueC->setValue(calc_end_value(ui->dsb_StartValueC->value(), arg1));
}

void Enhancededitorwindow::on_dsb_StepVauleD_valueChanged(double arg1)
{
    ui->dsb_EndValueD->setValue(calc_end_value(ui->dsb_StartValueD->value(), arg1));
}

void Enhancededitorwindow::on_dsb_StepVauleE_valueChanged(double arg1)
{
    ui->dsb_EndValueE->setValue(calc_end_value(ui->dsb_StartValueE->value(), arg1));
}

void Enhancededitorwindow::on_dsb_StepVauleF_valueChanged(double arg1)
{
    ui->dsb_EndValueF->setValue(calc_end_value(ui->dsb_StartValueF->value(), arg1));
}

void Enhancededitorwindow::on_dsb_StepVauleG_valueChanged(double arg1)
{
    ui->dsb_EndValueG->setValue(calc_end_value(ui->dsb_StartValueG->value(), arg1));
}

void Enhancededitorwindow::on_dsb_StepVauleH_valueChanged(double arg1)
{
    ui->dsb_EndValueH->setValue(calc_end_value(ui->dsb_StartValueH->value(), arg1));
}

//***********************************************************************************************************************


void Enhancededitorwindow::on_dsb_StartValueA_valueChanged(double arg1)
{
    ui->dsb_EndValueA->setValue(calc_end_value(arg1, ui->dsb_StepVauleA->value()));
}

void Enhancededitorwindow::on_dsb_StartValueB_valueChanged(double arg1)
{
    ui->dsb_EndValueB->setValue(calc_end_value(arg1, ui->dsb_StepVauleB->value()));
}

void Enhancededitorwindow::on_dsb_StartValueC_valueChanged(double arg1)
{
    ui->dsb_EndValueC->setValue(calc_end_value(arg1, ui->dsb_StepVauleC->value()));
}

void Enhancededitorwindow::on_dsb_StartValueD_valueChanged(double arg1)
{
    ui->dsb_EndValueD->setValue(calc_end_value(arg1, ui->dsb_StepVauleD->value()));
}

void Enhancededitorwindow::on_dsb_StartValueE_valueChanged(double arg1)
{
    ui->dsb_EndValueE->setValue(calc_end_value(arg1, ui->dsb_StepVauleE->value()));
}

void Enhancededitorwindow::on_dsb_StartValueF_valueChanged(double arg1)
{
    ui->dsb_EndValueF->setValue(calc_end_value(arg1, ui->dsb_StepVauleF->value()));
}

void Enhancededitorwindow::on_dsb_StartValueG_valueChanged(double arg1)
{
    ui->dsb_EndValueG->setValue(calc_end_value(arg1, ui->dsb_StepVauleG->value()));
}

void Enhancededitorwindow::on_dsb_StartValueH_valueChanged(double arg1)
{
    ui->dsb_EndValueH->setValue(calc_end_value(arg1, ui->dsb_StepVauleH->value()));
}

//***********************************************************************************************************************

// calc all atoms with choosen parameters and save to list and to drive
void Enhancededitorwindow::on_pushButton_clicked()
{
    QStringList results_list;
    results_list.clear();

    QString savePath = QString("Matching-Pursuit-Toolbox/%1.pdict").arg(ui->tb_atom_name->text());
    QFile dict(savePath);

    if(ui->tb_atom_name->text().isEmpty())
    {
        QMessageBox::warning(this, tr("Error"),
        tr("There was no name assigned."));
        ui->tb_atom_name->setFocus();
        return;
    }

    for(qint32 k = 0; k < names_list.length(); k++)
    {
        if(QString::compare(ui->tb_atom_name->text(), names_list.at(k)) == 0)
        {
            QMessageBox::warning(this, tr("Error"),
                tr("The name is already taken."));

                ui->tb_atom_name->setFocus();
                ui->tb_atom_name->selectAll();
                return;
        }
    }

    if (dict.open (QIODevice::WriteOnly| QIODevice::Append))
    {
        QTextStream stream( &dict );
        stream << QString("atomcount = %1 ").arg(_atom_count) << "\n";

        qint32 atomIndex = 0;
        qint32 count_h = 0;

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
                                        qreal temp_a = 0;
                                        qreal temp_b = 0;
                                        qreal temp_c = 0;
                                        qreal temp_d = 0;
                                        qreal temp_e = 0;
                                        qreal temp_f = 0;
                                        qreal temp_g = 0;
                                        qreal temp_h = 0;
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

                                        for(qint32 i = 0; i < _sample_count; i++)
                                            results_list.append( QString::number(formula_parser.calculation(ui->cb_AtomFormula->currentText(), i)));

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
        dict.close();
    }
}
