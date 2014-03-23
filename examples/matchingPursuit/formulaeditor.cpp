//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "formulaeditor.h"
#include "ui_formulaeditor.h"
#include "math.h"
#include "FLOAT.H"
#include "mainwindow.h"
#include <iostream>

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include "QtGui"
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

#define PI 3.14159265358979323846

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

    callAtomPaintWindow = new AtomPaintWindow();

    callAtomPaintWindow->setFixedSize(510,200);
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
    m_strStandardFunction.append("SIN"); //muß in der Reihenfolge nach SINH stehen!!!
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
}

Formulaeditor::~Formulaeditor()
{
    delete ui;
}

void AtomPaintWindow::paintEvent(QPaintEvent *event)
{    
    PaintSignal(atomList, QSize(510,200));
}

void AtomPaintWindow::PaintSignal(QList<qreal> valueList, QSize windowSize)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillRect(0,0,windowSize.width(),windowSize.height(),QBrush(Qt::white));


    if(valueList.length() > 0)
    {
        qint32 borderMarginHeigth = 15;     // verkleinert Zeichenfläche von GraphWindow um borderMargin Pixel in der Höhe
        qint32 borderMarginWidth = 5;       // verkleinert Zeichenfläche von GraphWindow um borderMargin Pixel in der Breite
        qint32 i = 0;                       // Laufindex
        qreal maxNeg = 0;                   // Kleinster Signalwert
        qreal maxPos = 0;                   // groesster Signalwert
        qreal absMin = 0;                   // Minimum der Absolutbetraege von maxNeg und maxPos
        qint32 drawFactor = 0;              // Verschiebungsfaktor für Nachkommastellen (linear)
        qint32 startDrawFactor = 1;         // Verschiebungsfaktor für Nachkommastellen (exponentiell-Basis 10)
        qint32 decimalPlace = 0;            // Nachkommastellen für Achsenbeschriftung        
        QPolygonF polygon;                  // Punkte zum Zeichnen des eingelesenen Signals
        QList<qreal> internListValue;       // interne representation der y-Werte des Signals (nur für grafische Darstellung)

        internListValue.clear();
        while(i < valueList.length())
        {
                internListValue.append(valueList.at(i));            //TODO wie blöd
                i++;
        }

        // Fenster weiss übermalen
        painter.fillRect(0,0,windowSize.width(),windowSize.height(),QBrush(Qt::white));

        // Maximum und Minimum des Signals finden
        i = 0;
        while(i < valueList.length())
        {
            if(valueList.at(i) > maxPos)
                maxPos = valueList.at(i);

            if(valueList.at(i) < maxNeg )
                maxNeg = valueList.at(i);
            i++;
        }

        if(maxPos > fabs(maxNeg)) absMin = maxNeg;      // findet das absolute Minimum der beiden globalen Extremwerte (maxPos, maxNeg)
        else     absMin = maxPos;

        if(absMin != 0)                                 // absMin darf nicht null sein: sonst Endlosschleife
        {
            while(true)                                 // um wieviel muss die Nachkommastelle verschoben werden?
            {
                if(fabs(absMin) < 1)                    // Bei Signalen, bei denen absMin betragsmäßig größer 1 ist, muss keine Nachkommastelle verschoben werden
                {
                    absMin = absMin * 10;
                    drawFactor++;                       // Verschiebungfaktor (zählt die Anzahl der Nachkommastellen um die verschoben wurde
                }
                if(fabs(absMin) >= 1) break;
            }
        }

        // Verschiebung der Nachkommastellen um drawFactor für alle Signalpunkte und anschließende Übernahme in interne Liste
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
        // Absolute Signalhöhe
        if(maxNeg <= 0)     maxmax = maxPos - maxNeg;
        else  maxmax = maxPos + maxNeg;


        // Achsenbeschriftung skalieren        
        qreal scaleYText = (qreal)maxmax / (qreal)10;               // Signalwerte werden in 15tel unterteilt für y-Achsenbeschriftung (16 Werte)
        qint32 negScale =  round(maxNeg * 10 / maxmax);             // Startfaktor für y-Achsenbeschriftung

        //Bestimmen der Länge des Beschriftungstextes der y-Achse für Verschiebung der y-Achse nach rechts (damit Text nicht über Achse geschrieben wird)
        qint32 k = 0;
        qint32 negScale2 = negScale;
        qint32 maxStrLenght = 0;
        while(k < 16)
        {
            QString string2;

            qreal scaledYText = negScale2 * scaleYText / (qreal)startDrawFactor;                                     // Skalenwert Y-Achse
            string2  = QString::number(scaledYText, 'f', decimalPlace + 1);                                          // Skalenwert als String mit richtiger Nachkommastelle (Genauigkeit je nach Signalwertebereich)

            if(string2.length()> maxStrLenght) maxStrLenght = string2.length();

            k++;
            negScale2++;
        }
        maxStrLenght = 6 + maxStrLenght * 6;

        //todo abstand einstellen damit hinten der achsentext mit hinpasst
        //while(((windowSize.width() - maxStrLenght - borderMarginWidth)*100) % 15)  borderMarginWidth++;

        qreal inStartValue = startValue;
        qreal inEndValue = endValue;        

        // Signal skalieren
        qreal scaleX = ((qreal)(windowSize.width() - maxStrLenght - 6 * borderMarginWidth))/ ((qreal)valueList.length() - 1);
        qreal scaleY = (qreal)(windowSize.height() - borderMarginHeigth) / (qreal)maxmax;

        //Achsen skalieren
        qreal scaleXAchse = (qreal)(windowSize.width() - maxStrLenght - 6 * borderMarginWidth) / (qreal)15;
        qreal scaleYAchse = (qreal)(windowSize.height() - borderMarginHeigth) / (qreal)10;

        // Position der Achsbeschriftung der x-Achse
        qint32 xAxisTextPos = 8;
        if(maxNeg == 0) xAxisTextPos = -10; // wenn Signal nur positiv: Beschriftung oberhalb der Achse

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

            qreal scaledYText = negScale * scaleYText / (qreal)startDrawFactor;                                    // Skalenwert Y-Achse

            string  = QString::number(scaledYText, 'f', decimalPlace + 1);                                          // Skalenwert als String mit richtiger Nachkommastelle (Genauigkeit je nach Signalwertebereich)

            if(negScale == 0)                                                                                       // x-Achse erreicht (y-Wert = 0)
            {
                // Eintragen der skalierten Signalpunkte
                qint32 h = 0;
                while(h < valueList.length())
                {
                    polygon.append(QPointF(h * scaleX + maxStrLenght + 10,  -((internListValue.at(h) * scaleY + ((i - 1) * scaleYAchse)-(windowSize.height()) + borderMarginHeigth / 2))));
                    h++;
                }

                // X-Achse zeichnen
                qreal j = 0;
                inStartValue = startValue;                
                while(inStartValue <= inEndValue)
                {
                    QString str;
                    painter.drawText(j * scaleXAchse + maxStrLenght + 3, -(((i - 1) * scaleYAchse)-(windowSize.height())) + xAxisTextPos, str.append(QString::number(inStartValue, 'f', decimalPlaceX)));      // Skalenwert X-Achse
                    painter.drawLine(j * scaleXAchse + maxStrLenght + 10, -(((i - 1) * scaleYAchse)-(windowSize.height() - borderMarginHeigth / 2 - 2)), j * scaleXAchse + maxStrLenght + 10 , -(((i - 1) * scaleYAchse)-(windowSize.height() - borderMarginHeigth / 2 + 2)));   // Anstriche X-Achse
                    j++;
                    inStartValue += internStepWidth;
                }
                painter.drawLine(maxStrLenght, -(((i - 1) * scaleYAchse)-(windowSize.height()) + borderMarginHeigth / 2), windowSize.width()-5, -(((i - 1) * scaleYAchse)-(windowSize.height()) + borderMarginHeigth / 2));
            }

            painter.drawText(3, -((i - 1) * scaleYAchse - windowSize.height()) - borderMarginHeigth/2 + 4, string);     // Skalenwert Y-Achse zeichen
            painter.drawLine(maxStrLenght - 2, -(((i - 1) * scaleYAchse)-(windowSize.height()) + borderMarginHeigth / 2), maxStrLenght + 2, -(((i - 1) * scaleYAchse)-(windowSize.height()) + borderMarginHeigth / 2));  // Anstriche Y-Achse
            i++;
            negScale++;
        }

        painter.drawLine(maxStrLenght, 2, maxStrLenght, windowSize.height() - 2);     // Y-Achse zeichen

        painter.drawPolyline(polygon);                 // Signal zeichen
    }
    painter.end();
}

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


//***************************************************************************************************************
void Formulaeditor::on_btt_Test_clicked()
{
    Formulaeditor FormulaParser;

    FormulaParser.SetFunctConst(0, ui->tb_A->text().toFloat());
    FormulaParser.SetFunctConst(1, ui->tb_B->text().toFloat());
    FormulaParser.SetFunctConst(2, ui->tb_C->text().toFloat());
    FormulaParser.SetFunctConst(3, ui->tb_D->text().toFloat());
    FormulaParser.SetFunctConst(4, ui->tb_E->text().toFloat());
    FormulaParser.SetFunctConst(5, ui->tb_F->text().toFloat());
    FormulaParser.SetFunctConst(6, ui->tb_G->text().toFloat());
    FormulaParser.SetFunctConst(7, ui->tb_H->text().toFloat());

    double retValue = FormulaParser.Calculation(ui->tb_Formula->text(), ui->dsb_StartValue->value());  // TODO Float oder nur Int bei X

    if(errorText.isEmpty())
    {
        if(ui->lb_StartValue->isEnabled())
            ui->btt_Save->setEnabled(true);
        if(retValue < 0.0000000000001 && retValue > -0.0000000000001)
            retValue = 0;
        ui->lb_Result->setText(QString("Ergebnis vom Startwert = %1").arg(retValue));
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
        QMessageBox::warning(this, tr("Fehler"), tr("Die Schrittweite darf nicht null sein."));
        return;
    }

    if(ui->dsb_StartValue->isEnabled() && ui->dsb_EndValue->isEnabled() && ui->dsb_StepWidth->isEnabled())
    {
        while(internStartValue  <= endValue)
        {
            resultsList.append(FormulaParser.Calculation(ui->tb_Formula->text(), internStartValue ));
            internStartValue  += stepWidth;
        }
    }
    else
    {
        qreal result = FormulaParser.Calculation(ui->tb_Formula->text(), internStartValue );
        ui->lb_Result->setText(QString("Ergebnis = %1").arg(result));
    }

    atomList = resultsList;
    atomLength = resultsList.length();
    errorText = "";
    callAtomPaintWindow->update();
    update();
}

// Tritt ein wenn der Button "Speichern" geklickt wird.
void Formulaeditor::on_btt_Save_clicked()
{    
    QFile saveFile("Matching-Pursuit-Toolbox/user.fml");
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

}

//**********************************************************************************************************************
// Formelmethoden ******************************************************************************************************
//**********************************************************************************************************************

qreal Formulaeditor::SignFactor(qint32& nPosition, QString& strCharacter)
{
  if (strCharacter == "-")
    {
        Char_n(nPosition, strCharacter);
        return (-1.0) * Factor(nPosition, strCharacter);
    }
  else return Factor(nPosition, strCharacter);
}

void Formulaeditor::StripFormula(QString &strFormula)
{
    qint32 level = 0;

    if (strFormula.length() < 1) return;

    // Kommas durch Punkte ersetzen, alles runde Klammern
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

    // Einschließende Klammern beseitigen
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
            level = -1; // Markierung
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

    // Führende und abschließende Leerzeichen und positive Vorzeichen entfernen
    while (strFormula.at(0) == '+'
           || strFormula.at(0) == ' ')
    {
        strFormula = strFormula.mid(1);
    }
    strFormula.trimmed();

    //Alle unnötigen Klammern beseitigen
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
                //rechte Klammer
                QString left = strFormula.left(i);
                QString mid = strFormula.mid(i+1);
                strFormula = left + "|" + mid;

                //linke Klammer
                left = strFormula.left(Pos[level] + 1);
                mid = strFormula.mid(Pos[level+1] + 1);
                strFormula = left + "|" + mid;

                j = 0;
            }
        }
    }
    strFormula.replace("|", "");
}

double Formulaeditor::Calculation(QString strFormula, qreal xValue, bool strip)
{
    qint32  nPosition;
    QString strCharacter;
    qreal	ergebnis;

    if (strFormula.length() < 1) return 0.0;

    m_strErrortext = "";

    if (strip) StripFormula(strFormula);

    m_strFunction = strFormula;
    m_dFktValue = xValue;
    if (m_dFktValue == 0)
    m_dFktValue = FLT_MIN;
    nPosition = 0;
    Char_n(nPosition, strCharacter);

    ergebnis = Expression(nPosition, strCharacter);

    return ergebnis;
}

double Formulaeditor::Expression(int& nPosition, QString& strCharacter)
{
  QString strOperator;
  double erg = SimpleExpression(nPosition, strCharacter);
  while (strCharacter == "+" || strCharacter == "-")
  {
    strOperator = strCharacter;
    Char_n(nPosition, strCharacter);
    if (strOperator == "+")
        erg += SimpleExpression(nPosition, strCharacter);
    else if (strOperator == "-")
        erg -= SimpleExpression(nPosition, strCharacter);
  }

  return erg;
}

double Formulaeditor::SimpleExpression(int& nPosition, QString& strCharacter)
{
    double s,dum;
    QString strOperator;
    s = Term(nPosition, strCharacter);
    while (strCharacter == "*" || strCharacter == "/")
    {
        strOperator = strCharacter;
        Char_n(nPosition, strCharacter);
        if (strOperator == "*")
            s = s * Term(nPosition, strCharacter);
        else if (strOperator == "/")
        {
            dum = Term(nPosition, strCharacter);
            if (dum != 0)   s = s / dum;
            else    errorText = QString("Dividieren durch 0 ist nicht möglich.");
        }
    }
    return s;
}

double Formulaeditor::Term(int& nPosition, QString& strCharacter)
{
  qreal t,vz;
  t = SignFactor(nPosition, strCharacter);
  while (strCharacter == "^")
  {
      Char_n(nPosition, strCharacter);
      vz = SignFactor(nPosition, strCharacter);

      if ((t <= 0 && fabs(vz) <= 1) || (t <= 0 && vz != qint32(vz))) errorText = QString("Radizieren negativer Zahlen ist im Reelen nicht möglich.");
      else    t = pow(t,vz);
  }
  return t;
}

double Formulaeditor::Char_n(int& nPosition, QString& strCharacter)
{
    do
    {
        nPosition ++;
        if (nPosition <= m_strFunction.length())
            strCharacter = m_strFunction.mid(nPosition - 1, 1);
        else
            strCharacter = strChar_("?");
    }
    while (strCharacter == " ");

    return nPosition;
}

void Formulaeditor::SetFormula(QString Formula)
{
    m_strFormula = Formula;
}

QString Formulaeditor::GetFormula()
{
    return m_strFormula;
}

double Formulaeditor::Factor(qint32& nPosition, QString& strCharacter)
{
    qreal f = 0.0;
    qint32 wI = 0, wL = 0, wBeginn = 0, wError = 0;

    if	(strCharacter == strChar_(0)) return 0.0;
    // ließt eine Zahl aus und speichert diese als float in der variablen f
    if (((strCharacter >= "0") && (strCharacter <= "9")) || (strCharacter == "."))
    {
        wBeginn = nPosition;

        do
        {
            Char_n(nPosition, strCharacter);
        }
        while ((((strCharacter >= "0") && (strCharacter <= "9")) || (strCharacter == ".")));

        if (strCharacter == ".")
        {
            do
            {
                Char_n(nPosition, strCharacter);
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
            Char_n(nPosition, strCharacter);
            f = Expression(nPosition, strCharacter);
            if (strCharacter == ")")
                Char_n(nPosition, strCharacter);
        }
        else if (strCharacterUpper == "X")
        {
            Char_n(nPosition, strCharacter);
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
                    Char_n(nPosition, strCharacter);
                    // ! Rekursion !!!!!!!!!!!!!!!!!!!!!!
                    f = Factor(nPosition, strCharacter);
                    //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                    if (strFunktionUpper == "ABS")
                        f = fabs(f);
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
                        if (fabs(f) < 1)
                            f = asin(f);
                        else
                            wError = -1;
                    }
                    else if (strFunktionUpper == "ARCCOS")
                    {
                        if (fabs(f) <= 1)
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
                        if (fabs(f) >= 1)
                            f = ArCosh(f);
                        else
                            wError = -1;
                    }
                    else if (strFunktionUpper == "ARTANH")
                    {
                        if (fabs(f) <= 1)
                            f = ArTanh(f);
                        else
                            wError = -1;
                    }
                    break;
                }
            }
            if (!gefunden)
            {
                Char_n(nPosition, strCharacter);
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

void Formulaeditor::SetFunctConst(int index, double val)
{
    //zwischen 0 und 9
    if (index >= 0 && index < 9)   m_dFunctionConstant[index] = val;
    else errorText = QString("Programmfehler in SetFunctConst()");
}

QString Formulaeditor::strChar_(QString DecimalZahl)
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
    // liefert den Winkel eines arithmetischen Ausdrucks im Gradmaß.
    return x * 180.0 / PI;
}

double Formulaeditor::RAD(double x /* grad */)
{
    // liefert das Bogenmaß eines im Gradmaß vorliegenden Winkels.
    return x * PI / 180.0;
}

QString Formulaeditor::GetNextToken(QString &strSrc, const QString strDelim)
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

//**********************************************************************************************************************
//**********************************************************************************************************************
//**********************************************************************************************************************

