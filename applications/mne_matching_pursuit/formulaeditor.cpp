//=============================================================================================================
/**
* @file     formulaeditor.cpp
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
*     * Neither the name of the Massachusetts General Hospital nor the names of its contributors may be used
*       to endorse or promote products derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MASSACHUSETTS GENERAL HOSPITAL BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*
* @brief    Definition of FormulaEditor class.
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "formulaeditor.h"
#include "ui_formulaeditor.h"
#include "mainwindow.h"

#include <math.h>
#include <float.h>
#include <iostream>

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtGui>
#include <QtCore>
#include <QMessageBox>

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================



//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

QString oldStringX = "";
QString oldStringA = "";
QString oldStringB = "";
QString oldStringC = "";
QString oldStringD = "";
QString oldStringE = "";
QString oldStringF = "";
QString oldStringG = "";
QString oldStringH = "";
QString Formulaeditor::g_strF = "";
QString errorText = "";
QList<qreal> atomList;
qint32 atomLength;

qreal startValue;
qreal endValue;
qreal stepWidth;

//*************************************************************************************************************
//=============================================================================================================
// MAIN
//=============================================================================================================
Formulaeditor::Formulaeditor(QWidget *parent) :    QWidget(parent),    ui(new Ui::Formulaeditor)
{
    ui->setupUi(this);

    QSettings settings;
    move(settings.value("pos_formula_editor", QPoint(200, 200)).toPoint());
    resize(settings.value("size_formula_editor", QSize(555, 418)).toSize());

    callAtomPaintWindow = new AtomPaintWindow();
    ui->l_PaintAtom->addWidget(callAtomPaintWindow);

    atomList.clear();

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

    callAtomPaintWindow->update();
    //update();
    m_dFktValue = 0;
}

//*************************************************************************************************************************************

Formulaeditor::~Formulaeditor()
{
    delete ui;
}

//*************************************************************************************************************************************

void Formulaeditor::closeEvent(QCloseEvent * event)
{
    Q_UNUSED(event);
    QSettings settings;
    if(!this->isMaximized())
    {
        settings.setValue("pos_formula_editor", pos());
        settings.setValue("size_formula_editor", size());
    }
}

//*************************************************************************************************************************************

void AtomPaintWindow::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    paint_signal(atomList, this->size());
}

//*************************************************************************************************************************************

void AtomPaintWindow::paint_signal(QList<qreal> valueList, QSize windowSize)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillRect(0,0,windowSize.width(),windowSize.height(),QBrush(Qt::white));


    if(valueList.length() > 0)
    {
        qint32 borderMarginHeigth = 15;     // reduce paintspace in GraphWindow of borderMargin pixels
        qint32 borderMarginWidth = 5;       // reduce paintspace in GraphWindow of borderMargin pixels
        qint32 i = 0;
        qreal maxNeg = 0;                   // smalest signalvalue
        qreal maxPos = 0;                   // highest signalvalue
        qreal absMin = 0;                   // minimum of abs(maxNeg and maxPos)
        qint32 drawFactor = 0;              // shift factor for decimal places (linear)
        qint32 startDrawFactor = 1;         // shift factor for decimal places (exponential-base 10)
        qint32 decimalPlace = 0;            // decimal places for axis title
        QPolygonF polygon;                  // points for drwing the signal
        QList<qreal> internListValue;       // intern representation of y-axis values of the signal (for painting only)

        internListValue.clear();
        while(i < valueList.length())
        {
                internListValue.append(valueList.at(i));            //TODO stupid
                i++;
        }

        // paint window white
        painter.fillRect(0,0,windowSize.width(),windowSize.height(),QBrush(Qt::white));

        // find min and max of signal
        i = 0;
        while(i < valueList.length())
        {
            if(valueList.at(i) > maxPos)
                maxPos = valueList.at(i);

            if(valueList.at(i) < maxNeg )
                maxNeg = valueList.at(i);
            i++;
        }

        if(maxPos > std::fabs(maxNeg)) absMin = maxNeg;      // find absolute minimum of (maxPos, maxNeg)
        else     absMin = maxPos;

        if(absMin != 0)                                 // absMin must not be zero
        {
            while(true)                                 // shift factor for decimal places?
            {
                if(std::fabs(absMin) < 1)                    // if absMin > 1 , no shift of decimal places nescesary
                {
                    absMin = absMin * 10;
                    drawFactor++;                       // shiftfactor counter
                }
                if(std::fabs(absMin) >= 1) break;
            }
        }

        // shift of decimal places with drawFactor for all signalpoints and save to intern list
        while(drawFactor > 0)
        {
            i = 0;
            while(i < valueList.length())
            {
                qreal replaceValue = internListValue.at(i) * 10;
                internListValue.replace(i, replaceValue);
                i++;
            }
            startDrawFactor = startDrawFactor * 10;
            decimalPlace++;
            maxPos = maxPos * 10;
            maxNeg = maxNeg * 10;
            drawFactor--;
        }

        qreal maxmax;
        // absolute signalheight
        if(maxNeg <= 0)     maxmax = maxPos - maxNeg;
        else  maxmax = maxPos + maxNeg;


        // scale axis title
        qreal scaleYText = (qreal)maxmax / (qreal)10;
        qint32 negScale = floor((maxNeg * 10 / maxmax)+0.5);

        //find lenght of text of y-axis for shift of y-axis to the right (so the text will stay readable and is not painted into the y-axis
        qint32 k = 0;
        qint32 negScale2 = negScale;
        qint32 maxStrLenght = 0;
        while(k < 16)
        {
            QString string2;

            qreal scaledYText = negScale2 * scaleYText / (qreal)startDrawFactor;    // scale value y-axis
            string2  = QString::number(scaledYText, 'f', decimalPlace + 1);         // scale in string with correct decimal places (precision depends on signal codomain)

            if(string2.length()> maxStrLenght) maxStrLenght = string2.length();

            k++;
            negScale2++;
        }
        maxStrLenght = 6 + maxStrLenght * 6;

        //todo adjust distance for last text
        //while(((windowSize.width() - maxStrLenght - borderMarginWidth)*100) % 15)  borderMarginWidth++;

        qreal inStartValue = startValue;
        qreal inEndValue = endValue;        

        // scale signal
        qreal scaleX = ((qreal)(windowSize.width() - maxStrLenght - 6 * borderMarginWidth))/ ((qreal)valueList.length() - 1);
        qreal scaleY = (qreal)(windowSize.height() - borderMarginHeigth) / (qreal)maxmax;

        //scale axis
        qreal scaleXAchse = (qreal)(windowSize.width() - maxStrLenght - 6 * borderMarginWidth) / (qreal)15;
        qreal scaleYAchse = (qreal)(windowSize.height() - borderMarginHeigth) / (qreal)10;

        // position of title of x-axis
        qint32 xAxisTextPos = 8;
        if(maxNeg == 0) xAxisTextPos = -10; // if signal only positiv: titles above axis

        i = 1;

        qreal internStepWidth = (endValue - startValue) / 15;
        qint32 decimalPlaceX = 0;
        if(internStepWidth > 0.000999999999999)
            decimalPlaceX = 3;
        if(internStepWidth > 0.009999999)
            decimalPlaceX = 2;
        if(internStepWidth > 0.0999999999)
            decimalPlaceX = 1;
        if(internStepWidth > 0.999999)
            decimalPlace = 0;
        while(i <= 11)
        {
            QString string;

            qreal scaledYText = negScale * scaleYText / (qreal)startDrawFactor; // scale value y-axis

            string  = QString::number(scaledYText, 'f', decimalPlace + 1);      // scale in string with correct decimal places (precision depends on signal codomain)

            if(negScale == 0)                                                   // x-axis reached (y-value = 0)
            {
                // append scaled signalpoints
                qint32 h = 0;
                while(h < valueList.length())
                {
                    polygon.append(QPointF(h * scaleX + maxStrLenght + 10,  -((internListValue.at(h) * scaleY + ((i - 1) * scaleYAchse)-(windowSize.height()) + borderMarginHeigth / 2))));
                    h++;
                }

                // paint x-axis
                qreal j = 0;
                inStartValue = startValue;                
                while(inStartValue <= inEndValue)
                {
                    QString str;
                    painter.drawText(j * scaleXAchse + maxStrLenght + 3, -(((i - 1) * scaleYAchse)-(windowSize.height())) + xAxisTextPos, str.append(QString::number(inStartValue, 'f', decimalPlaceX)));      // scale value x-axis
                    painter.drawLine(j * scaleXAchse + maxStrLenght + 10, -(((i - 1) * scaleYAchse)-(windowSize.height() - borderMarginHeigth / 2 - 2)), j * scaleXAchse + maxStrLenght + 10 , -(((i - 1) * scaleYAchse)-(windowSize.height() - borderMarginHeigth / 2 + 2)));   // marks of x-axis
                    j++;
                    inStartValue += internStepWidth;
                }
                painter.drawLine(maxStrLenght, -(((i - 1) * scaleYAchse)-(windowSize.height()) + borderMarginHeigth / 2), windowSize.width()-5, -(((i - 1) * scaleYAchse)-(windowSize.height()) + borderMarginHeigth / 2));
            }

            painter.drawText(3, -((i - 1) * scaleYAchse - windowSize.height()) - borderMarginHeigth/2 + 4, string);     // paint scale value y-axis
            painter.drawLine(maxStrLenght - 2, -(((i - 1) * scaleYAchse)-(windowSize.height()) + borderMarginHeigth / 2), maxStrLenght + 2, -(((i - 1) * scaleYAchse)-(windowSize.height()) + borderMarginHeigth / 2));  // marks of y-axis
            i++;
            negScale++;
        }

        painter.drawLine(maxStrLenght, 2, maxStrLenght, windowSize.height() - 2);     // paint y-axis

        painter.drawPolyline(polygon);                 // paint signal
    }
    painter.end();
}

//*************************************************************************************************************************************

void Formulaeditor::on_tb_Formula_textChanged(const QString &arg1)
{
    ui->btt_Save->setEnabled(false);
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

    ui->lb_A->setEnabled(false);
    ui->tb_A->setEnabled(false);
    ui->lb_B->setEnabled(false);
    ui->tb_B->setEnabled(false);
    ui->lb_C->setEnabled(false);
    ui->tb_C->setEnabled(false);
    ui->lb_D->setEnabled(false);
    ui->tb_D->setEnabled(false);
    ui->lb_E->setEnabled(false);
    ui->tb_E->setEnabled(false);
    ui->lb_F->setEnabled(false);
    ui->tb_F->setEnabled(false);
    ui->lb_G->setEnabled(false);
    ui->tb_G->setEnabled(false);
    ui->lb_H->setEnabled(false);
    ui->tb_H->setEnabled(false);
    ui->lb_StartValue->setEnabled(false);
    ui->lb_StepWidth->setEnabled(false);
    ui->lb_EndValue->setEnabled(false);
    ui->dsb_StartValue->setEnabled(false);
    ui->dsb_StepWidth->setEnabled(false);
    ui->dsb_EndValue->setEnabled(false);

    for(qint32 j = 0; j < foundChar.length(); j++)
    {
        if(foundChar.at(j) =='A')
        {
            ui->lb_A->setEnabled(true);
            ui->tb_A->setEnabled(true);
        }
        else if(foundChar.at(j) =='B')
        {
            ui->lb_B->setEnabled(true);
            ui->tb_B->setEnabled(true);
        }
        else if(foundChar.at(j) =='C')
        {
            ui->lb_C->setEnabled(true);
            ui->tb_C->setEnabled(true);
        }
        else if(foundChar.at(j) =='D')
        {
            ui->lb_D->setEnabled(true);
            ui->tb_D->setEnabled(true);
        }
        else if(foundChar.at(j) =='E')
        {
            ui->lb_E->setEnabled(true);
            ui->tb_E->setEnabled(true);
        }
        else if(foundChar.at(j) =='F')
        {
            ui->lb_F->setEnabled(true);
            ui->tb_F->setEnabled(true);
        }
        else if(foundChar.at(j) =='G')
        {
            ui->lb_G->setEnabled(true);
            ui->tb_G->setEnabled(true);
        }
        else if(foundChar.at(j) =='H')
        {
            ui->lb_H->setEnabled(true);
            ui->tb_H->setEnabled(true);
        }
        else if(foundChar.at(j) =='X')
        {
            ui->lb_StartValue->setEnabled(true);
            ui->lb_StepWidth->setEnabled(true);
            ui->lb_EndValue->setEnabled(true);
            ui->dsb_StartValue->setEnabled(true);
            ui->dsb_StepWidth->setEnabled(true);
            ui->dsb_EndValue->setEnabled(true);
        }
    }
}

//*************************************************************************************************************************************

void Formulaeditor::on_tb_A_textChanged(const QString &arg1)
{
    bool ok = false;
    arg1.toFloat(&ok);
    if(ok)  oldStringA = arg1;
    else if(arg1 != "")   ui->tb_A->setText(oldStringA);
}

void Formulaeditor::on_tb_B_textChanged(const QString &arg1)
{
    bool ok = false;
    arg1.toFloat(&ok);
    if(ok)  oldStringB = arg1;
    else if(arg1 != "")   ui->tb_B->setText(oldStringB);
}

void Formulaeditor::on_tb_C_textChanged(const QString &arg1)
{
    bool ok = false;
    arg1.toFloat(&ok);
    if(ok)  oldStringC = arg1;
    else if(arg1 != "")    ui->tb_C->setText(oldStringC);
}

void Formulaeditor::on_tb_D_textChanged(const QString &arg1)
{
    bool ok = false;
    arg1.toFloat(&ok);
    if(ok)  oldStringD = arg1;
    else if(arg1 != "")   ui->tb_D->setText(oldStringD);
}

void Formulaeditor::on_tb_E_textChanged(const QString &arg1)
{
    bool ok = false;
    arg1.toFloat(&ok);
    if(ok)  oldStringE = arg1;
    else if(arg1 != "")   ui->tb_E->setText(oldStringE);
}

void Formulaeditor::on_tb_F_textChanged(const QString &arg1)
{
    bool ok = false;
    arg1.toFloat(&ok);
    if(ok)  oldStringF = arg1;
    else if(arg1 != "")   ui->tb_F->setText(oldStringF);
}

void Formulaeditor::on_tb_G_textChanged(const QString &arg1)
{
    bool ok = false;
    arg1.toFloat(&ok);
    if(ok)  oldStringG = arg1;
    else if(arg1 != "")   ui->tb_G->setText(oldStringG);
}

void Formulaeditor::on_tb_H_textChanged(const QString &arg1)
{
    bool ok = false;
    arg1.toFloat(&ok);
    if(ok)  oldStringH = arg1;
    else if(arg1 != "")   ui->tb_H->setText(oldStringH);
}

void Formulaeditor::on_dsb_StartValue_editingFinished()
{
    ui->dsb_EndValue->setMinimum(ui->dsb_StartValue->value() + ui->dsb_StepWidth->value());
}

void Formulaeditor::on_dsb_StepWidth_editingFinished()
{
    ui->dsb_EndValue->setMinimum(ui->dsb_StartValue->value() + ui->dsb_StepWidth->value());
}

//*************************************************************************************************************************************

void Formulaeditor::on_btt_Test_clicked()
{
    Formulaeditor FormulaParser;

    FormulaParser.set_funct_const(0, ui->tb_A->text().toFloat());
    FormulaParser.set_funct_const(1, ui->tb_B->text().toFloat());
    FormulaParser.set_funct_const(2, ui->tb_C->text().toFloat());
    FormulaParser.set_funct_const(3, ui->tb_D->text().toFloat());
    FormulaParser.set_funct_const(4, ui->tb_E->text().toFloat());
    FormulaParser.set_funct_const(5, ui->tb_F->text().toFloat());
    FormulaParser.set_funct_const(6, ui->tb_G->text().toFloat());
    FormulaParser.set_funct_const(7, ui->tb_H->text().toFloat());

    double retValue = FormulaParser.calculation(ui->tb_Formula->text(), ui->dsb_StartValue->value());  // TODO Float or only Int at X

    if(errorText.isEmpty())
    {
        if(ui->lb_StartValue->isEnabled())
            ui->btt_Save->setEnabled(true);
        if(retValue < 0.0000000000001 && retValue > -0.0000000000001)
            retValue = 0;
        ui->lb_Result->setText(QString("result start value = %1").arg(retValue));
    }
    else
    {
        ui->lb_Result->setText(errorText);
        errorText = "";
        return;
    }
    QList<qreal> resultsList;
    resultsList.clear();
    startValue = ui->dsb_StartValue->value();
    qreal internStartValue = startValue;
    endValue =  ui->dsb_EndValue->value();
    stepWidth = ui->dsb_StepWidth->value();
    if(stepWidth == 0)
    {
        QMessageBox::warning(this, tr("error"), tr("Increment can not be null."));
        return;
    }

    if(ui->dsb_StartValue->isEnabled() && ui->dsb_EndValue->isEnabled() && ui->dsb_StepWidth->isEnabled())
    {
        while(internStartValue  <= endValue)
        {
            resultsList.append(FormulaParser.calculation(ui->tb_Formula->text(), internStartValue ));
            internStartValue  += stepWidth;
        }
    }
    else
    {
        qreal result = FormulaParser.calculation(ui->tb_Formula->text(), internStartValue );
        ui->lb_Result->setText(QString("result = %1").arg(result));
    }

    atomList = resultsList;
    atomLength = resultsList.length();
    errorText = "";
    callAtomPaintWindow->update();
    update();
}

//*************************************************************************************************************************************

// access when "sforumla save" clicked
void Formulaeditor::on_btt_Save_clicked()
{    
    QFile saveFile(QDir::homePath() + "/" + "Matching-Pursuit-Toolbox/user.fml");
    if(!saveFile.exists())
    {
        if (saveFile.open(QIODevice::ReadWrite | QIODevice::Text))
        saveFile.close();
    }

    if (saveFile.open(QIODevice::WriteOnly| QIODevice::Append))
    {
        QTextStream stream( &saveFile);
        stream << ui->tb_Formula->text() << "\n";
    }
    saveFile.close();

    ui->tb_Formula->clear();
    ui->tb_A->setText("0");
    ui->tb_B->setText("0");
    ui->tb_C->setText("0");
    ui->tb_D->setText("0");
    ui->tb_E->setText("0");
    ui->tb_F->setText("0");
    ui->tb_G->setText("0");
    ui->tb_H->setText("0");
    ui->lb_Result->setText("result start value = ---");
    atomList.clear();
    update();

    emit formula_saved();
}

//*************************************************************************************************************************************
// formula methods    Copyright: 2004, Ralf Wirtz   adapted to qt for formula editor by Daniel Knobl **********************************
// this Code is licensed under The Code Project Open License (CPOL) 1.02 **************************************************************
//*************************************************************************************************************************************

qreal Formulaeditor::sign_factor(qint32& nPosition, QString& strCharacter)
{
  if (strCharacter == "-")
    {
        char_n(nPosition, strCharacter);
        return (-1.0) * factor(nPosition, strCharacter);
    }
  else return factor(nPosition, strCharacter);
}

//*************************************************************************************************************************************

void Formulaeditor::strip_formula(QString &strFormula)
{
    qint32 level = 0;

    if (strFormula.length() < 1) return;

    // replace comma by points all round brackets
    strFormula.replace("PI", "3.14159265358979323846");
    strFormula.replace("pi", "3.14159265358979323846");
    strFormula.replace("Pi", "3.14159265358979323846");
    strFormula.replace("pI", "3.14159265358979323846");
    strFormula.replace(" ", "");
    strFormula.replace(",", ".");
    strFormula.replace("[", "(");
    strFormula.replace("]", ")");
    strFormula.replace("{", "(");
    strFormula.replace("}", ")");

    strFormula.replace("*(1)","");
    strFormula.replace("(1)*","");

    strFormula.replace("*(x)","*x");    
    strFormula.replace("((x)*","(x*");
    strFormula.replace("+(x)*","+x*");
    strFormula.replace("-(x)*","-x*");
    strFormula.replace("*(x)*","*x*");
    strFormula.replace("(sin(x))","sin(x)");
    strFormula.replace("(cos(x))","cos(x)");
    strFormula.replace("(cot(x))","cot(x)");
    strFormula.replace("(tan(x))","tan(x)");
    strFormula.replace("(exp(x))","exp(x)");
    strFormula.replace("(log(x))","log(x)");
    strFormula.replace("(ln(x))","ln(x)");

    // delete enclosing marks
    for (qint32 i = 0; i < strFormula.length(); i++)
    {
        if(QString::compare(strFormula.at(i), "(") == 0)
        {
            level++;
            continue;
        }
        if(QString::compare(strFormula.at(i), ")") == 0)
        {
            level--;
            continue;
        }

        if (level == 0 && i < strFormula.length() - 1)
        {
            level = -1; // marker
            break;
        }
    }

    if (level != -1)
    {
        while (strFormula.at(0) == '(' && strFormula.at(strFormula.length() - 1) == ')')
        {
            strFormula = strFormula.mid(1, strFormula.length() - 2);
        }
    }

    // delete whitespaces and positive signs
    while (strFormula.at(0) == '+'
           || strFormula.at(0) == ' ')
    {
        strFormula = strFormula.mid(1);
    }
    strFormula.trimmed();

    //remove unused brackets
    qint32 Pos[1000];
    qint32 j = 0;
    level = 0;
    qint32 l = strFormula.length();

    for (qint32 i = 0; i < l; i++)
    {
        if (strFormula.at(i) == '(')
        {
            qint32 min = 0;
            if(i+1 < l-1)
                    min = i+1;
            else
                    min = l - 1;
            if (i == 0 || (i > 0 && (strFormula.at(i-1) == '(' || strFormula.at(min) == '(')))
            {
                level++;
                Pos[++j] = i;
            }
        }
        else if (strFormula.at(i) == ')')
        {
            level--;
            if (level > 0 && strFormula.at(i+1) == ')')
            {
                //right bracket
                QString left = strFormula.left(i);
                QString mid = strFormula.mid(i+1);
                strFormula = left + "|" + mid;

                //left bracket
                left = strFormula.left(Pos[level] + 1);
                mid = strFormula.mid(Pos[level+1] + 1);
                strFormula = left + "|" + mid;

                j = 0;
            }
        }
    }
    strFormula.replace("|", "");
}

//*************************************************************************************************************************************

double Formulaeditor::calculation(QString strFormula, qreal xValue, bool strip)
{
    qint32  nPosition;
    QString strCharacter;
    qreal	result;

    if (strFormula.length() < 1) return 0.0;

    m_strErrortext = "";

    if (strip) strip_formula(strFormula);

    m_strFunction = strFormula;
    m_dFktValue = xValue;
    if (m_dFktValue == 0)
    m_dFktValue = FLT_MIN;
    nPosition = 0;
    char_n(nPosition, strCharacter);

    result = expression(nPosition, strCharacter);

    return result;
}

//*************************************************************************************************************************************

double Formulaeditor::expression(int& nPosition, QString& strCharacter)
{
  QString strOperator;
  double erg = simple_expression(nPosition, strCharacter);
  while (strCharacter == "+" || strCharacter == "-")
  {
    strOperator = strCharacter;
    char_n(nPosition, strCharacter);
    if (strOperator == "+")
        erg += simple_expression(nPosition, strCharacter);
    else if (strOperator == "-")
        erg -= simple_expression(nPosition, strCharacter);
  }

  return erg;
}

//*************************************************************************************************************************************

double Formulaeditor::simple_expression(int& nPosition, QString& strCharacter)
{
    double s,dum;
    QString strOperator;
    s = term(nPosition, strCharacter);
    while (strCharacter == "*" || strCharacter == "/")
    {
        strOperator = strCharacter;
        char_n(nPosition, strCharacter);
        if (strOperator == "*")
            s = s * term(nPosition, strCharacter);
        else if (strOperator == "/")
        {
            dum = term(nPosition, strCharacter);
            if (dum != 0)   s = s / dum;
            else    errorText = QString("Divide by 0 is not possible.");
        }
    }
    return s;
}

//*************************************************************************************************************************************

double Formulaeditor::term(int& nPosition, QString& strCharacter)
{
  qreal t,vz;
  t = sign_factor(nPosition, strCharacter);
  while (strCharacter == "^")
  {
      char_n(nPosition, strCharacter);
      vz = sign_factor(nPosition, strCharacter);

      if ((t <= 0 && std::fabs(vz) <= 1) || (t <= 0 && vz != qint32(vz))) errorText = QString("Extraction of square root of negative numbers is not possible using dense matrix algebra.");
      else    t = pow(t,vz);
  }
  return t;
}

//*************************************************************************************************************************************

double Formulaeditor::char_n(int& nPosition, QString& strCharacter)
{
    do
    {
        nPosition ++;
        if (nPosition <= m_strFunction.length())
            strCharacter = m_strFunction.mid(nPosition - 1, 1);
        else
            strCharacter = str_char("?");
    }
    while (strCharacter == " ");

    return nPosition;
}

//*************************************************************************************************************************************

void Formulaeditor::set_formula(QString Formula)
{
    m_strFormula = Formula;
}

//*************************************************************************************************************************************

QString Formulaeditor::get_formula()
{
    return m_strFormula;
}

//*************************************************************************************************************************************

double Formulaeditor::factor(qint32& nPosition, QString& strCharacter)
{
    qreal f = 0.0;
    qint32 wI = 0, wL = 0, wBeginn = 0, wError = 0;

    if	(strCharacter == str_char(0)) return 0.0;
    // read digit and save as float in f
    if (((strCharacter >= "0") && (strCharacter <= "9")) || (strCharacter == "."))
    {
        wBeginn = nPosition;

        do
        {
            char_n(nPosition, strCharacter);
        }
        while ((((strCharacter >= "0") && (strCharacter <= "9")) || (strCharacter == ".")));

        if (strCharacter == ".")
        {
            do
            {
                char_n(nPosition, strCharacter);
            }
            while (!(((qint8)strCharacter.at(0).digitValue() >= 0) && ((qint8)strCharacter.at(0).digitValue() <=  9))  || (strCharacter.at(0) == '.'));
        }

        QString g_strF = m_strFunction.mid(wBeginn - 1, nPosition - wBeginn);
        f = g_strF.toFloat();
    }
    else
    {
        QString strCharacterUpper = strCharacter.toUpper();
        if (strCharacter == "(")
        {
            char_n(nPosition, strCharacter);
            f = expression(nPosition, strCharacter);
            if (strCharacter == ")")
                char_n(nPosition, strCharacter);
        }
        else if (strCharacterUpper == "X")
        {
            char_n(nPosition, strCharacter);
            f = m_dFktValue;
        }
        else
        {
            bool gefunden = false;
            qint32 AnzStdFunctions = m_strStandardFunction.length() - 1;
            for (wI = 1; wI <= AnzStdFunctions; wI++)
            {
                wL = m_strStandardFunction.at(wI).length();
                QString strFunktionUpper = m_strFunction.mid(nPosition - 1, wL);
                strFunktionUpper = strFunktionUpper.toUpper();
                QString strDummy(m_strStandardFunction.at(wI));
                strDummy = strDummy.toUpper();
                if (strFunktionUpper == strDummy)
                {
                    gefunden = true;
                    nPosition = nPosition + wL - 1;
                    char_n(nPosition, strCharacter);
                    // ! recursion !!!!!!!!!!!!!!!!!!!!!!
                    f = factor(nPosition, strCharacter);
                    //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                    if (strFunktionUpper == "ABS")
                        f = std::fabs(f);
                    else if (strFunktionUpper == "SQRT")
                        if (f >= 0)
                            f = sqrt(f);
                        else
                            wError = -1;
                    else if (strFunktionUpper == "SINH")
                        f = sinh(f);
                    else if (strFunktionUpper == "COSH")
                        f = cosh(f);
                    else if (strFunktionUpper == "TANH")
                        f = tanh(f);
                    else if (strFunktionUpper == "ARCTAN")
                        f = atan(f);
                    else if (strFunktionUpper == "LN")
                    {
                        if (f >= 0)
                            f = log(f);
                        else
                            wError = -1;
                    }
                    else if (strFunktionUpper == "LOG")
                    {
                        if (f >= 0)
                            f = log10(f);
                        else
                            wError = -1;
                    }
                    else if (strFunktionUpper == "EXP")
                    {
                        //if (f <= 41)
                            f = exp(f);
                        //else
                            //wError = -1;
                    }
                    else if (strFunktionUpper == "SIN")
                        f = sin(f);
                    else if (strFunktionUpper == "COS")
                        f = cos(f);
                    else if (strFunktionUpper == "COT")
                        f = cot(f);
                    else if (strFunktionUpper == "TAN")
                    {
                        if (cos(f) != 0)
                            f = tan(f);
                        else
                            wError = -1;
                    }
                    else if (strFunktionUpper == "ARCSIN")
                    {
                        if (std::fabs(f) < 1.0)
                            f = asin(f);
                        else
                            wError = -1;
                    }
                    else if (strFunktionUpper == "ARCCOS")
                    {
                        if (std::fabs(f) <= 1.0)
                            f = acos(f);
                        else
                            wError = -1;
                    }
                    else if (strFunktionUpper == "SIGN")
                        f = signl(f);                    
                    else if (strFunktionUpper == "RAD")
                        f = RAD(f);
                    else if (strFunktionUpper == "DEG")
                        f = DEG(f);
                    else if (strFunktionUpper == "ARSINH")
                        f = ArSinh(f);                   
                    else if (strFunktionUpper == "ARCOSH")
                    {
                        if (std::fabs(f) >= 1.0)
                            f = ArCosh(f);
                        else
                            wError = -1;
                    }
                    else if (strFunktionUpper == "ARTANH")
                    {
                        if (std::fabs(f) <= 1.0)
                            f = ArTanh(f);
                        else
                            wError = -1;
                    }
                    break;
                }
            }
            if (!gefunden)
            {
                char_n(nPosition, strCharacter);
                if (strCharacterUpper == "A")
                    f = m_dFunctionConstant[0];
                else if (strCharacterUpper == "B")
                    f = m_dFunctionConstant[1];
                else if (strCharacterUpper == "C")
                    f = m_dFunctionConstant[2];
                else if (strCharacterUpper == "D")
                    f = m_dFunctionConstant[3];
                else if (strCharacterUpper == "E")
                    f = m_dFunctionConstant[4];
                else if (strCharacterUpper == "F")
                    f = m_dFunctionConstant[5];
                else if (strCharacterUpper == "G")
                    f = m_dFunctionConstant[6];
                else if (strCharacterUpper == "H")
                    f = m_dFunctionConstant[7];
            }
        }
    }

    if (wError == -1)           errorText = QString("General Parser Error blocked!");

    return f;
}

//*************************************************************************************************************************************

void Formulaeditor::set_funct_const(int index, double val)
{
    //between 0 and 9
    if (index >= 0 && index < 9)   m_dFunctionConstant[index] = val;
    else errorText = QString("Error in SetFunctConst()");
}

QString Formulaeditor::str_char(QString DecimalZahl)
{
    if(DecimalZahl == "?")        return QString("?");
    else        return DecimalZahl;
}

double Formulaeditor::SINQ(double Winkel_grad)
{
    double Winkel_rad = PI * Winkel_grad / 180.0;
    return sin(Winkel_rad);
}

double Formulaeditor::COSQ(double Winkel_grad)
{
  // const float PI = 3.141592654f;
    double Winkel_rad = PI * Winkel_grad / 180.0;
    return cos(Winkel_rad);
}

double Formulaeditor::cot(double x)
{
    return cos(x)/sin(x);
}

double Formulaeditor::DEG(double x /* rad */)
{
    // returns angle in grad
    return x * 180.0 / PI;
}

double Formulaeditor::RAD(double x /* grad */)
{
    // returns angle in rad
    return x * PI / 180.0;
}

QString Formulaeditor::get_next_token(QString &strSrc, const QString strDelim)
{
    QString token;
    int idx = strSrc.indexOf(strDelim);
    if(idx != -1)
    {
        token  = strSrc.left(idx);
        strSrc = strSrc.right(strSrc.length() - (idx + 1) );
    }
    else
    {
        token = strSrc;
        strSrc = "";
    }
    return token;
}

long double Formulaeditor::signl(long double x)
{
    if (x > 0.0L) return 1.0L;
    if (x < 0.0L) return -1.0L;
    return 0.0L;
}

double Formulaeditor::ArSinh(double x)
{
    if (x < 0)
        return -log(-x + sqrt(sqr(-x) + 1));
    return log(x + sqrt(sqr(x) + 1));
}

double Formulaeditor::ArCosh(double x)
{
    return log(x + sqrt(sqr(x) - 1));
}

double Formulaeditor::ArTanh(double x)
{
    return 0.5*logl((1 + x)/ (1 - x));
}

double Formulaeditor::ArCoth(double x)
{
    return 0.5*log((x + 1)/ (x - 1));
}

double Formulaeditor::sqr(double x)
{
    return x*x;
}

//*************************************************************************************************************************************
// end formula methods    Copyright: 2004, Ralf Wirtz   adapted to qt for formula editor by Daniel Knobl ******************************
//*************************************************************************************************************************************

