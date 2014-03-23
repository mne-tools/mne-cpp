//MATCHING PURSUIT
//=============================================================================================================
/**
* @file     main.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     July, 2012
*
* @section  LICENSE
*
* Copyright (C) 2012, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Example of reading raw data
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <utils/mp/atom.h>
#include "mainwindow.h"
#include "editorwindow.h"
#include "ui_editorwindow.h"
#include "stdio.h"

#include "deletemessagebox.h"
#include "ui_deletemessagebox.h"

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtGui>
#include <QApplication>
#include <QModelIndex>
#include <QMessageBox>

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;

//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS

bool allCombined = false;

qreal linStepWidthScale = 1;
qreal linStepWidthModu = 1;
qreal linStepWidthPhase = 1;
qreal linStepWidthChirp = 1;

qreal expStepWidthScale = 2;
qreal expStepWidthModu = 2;
qreal expStepWidthPhase = 2;
qreal expStepWidthChirp = 02;

qreal startValueScale = 0.05;
qreal startValueModu = 0;
qreal startValuePhase = 0;
qreal startValueChirp = 0;

qreal endValueScale = 0.05;
qreal endValueModu = 0;
qreal endValuePhase = 0;
qreal endValueChirp = 0;

QString partDictName = "";

QList<qreal> scaleList;
QList<qreal> moduList;
QList<qreal> phaseList;
QList<qreal> chirpList;

qint32 atomCount = 1;

Atom::AtomType atomType;

//*************************************************************************************************************
//=============================================================================================================
// MAIN
//=============================================================================================================
EditorWindow::EditorWindow(QWidget *parent) :    QMainWindow(parent),    ui(new Ui::EditorWindow)
{
    this->setAccessibleName("simple");
    ui->setupUi(this);
    readDicts();
}

EditorWindow::~EditorWindow()
{
    delete ui;
}

void EditorWindow::readDicts()
{
    QDir dictDir = QDir("Matching-Pursuit-Toolbox");

    QStringList filterList;
    filterList.append("*.dict");
    filterList.append("*.pdict");

    QFileInfoList fileList =  dictDir.entryInfoList(filterList);    

    for(int i = 0; i < fileList.length(); i++)
    {
        QIcon dictIcon;
        QFileInfo file = fileList.at(i);
        QString tooltip;
        if(QString::compare(file.completeSuffix(), "dict") == 0)
        {
            dictIcon.addFile(":/images/icons/DictIcon.png");
            tooltip = QString("%1.dict").arg(file.baseName());
        }
        else
        {
             dictIcon.addFile(":/images/icons/PartDictIcon.png");
             tooltip = QString("%1.pdict").arg(file.baseName());
        }

        QListWidgetItem *item = new QListWidgetItem;
        item->setToolTip(tooltip);
        item->setIcon(dictIcon);
        item->setText(fileList.at(i).baseName());

        ui->list_AllDict->addItem(item);
    }
    if(ui->list_AllDict->count() > 1)   ui->list_AllDict->itemAt(0, 0)->setSelected(true);
}

// Berechent die Anzahl der Atome wenn Allekombinieren an ist
void EditorWindow::calcAtomCountAllCombined()
{
    qint32 count = 0;
    qint32 scaleCount = 1;
    qint32 moduCount = 1;
    qint32 phaseCount = 1;
    qint32 chirpCount = 1;

    if(scaleList.length() != 0) scaleCount = scaleList.length();
    if(moduList.length() != 0)  moduCount = moduList.length();
    if(phaseList.length() != 0) phaseCount = phaseList.length();
    if(chirpList.length() != 0) chirpCount = chirpList.length();

    if(atomType == Atom::Gauss)
        count = scaleCount  * moduCount * phaseCount;
    else if( atomType == Atom::Chirp)
        count = scaleCount  * moduCount * phaseCount * chirpCount;

    if(count > 1000000)
    {
        QMessageBox::warning(this, tr("Fehler"),
        tr("Die Anzahl der zu berechnenden Atome ist zu groß."));
        return;
    }
    ui->spb_AtomCount->setValue(count);
}

// Berechnung der Parameterwerte mit linearer Schrittweite
QList<qreal> EditorWindow::calcLinPosParameters(qreal startValue, qreal linStepValue)
{
    QList<qreal> resultList;
    qint32 i = 0;
    qreal result = 0;

    i = 0;
    while(i < atomCount)
    {
        result =  startValue + (i * linStepValue);
        resultList.append(result);
        i++;
    }
    return resultList;
}

// Berechnung der Parameterwerte mit linearer Schrittweite (negativ)
QList<qreal> EditorWindow::calcLinNegParameters(qreal startValue, qreal linStepValue)
{
    QList<qreal> resultList;
    qint32 i = 0;
    qreal result = 0;

    i = 0;
    while(i < atomCount)
    {
        result =  startValue - (i * linStepValue);
        resultList.append(result);
        i++;
    }
    return resultList;
}

// Berechnung der Parameterwerte mit expotentieller Schrittweite (positiv)
QList<qreal> EditorWindow::calcExpPosParameters(qreal startValue, qreal expStepValue)
{
    QList<qreal> resultList;
    qint32 i = 0;
    qreal result = 0;

    i = 0;
    while(i < atomCount)
    {
        result =  startValue + pow(i, expStepValue);
        resultList.append(result);
        i++;
    }
    return resultList;
}

// Berechnung der Parameterwerte mit expotentieller Schrittweite (negativ)
QList<qreal> EditorWindow::calcExpNegParameters(qreal startValue, qreal expStepValue)
{
    QList<qreal> resultList;
    qint32 i = 0;
    qreal result = 0;

    i = 0;
    while(i < atomCount)
    {
        result =  startValue - pow(i, expStepValue);
        resultList.append(result);
        i++;
    }
    return resultList;
}

// Berechnet die Skalierungsparamterwerte und speichert sie in eine Liste
QList<qreal> EditorWindow::calcParameterValuesScale(qreal startValue, qreal linStepValue, qreal expStepValue)
{
    QList<qreal> resultList;
    resultList.clear();

    if(ui->rb_NoStepScale->isChecked())
        resultList.append(startValue);
    else if(ui->rb_LinStepScale->isChecked())
    {
        if(ui->rb_PosCountScale->isChecked())   resultList = calcLinPosParameters(startValue, linStepValue);
        else                                    resultList = calcLinNegParameters(startValue, linStepValue);
    }
    else if(ui->rb_ExpStepScale->isChecked())
    {
        if(ui->rb_PosCountScale->isChecked())   resultList = calcExpPosParameters(startValue, expStepValue);
        else                                    resultList = calcExpNegParameters(startValue, expStepValue);
    }
    if(!resultList.isEmpty())   ui->dspb_EndValueScale->setValue(resultList.last());
    return resultList;
}

// Berechnet die Skalierungsparamterwerte und speichert sie in eine Liste (AllCombined)
QList<qreal> EditorWindow::calcAllCombParameterValuesScale(qreal startValue, qreal endvalue, qreal linStepValue, qreal expStepValue)
{
    QList<qreal> resultList;
    resultList.clear();
    qreal temp = endvalue - startValue;
    if(ui->rb_NoStepScale->isChecked())
        resultList.append(startValue);
    else if(ui->rb_LinStepScale->isChecked())
    {
        atomCount = temp / linStepValue + 1;
        resultList = calcLinPosParameters(startValue, linStepValue);
    }
    else if(ui->rb_ExpStepScale->isChecked())
    {
        atomCount = pow(temp, (1/ expStepValue)) + 1;
        resultList = calcExpPosParameters(startValue, expStepValue);
    }
    return resultList;
}

// Berechnet die Modulationsparamterwerte und speichert sie in eine Liste
QList<qreal> EditorWindow::calcParameterValuesModu(qreal startValue, qreal linStepValue, qreal expStepValue)
{
    QList<qreal> resultList;
    resultList.clear();

    if(ui->rb_NoStepModu->isChecked())
        resultList.append(startValue);
    else if(ui->rb_LinStepModu->isChecked())
    {
        if(ui->rb_PosCountModu->isChecked())    resultList = calcLinPosParameters(startValue, linStepValue);
        else                                    resultList = calcLinNegParameters(startValue, linStepValue);
    }
    else if(ui->rb_ExpStepModu->isChecked())
    {
        if(ui->rb_PosCountModu->isChecked())   resultList = calcExpPosParameters(startValue, expStepValue);
        else                                   resultList = calcExpNegParameters(startValue, expStepValue);
    }
    if(!resultList.isEmpty())   ui->dspb_EndValueModu->setValue(resultList.last());
    return resultList;
}

// Berechnet die Modulationsparamterwerte und speichert sie in eine Liste (AllCombined)
QList<qreal> EditorWindow::calcAllCombParameterValuesModu(qreal startValue, qreal endvalue, qreal linStepValue, qreal expStepValue)
{
    QList<qreal> resultList;
    resultList.clear();
    qreal temp = endvalue - startValue;
    if(ui->rb_NoStepModu->isChecked())
        resultList.append(startValue);
    else if(ui->rb_LinStepModu->isChecked())
    {
        atomCount = temp / linStepValue + 1;
        resultList = calcLinPosParameters(startValue, linStepValue);
    }
    if(ui->rb_ExpStepModu->isChecked())
    {        
        atomCount = pow(temp, (1.0/ expStepValue)) + 1;
        resultList = calcExpPosParameters(startValue, expStepValue);
    }
    return resultList;
}

// Berechnet die Phaseparameterwerte und speichert sie in eine Liste
QList<qreal> EditorWindow::calcParameterValuesPhase(qreal startValue, qreal linStepValue, qreal expStepValue)
{
    QList<qreal> resultList;
    resultList.clear();

    if(ui->rb_NoStepPhase->isChecked())
        resultList.append(startValue);
    else if(ui->rb_LinStepPhase->isChecked())
    {
        if(ui->rb_PosCountPhase->isChecked())   resultList = calcLinPosParameters(startValue, linStepValue);
        else                                    resultList = calcLinNegParameters(startValue, linStepValue);
    }
    else if(ui->rb_ExpStepPhase->isChecked())
    {
        if(ui->rb_PosCountPhase->isChecked())   resultList = calcExpPosParameters(startValue, expStepValue);
        else                                    resultList = calcExpNegParameters(startValue, expStepValue);
    }
    if(!resultList.isEmpty())   ui->dspb_EndValuePhase->setValue(resultList.last());
    return resultList;
}

// Berechnet die Phasenparamterwerte und speichert sie in eine Liste (AllCombined)
QList<qreal> EditorWindow::calcAllCombParameterValuesPhase(qreal startValue, qreal endvalue, qreal linStepValue, qreal expStepValue)
{
    QList<qreal> resultList;
    resultList.clear();
    qreal temp = endvalue - startValue;
    if(ui->rb_NoStepPhase->isChecked())
        resultList.append(startValue);
    else if(ui->rb_LinStepPhase->isChecked())
    {
        atomCount = temp / linStepValue + 1;
        resultList = calcLinPosParameters(startValue, linStepValue);
    }
    else if(ui->rb_ExpStepPhase->isChecked())
    {
        atomCount = pow(temp, (1/ expStepValue)) + 1;
        resultList = calcExpPosParameters(startValue, expStepValue);
    }
    return resultList;
}

// Berechnet die Chirpparamterwerte und speichert sie in eine Liste
QList<qreal> EditorWindow::calcParameterValuesChirp(qreal startValue, qreal linStepValue, qreal expStepValue)
{
    QList<qreal> resultList;
    resultList.clear();

    if(ui->rb_NoStepChirp->isChecked())
        resultList.append(startValue);
    else if(ui->rb_LinStepChirp->isChecked())
    {
        if(ui->rb_PosCountChirp->isChecked())   resultList = calcLinPosParameters(startValue, linStepValue);
        else                                    resultList = calcLinNegParameters(startValue, linStepValue);
    }
    else if(ui->rb_ExpStepChirp->isChecked())
    {
        if(ui->rb_PosCountChirp->isChecked())   resultList = calcExpPosParameters(startValue, expStepValue);
        else                                    resultList = calcExpNegParameters(startValue, expStepValue);
    }
    if(!resultList.isEmpty())   ui->dspb_EndValuePhase->setValue(resultList.last());
    return resultList;
}

// Berechnet die Chirpparamterwerte und speichert sie in eine Liste (AllCombined)
QList<qreal> EditorWindow::calcAllCombParameterValuesChirp(qreal startValue, qreal endvalue, qreal linStepValue, qreal expStepValue)
{
    QList<qreal> resultList;
    resultList.clear();
    qreal temp = endvalue - startValue;
    if(ui->rb_NoStepChirp->isChecked())
        resultList.append(startValue);
    else if(ui->rb_LinStepChirp->isChecked())
    {        
        atomCount = temp / linStepValue + 1;
        resultList = calcLinPosParameters(startValue, linStepValue);
    }
    else if(ui->rb_ExpStepChirp->isChecked())
    {
        atomCount = pow(temp, (1/ expStepValue)) + 1;
        resultList = calcExpPosParameters(startValue, expStepValue);
    }
    return resultList;
}

//  Berechnet die Skalierungspparameter
void EditorWindow::calcScaleValue()
{
    if(allCombined)
    {
        ui->dspb_EndValueScale->setDisabled(ui->rb_NoStepScale->isChecked());
        if(ui->rb_LinStepScale->isChecked())        ui->dspb_EndValueScale->setMinimum(startValueScale + linStepWidthScale);
        else if(ui->rb_ExpStepScale->isChecked())  ui->dspb_EndValueScale->setMinimum(startValueScale + 1);
        else
        {
            ui->dspb_EndValueScale->setMinimum(startValueScale);
            ui->dspb_EndValueScale->setValue(startValueScale);
        }    
        scaleList = calcAllCombParameterValuesScale(startValueScale, endValueScale, linStepWidthScale, expStepWidthScale);
        calcAtomCountAllCombined();
    }
    else
    {
        ui->dspb_EndValueScale->setMinimum(0.05);
        scaleList = calcParameterValuesScale(startValueScale, linStepWidthScale, expStepWidthScale);
    }
}

// Berechnet die Modulationsparameter
void EditorWindow::calcModuValue()
{
    if(allCombined)
    {
        ui->dspb_EndValueModu->setDisabled(ui->rb_NoStepModu->isChecked());
        if(ui->rb_LinStepModu->isChecked())        ui->dspb_EndValueModu->setMinimum(startValueModu + linStepWidthModu);
        else if(ui->rb_ExpStepModu->isChecked())  ui->dspb_EndValueModu->setMinimum(startValueModu + 1);
        else
        {
            ui->dspb_EndValueModu->setMinimum(startValueModu);
            ui->dspb_EndValueModu->setValue(startValueModu);
        }
        moduList = calcAllCombParameterValuesModu(startValueModu, endValueModu, linStepWidthModu, expStepWidthModu);
        calcAtomCountAllCombined();
    }
    else
        moduList = calcParameterValuesModu(startValueModu, linStepWidthModu, expStepWidthModu);
}

// Berechnet die Phasenparameter
void EditorWindow::calcPhaseValue()
{
    if(allCombined)
    {
        ui->dspb_EndValuePhase->setDisabled(ui->rb_NoStepPhase->isChecked());
        if(ui->rb_LinStepPhase->isChecked())        ui->dspb_EndValuePhase->setMinimum(startValuePhase + linStepWidthPhase);
        else if(ui->rb_ExpStepPhase->isChecked())  ui->dspb_EndValuePhase->setMinimum(startValuePhase + 1);
        else
        {
            ui->dspb_EndValuePhase->setMinimum(startValuePhase);
            ui->dspb_EndValuePhase->setValue(startValuePhase);
        }
        phaseList = calcAllCombParameterValuesPhase(startValuePhase, endValuePhase, linStepWidthPhase, expStepWidthPhase);
        calcAtomCountAllCombined();
    }
    else
        phaseList = calcParameterValuesPhase(startValuePhase, linStepWidthPhase, expStepWidthPhase);
}

// Berechnet die Chirpparameter
void EditorWindow::calcChirpValue()
{
    if(allCombined)
    {
        ui->dspb_EndValueChirp->setDisabled(ui->rb_NoStepChirp->isChecked());
        if(ui->rb_LinStepChirp->isChecked())        ui->dspb_EndValueChirp->setMinimum(startValueChirp + linStepWidthChirp);
        else if(ui->rb_ExpStepChirp->isChecked())  ui->dspb_EndValueChirp->setMinimum(startValueChirp + 1);
        else
        {
            ui->dspb_EndValueChirp->setMinimum(startValueChirp);
            ui->dspb_EndValueChirp->setValue(startValueChirp);
        }
        chirpList = calcAllCombParameterValuesChirp(startValueChirp, endValueChirp, linStepWidthChirp, expStepWidthChirp);
        calcAtomCountAllCombined();
    }
    else
        chirpList = calcParameterValuesChirp(startValueChirp, linStepWidthChirp, expStepWidthChirp);   
}

// Tritt ein, wenn der Name geändert wird
void EditorWindow::on_tb_PartDictName_editingFinished()
{
    partDictName = ui->tb_PartDictName->text();

    calcScaleValue();
    calcModuValue();
    calcPhaseValue();
    calcChirpValue();
}

// Tritt ein, wenn Alle-Kombinieren geklickt wird
void EditorWindow::on_chb_CombAllPara_toggled(bool checked)
{
    allCombined = checked;
    if(checked)
    {
        ui->spb_AtomCount->setMaximum(1000000);
        ui->spb_AtomCount->setMinimum(0);
        ui->spb_AtomCount->setValue(0);
    }
    else ui->spb_AtomCount->setMinimum(1);

    ui->spb_AtomCount->setDisabled(checked);
    ui->dspb_EndValueScale->setEnabled(true);
    ui->dspb_EndValueModu->setEnabled(true);
    ui->dspb_EndValuePhase->setEnabled(true);
    ui->dspb_EndValueChirp->setEnabled(true);

    ui->dspb_LinStepScale->setEnabled(ui->rb_LinStepScale->isChecked());
    ui->dspb_LinStepModu->setEnabled(ui->rb_LinStepModu->isChecked());
    ui->dspb_LinStepPhase->setEnabled(ui->rb_LinStepPhase->isChecked());
    ui->dspb_LinStepChirp->setEnabled(ui->rb_LinStepChirp->isChecked());

    ui->dspb_ExpStepScale->setEnabled(ui->rb_ExpStepScale->isChecked());
    ui->dspb_ExpStepModu->setEnabled(ui->rb_ExpStepModu->isChecked());
    ui->dspb_ExpStepPhase->setEnabled(ui->rb_ExpStepPhase->isChecked());
    ui->dspb_ExpStepChirp->setEnabled(ui->rb_ExpStepChirp->isChecked());

    ui->lb_LinNScale->setEnabled(ui->rb_LinStepScale->isChecked());
    ui->lb_LinNModu->setEnabled(ui->rb_LinStepModu->isChecked());
    ui->lb_LinNPhase->setEnabled(ui->rb_LinStepPhase->isChecked());
    ui->lb_LinNChirp->setEnabled(ui->rb_LinStepChirp->isChecked());

    ui->lb_ExpNScale->setEnabled(ui->rb_ExpStepScale->isChecked());
    ui->lb_ExpNModu->setEnabled(ui->rb_ExpStepModu->isChecked());
    ui->lb_ExpNPhase->setEnabled(ui->rb_ExpStepPhase->isChecked());
    ui->lb_ExpNChirp->setEnabled(ui->rb_ExpStepChirp->isChecked());

    ui->lb_EndValueScale->setDisabled(ui->rb_NoStepScale->isChecked());
    ui->lb_EndValueModu->setDisabled(ui->rb_NoStepModu->isChecked());
    ui->lb_EndValuePhase->setDisabled(ui->rb_NoStepPhase->isChecked());
    ui->lb_EndValueChirp->setDisabled(ui->rb_NoStepChirp->isChecked());

    ui->lb_CountDirectionScale->setDisabled(true);
    ui->lb_CountDirectionModu->setDisabled(true);
    ui->lb_CountDirectionPhase->setDisabled(true);
    ui->lb_CountDirectionChirp->setDisabled(true);

    ui->fr_CountDirectionScale->setDisabled(true);
    ui->fr_CountDirectionModu->setDisabled(true);
    ui->fr_CountDirectionPhase->setDisabled(true);
    ui->fr_CountDirectionChirp->setDisabled(true);

    ui->dspb_EndValueScale->setDisabled(ui->rb_NoStepScale->isChecked());
    ui->dspb_EndValueModu->setDisabled(ui->rb_NoStepModu->isChecked());
    ui->dspb_EndValuePhase->setDisabled(ui->rb_NoStepPhase->isChecked());
    ui->dspb_EndValueChirp->setDisabled(ui->rb_NoStepChirp->isChecked());

    if(ui->rb_LinStepScale->isChecked())   ui->dspb_EndValueScale->setMinimum(startValueScale + linStepWidthScale);
    else if(ui->rb_ExpStepScale->isChecked())   ui->dspb_EndValueScale->setMinimum(startValueScale + 1);
    else ui->dspb_EndValueScale->setValue(startValueScale);

    if(ui->rb_LinStepModu->isChecked())   ui->dspb_EndValueModu->setMinimum(startValueModu + linStepWidthModu);
    else if(ui->rb_ExpStepModu->isChecked())   ui->dspb_EndValueModu->setMinimum(startValueModu + 1);
    else ui->dspb_EndValueModu->setValue(startValueModu);

    if(ui->rb_LinStepPhase->isChecked())   ui->dspb_EndValuePhase->setMinimum(startValuePhase + linStepWidthPhase);
    else if(ui->rb_ExpStepPhase->isChecked())   ui->dspb_EndValuePhase->setMinimum(startValuePhase + 1);
    else ui->dspb_EndValuePhase->setValue(startValuePhase);

    if(ui->rb_LinStepChirp->isChecked())   ui->dspb_EndValueChirp->setMinimum(startValueChirp + linStepWidthChirp);
    else if(ui->rb_ExpStepChirp->isChecked())   ui->dspb_EndValueChirp->setMinimum(startValueChirp + 1);
    else ui->dspb_EndValueChirp->setValue(startValueChirp);

    if(!checked)
    {
        ui->dspb_EndValueScale->setMinimum(0.05);
        ui->dspb_EndValueModu->setMinimum(0.00);
        ui->dspb_EndValuePhase->setMinimum(0.00);
        ui->dspb_EndValueScale->setMinimum(0.00);

        ui->dspb_EndValueScale->setDisabled(true);
        ui->dspb_EndValueModu->setDisabled(true);
        ui->dspb_EndValuePhase->setDisabled(true);
        ui->dspb_EndValueChirp->setDisabled(true);

        ui->lb_CountDirectionScale->setEnabled(!ui->rb_NoStepScale->isChecked());
        ui->lb_CountDirectionModu->setEnabled(!ui->rb_NoStepModu->isChecked());
        ui->lb_CountDirectionPhase->setEnabled(!ui->rb_NoStepPhase->isChecked());
        ui->lb_CountDirectionChirp->setEnabled(!ui->rb_NoStepChirp->isChecked());

        ui->fr_CountDirectionScale->setEnabled(!ui->rb_NoStepScale->isChecked());
        ui->fr_CountDirectionModu->setEnabled(!ui->rb_NoStepModu->isChecked());
        ui->fr_CountDirectionPhase->setEnabled(!ui->rb_NoStepPhase->isChecked());
        ui->fr_CountDirectionChirp->setEnabled(!ui->rb_NoStepChirp->isChecked());

        if(ui->spb_AtomCount->value() == 1)
        {
            ui->spb_AtomCount->setValue(2);
            ui->spb_AtomCount->setValue(1);
        }
    }

    calcScaleValue();
    calcModuValue();
    calcPhaseValue();
    calcChirpValue();
}


void EditorWindow::on_spb_AtomLength_editingFinished()
{
    // Setzt den maximalen Startwert für die Skalierung
    ui->dspb_StartValueScale->setMaximum(10 * ui->spb_AtomLength->value());
    startValueScale = ui->dspb_StartValueScale->value();

    calcScaleValue();
    calcModuValue();
    calcPhaseValue();
    calcChirpValue();

}

// Anzahl der Atome einstellen (und Endwerte neuberechnen)
void EditorWindow::on_spb_AtomCount_valueChanged(int arg1)
{
    atomCount = ui->spb_AtomCount->value();

    bool oneAtom = true;
    if(atomCount != 1 || (atomCount == 1 && allCombined)) oneAtom = false;

    ui->rb_NoStepScale->setChecked(oneAtom);
    ui->rb_NoStepModu->setChecked(oneAtom);
    ui->rb_NoStepPhase->setChecked(oneAtom);
    ui->rb_NoStepChirp->setChecked(oneAtom);

    ui->rb_NoStepScale->setDisabled(oneAtom);
    ui->rb_NoStepModu->setDisabled(oneAtom);
    ui->rb_NoStepPhase->setDisabled(oneAtom);
    ui->rb_NoStepChirp->setDisabled(oneAtom);

    ui->lb_StepDefScale ->setDisabled(oneAtom);
    ui->lb_StepDefModu ->setDisabled(oneAtom);
    ui->lb_StepDefPhase ->setDisabled(oneAtom);
    ui->lb_StepDefChirp ->setDisabled(oneAtom);

    ui->rb_LinStepScale->setDisabled(oneAtom);
    ui->rb_LinStepModu->setDisabled(oneAtom);
    ui->rb_LinStepPhase->setDisabled(oneAtom);
    ui->rb_LinStepChirp->setDisabled(oneAtom);

    ui->rb_ExpStepScale->setDisabled(oneAtom);
    ui->rb_ExpStepModu->setDisabled(oneAtom);
    ui->rb_ExpStepPhase->setDisabled(oneAtom);
    ui->rb_ExpStepChirp->setDisabled(oneAtom);

    ui->lb_LinNScale->setDisabled(oneAtom);
    ui->lb_LinNModu->setDisabled(oneAtom);
    ui->lb_LinNPhase->setDisabled(oneAtom);
    ui->lb_LinNChirp->setDisabled(oneAtom);

    ui->lb_ExpNScale->setDisabled(oneAtom);
    ui->lb_ExpNModu->setDisabled(oneAtom);
    ui->lb_ExpNPhase->setDisabled(oneAtom);
    ui->lb_ExpNChirp->setDisabled(oneAtom);

    ui->dspb_LinStepScale->setDisabled(oneAtom);
    ui->dspb_LinStepModu->setDisabled(oneAtom);
    ui->dspb_LinStepPhase->setDisabled(oneAtom);
    ui->dspb_LinStepChirp->setDisabled(oneAtom);

    ui->dspb_ExpStepScale->setDisabled(oneAtom);
    ui->dspb_ExpStepModu->setDisabled(oneAtom);
    ui->dspb_ExpStepPhase->setDisabled(oneAtom);
    ui->dspb_ExpStepChirp->setDisabled(oneAtom);

    if(oneAtom)
    {
        ui->dspb_EndValueScale->setValue(startValueScale);
        ui->dspb_EndValueModu->setValue(startValueModu);
        ui->dspb_EndValuePhase->setValue(startValuePhase);
        ui->dspb_EndValueChirp->setValue(startValueChirp);

        ui->lb_EndValueScale->setDisabled(true);
        ui->lb_EndValueModu->setDisabled(true);
        ui->lb_EndValuePhase->setDisabled(true);
        ui->lb_EndValueChirp->setDisabled(true);
    }
    else
    {
        ui->lb_EndValueScale->setDisabled(ui->rb_NoStepScale->isChecked());
        ui->lb_EndValueModu->setDisabled(ui->rb_NoStepModu->isChecked());
        ui->lb_EndValuePhase->setDisabled(ui->rb_NoStepPhase->isChecked());
        ui->lb_EndValueChirp->setDisabled(ui->rb_NoStepChirp->isChecked());

        ui->dspb_LinStepScale->setEnabled(ui->rb_LinStepScale->isChecked());
        ui->dspb_LinStepModu->setEnabled(ui->rb_LinStepModu->isChecked());
        ui->dspb_LinStepPhase->setEnabled(ui->rb_LinStepPhase->isChecked());
        ui->dspb_LinStepChirp->setEnabled(ui->rb_LinStepChirp->isChecked());

        ui->dspb_ExpStepScale->setEnabled(ui->rb_ExpStepScale->isChecked());
        ui->dspb_ExpStepModu->setEnabled(ui->rb_ExpStepModu->isChecked());
        ui->dspb_ExpStepPhase->setEnabled(ui->rb_ExpStepPhase->isChecked());
        ui->dspb_ExpStepChirp->setEnabled(ui->rb_ExpStepChirp->isChecked());

        ui->lb_LinNScale->setEnabled(ui->rb_LinStepScale->isChecked());
        ui->lb_LinNModu->setEnabled(ui->rb_LinStepModu->isChecked());
        ui->lb_LinNPhase->setEnabled(ui->rb_LinStepPhase->isChecked());
        ui->lb_LinNChirp->setEnabled(ui->rb_LinStepChirp->isChecked());

        ui->lb_ExpNScale->setEnabled(ui->rb_ExpStepScale->isChecked());
        ui->lb_ExpNModu->setEnabled(ui->rb_ExpStepModu->isChecked());
        ui->lb_ExpNPhase->setEnabled(ui->rb_ExpStepPhase->isChecked());
        ui->lb_ExpNChirp->setEnabled(ui->rb_ExpStepChirp->isChecked());
    }

    calcScaleValue();
    calcModuValue();
    calcPhaseValue();
    calcChirpValue();
}

// Parameter: Skalierung
void EditorWindow::on_dspb_StartValueScale_editingFinished()
{
    startValueScale = ui->dspb_StartValueScale->value();
    calcScaleValue();
}

void EditorWindow::on_dspb_EndValueScale_editingFinished()
{
    endValueScale = ui->dspb_EndValueScale->value();
    calcScaleValue();
}

void EditorWindow::on_rb_NoStepScale_toggled(bool checked)
{
    if(checked) ui->lb_StartValueScale->setText("Wert:");
    else        ui->lb_StartValueScale->setText("Startwert:");
    ui->lb_EndValueScale->setDisabled(checked);
    if(!checked)
    {
        ui->lb_CountDirectionScale->setDisabled(allCombined);
        ui->fr_CountDirectionScale->setDisabled(allCombined);
    }
    else
    {
        ui->lb_CountDirectionScale->setDisabled(true);
        ui->fr_CountDirectionScale->setDisabled(true);
    }
    calcScaleValue();
}

void EditorWindow::on_rb_LinStepScale_toggled(bool checked)
{
    if(checked) linStepWidthScale = ui->dspb_LinStepScale->value();
    calcScaleValue();
}

void EditorWindow::on_rb_ExpStepScale_toggled(bool checked)
{
    if(checked) expStepWidthScale = ui->dspb_ExpStepScale->value();
    calcScaleValue();
}

void EditorWindow::on_dspb_LinStepScale_editingFinished()
{
    linStepWidthScale = ui->dspb_LinStepScale->value();
    calcScaleValue();
}

void EditorWindow::on_dspb_ExpStepScale_editingFinished()
{
    expStepWidthScale = ui->dspb_ExpStepScale->value();
    calcScaleValue();
}

void EditorWindow::on_rb_PosCountScale_toggled()
{
    if(!allCombined)
        scaleList = calcParameterValuesScale(startValueScale, linStepWidthScale, expStepWidthScale);
}

void EditorWindow::on_rb_NegCountScale_toggled()
{
    if(!allCombined)
        scaleList = calcParameterValuesScale(startValueScale, linStepWidthScale, expStepWidthScale);
}

// Parameter: Modulation
void EditorWindow::on_dspb_StartValueModu_editingFinished()
{
    startValueModu = ui->dspb_StartValueModu->value();
    calcModuValue();
}

void EditorWindow::on_dspb_EndValueModu_editingFinished()
{
    endValueModu = ui->dspb_EndValueModu->value();
    calcModuValue();
}

void EditorWindow::on_rb_NoStepModu_toggled(bool checked)
{
    if(checked) ui->lb_StartValueModu->setText("Wert:");
    else        ui->lb_StartValueModu->setText("Startwert:");
    ui->lb_EndValueModu->setDisabled(checked);
    if(!checked)
    {
        ui->lb_CountDirectionModu->setDisabled(allCombined);
        ui->fr_CountDirectionModu->setDisabled(allCombined);
    }
    else
    {
        ui->lb_CountDirectionModu->setDisabled(true);
        ui->fr_CountDirectionModu->setDisabled(true);
    }
    calcModuValue();
}

void EditorWindow::on_rb_LinStepModu_toggled(bool checked)
{
    if(checked) linStepWidthModu = ui->dspb_LinStepModu->value();
    calcModuValue();
}

void EditorWindow::on_rb_ExpStepModu_toggled(bool checked)
{
    if(checked) expStepWidthModu = ui->dspb_ExpStepModu->value();
    calcModuValue();
}

void EditorWindow::on_dspb_LinStepModu_editingFinished()
{
    linStepWidthModu = ui->dspb_LinStepModu->value();
    calcModuValue();
}

void EditorWindow::on_dspb_ExpStepModu_editingFinished()
{
    expStepWidthModu = ui->dspb_ExpStepModu->value();
    calcModuValue();
}

void EditorWindow::on_rb_PosCountModu_toggled()
{
    if(allCombined)
        moduList = calcParameterValuesModu(startValueModu, linStepWidthModu, expStepWidthModu);
}

void EditorWindow::on_rb_NegCountModu_toggled()
{
    if(allCombined)
        moduList = calcParameterValuesModu(startValueModu, linStepWidthModu, expStepWidthModu);
}

// Parameter: Phase
void EditorWindow::on_dspb_StartValuePhase_editingFinished()
{
    startValuePhase = ui->dspb_StartValuePhase->value();
    calcPhaseValue();
}

void EditorWindow::on_dspb_EndValuePhase_editingFinished()
{
    endValuePhase = ui->dspb_EndValuePhase->value();
    calcPhaseValue();
}

void EditorWindow::on_rb_NoStepPhase_toggled(bool checked)
{
    if(checked) ui->lb_StartValuePhase->setText("Wert:");
    else        ui->lb_StartValuePhase->setText("Startwert:");
    ui->lb_EndValuePhase->setDisabled(checked);
    if(!checked)
    {
        ui->lb_CountDirectionPhase->setDisabled(allCombined);
        ui->fr_CountDirectionPhase->setDisabled(allCombined);
    }
    else
    {
        ui->lb_CountDirectionPhase->setDisabled(true);
        ui->fr_CountDirectionPhase->setDisabled(true);
    }
    calcPhaseValue();
}

void EditorWindow::on_rb_LinStepPhase_toggled(bool checked)
{
    if(checked) linStepWidthPhase = ui->dspb_LinStepPhase->value();
    calcPhaseValue();
}

void EditorWindow::on_rb_ExpStepPhase_toggled(bool checked)
{
    if(checked) expStepWidthPhase = ui->dspb_ExpStepPhase->value();
    calcPhaseValue();
}

void EditorWindow::on_dspb_LinStepPhase_editingFinished()
{
    linStepWidthPhase = ui->dspb_LinStepPhase->value();
    calcPhaseValue();
}

void EditorWindow::on_dspb_ExpStepPhase_editingFinished()
{
    expStepWidthPhase = ui->dspb_ExpStepPhase->value();
    calcPhaseValue();
}

void EditorWindow::on_rb_PosCountPhase_toggled()
{
    if(allCombined)
        phaseList = calcParameterValuesPhase(startValuePhase, linStepWidthPhase, expStepWidthPhase);
}

void EditorWindow::on_rb_NegCountPhase_toggled()
{
    if(allCombined)
        phaseList = calcParameterValuesPhase(startValuePhase, linStepWidthPhase, expStepWidthPhase);
}

// Parameter: Chirp
void EditorWindow::on_dspb_StartValueChirp_editingFinished()
{
    startValueChirp = ui->dspb_StartValueChirp->value();
    calcChirpValue();
}

void EditorWindow::on_dspb_EndValueChirp_editingFinished()
{
    endValueChirp = ui->dspb_EndValueChirp->value();
    calcChirpValue();
}

void EditorWindow::on_rb_NoStepChirp_toggled(bool checked)
{
    if(checked) ui->lb_StartValueChirp->setText("Wert:");
    else        ui->lb_StartValueChirp->setText("Startwert:");
    ui->lb_EndValueChirp->setDisabled(checked);
    if(!checked)
    {
        ui->lb_CountDirectionChirp->setDisabled(allCombined);
        ui->fr_CountDirectionChirp->setDisabled(allCombined);
        ui->rb_NegCountChirp->setDisabled(allCombined);
        ui->rb_PosCountChirp->setDisabled(allCombined);
    }
    else
    {
        ui->lb_CountDirectionChirp->setDisabled(true);
        ui->fr_CountDirectionChirp->setDisabled(true);
        ui->rb_NegCountChirp->setDisabled(true);
        ui->rb_PosCountChirp->setDisabled(true);
    }
    calcChirpValue();
}

void EditorWindow::on_rb_LinStepChirp_toggled(bool checked)
{
     if(checked) linStepWidthChirp = ui->dspb_LinStepChirp->value();
     calcChirpValue();
}

void EditorWindow::on_rb_ExpStepChirp_toggled(bool checked)
{
    if(checked) expStepWidthChirp  = ui->dspb_ExpStepChirp->value();
    calcChirpValue();
}

void EditorWindow::on_dspb_LinStepChirp_editingFinished()
{
    linStepWidthChirp = ui->dspb_LinStepChirp->value();
    calcChirpValue();
}

void EditorWindow::on_dspb_ExpStepChirp_editingFinished()
{
    expStepWidthChirp = ui->dspb_ExpStepChirp->value();
    calcChirpValue();
}

void EditorWindow::on_rb_PosCountChirp_toggled()
{
    if(allCombined)
        chirpList = calcParameterValuesChirp(startValueChirp, linStepWidthChirp, expStepWidthChirp);
}

void EditorWindow::on_rb_NegCountChirp_toggled()
{
    if(allCombined)
        chirpList = calcParameterValuesChirp(startValueChirp, linStepWidthChirp, expStepWidthChirp);
}

// Legt fest ob es sich um ein Gauss- oder Chirp- Atom handelt
void EditorWindow::on_rb_GaussAtomType_toggled(bool checked)
{
    if(checked) atomType = Atom::Gauss;
}

void EditorWindow::on_rb_ChirpAtomType_toggled(bool checked)
{
    if(checked) atomType = Atom::Chirp;

    if(checked && ui->spb_AtomCount->value() == 1 && !allCombined)
    {
        ui->lb_StepDefChirp->setDisabled(true);
        ui->rb_NoStepChirp->setDisabled(true);
        ui->rb_LinStepChirp->setDisabled(true);
        ui->lb_LinNChirp->setDisabled(true);
        ui->dspb_LinStepChirp->setDisabled(true);
        ui->rb_ExpStepChirp->setDisabled(true);
        ui->lb_ExpNChirp->setDisabled(true);
        ui->dspb_ExpStepChirp->setDisabled(true);

        ui->lb_CountDirectionChirp->setDisabled(true);
        ui->rb_PosCountChirp->setDisabled(true);
        ui->rb_NegCountChirp->setDisabled(true);
        ui->lb_EndValueChirp->setDisabled(true);
        ui->dspb_EndValueChirp->setDisabled(true);

        //ui->dspb_EndValueChirp->s
    }
    else if(checked)
    {
        ui->lb_EndValueChirp->setDisabled(ui->rb_NoStepChirp->isChecked());
        ui->rb_PosCountChirp->setDisabled(ui->rb_NoStepChirp->isChecked());
        ui->rb_NegCountChirp->setDisabled(ui->rb_NoStepChirp->isChecked());
        ui->lb_EndValueChirp->setDisabled(!allCombined);
    }
}


// Berechnet alle Atome mit den eingestellten Parameter und speichert diese in einer Liste (und auf Platte)
void EditorWindow::on_btt_CalcAtoms_clicked()
{
    Atom *atom = new Atom();
    QStringList resultList;
    resultList.clear();

    QString savePath = QString(":/Matching-Pursuit-Toolbox/%1.pdict").arg(partDictName);
    QFile dict(savePath);

    if(partDictName.isEmpty())
    {
        QMessageBox::warning(this, tr("Fehler"),
        tr("Es wurde kein Name vergeben."));
        ui->tb_PartDictName->setFocus();
        return;
    }

    qint32 k = 0;
    while(k < ui->list_AllDict->count())
    {
        if(QString::compare(partDictName, ui->list_AllDict->item(k)->text()) == 0)
        {       QMessageBox::warning(this, tr("Fehler"),
                tr("Der Name ist schon vergeben."));

                ui->tb_PartDictName->setFocus();
                ui->tb_PartDictName->selectAll();
                return;
        }
        k++;
    }

    if (dict.open (QIODevice::WriteOnly| QIODevice::Append))
    {
        QTextStream stream( &dict );
        stream << QString("atomcount = %1 ").arg(atomCount)  << "\n";

        if(ui->chb_CombAllPara->isChecked())
        {
            qint32 atomIndex = 0;
            qint32 chirpCount = 0;
            qint32 chirpMax = 1;

            if(chirpList.length() != 0) chirpMax = chirpList.length();

            while(chirpCount < chirpMax)
            {
                qint32 phaseCount = 0;
                while(phaseCount < phaseList.length())
                {
                    qint32 moduCount = 0;
                    while(moduCount < moduList.length())
                    {
                        qint32 scaleCount = 0;
                        while(scaleCount < scaleList.length())
                        {
                            qreal tempScale = 0;
                            if(scaleList.length() == 1)    tempScale = scaleList.at(0);
                            else                           tempScale = scaleList.at(scaleCount);

                            qreal tempModu = 0;
                            if(moduList.length() ==1)      tempModu = moduList.at(0);
                            else                           tempModu = moduList.at(moduCount);

                            qreal tempPhase = 0;
                            if(phaseList.length() == 1)    tempPhase = phaseList.at(0);
                            else                           tempPhase = phaseList.at(phaseCount);

                            qreal tempChirp = 0;
                            if(chirpList.length() == 1)    tempChirp = chirpList.at(0);
                            else                           tempChirp = chirpList.at(chirpCount);

                            resultList = atom->CreateStringValues(ui->spb_AtomLength->value(), tempScale , tempModu, tempPhase, tempChirp, atomType);

                            stream << QString("%1_ATOM_%2 \n scale: %3 modu: %4 phase: %5 chrip: %6").arg(partDictName).arg(atomIndex).arg(tempScale).arg(tempModu).arg(tempPhase).arg(tempChirp) << "\n";
                            for (QStringList::Iterator it = resultList.begin(); it != resultList.end(); it++)
                                stream << *it << "\n";

                            atomIndex++;
                            scaleCount++;
                        }
                        moduCount++;
                    }
                    phaseCount++;
                }
                chirpCount++;
            }            
        }
        else
        {
            if(ui->spb_AtomCount->value() != 1)
            {
                if(ui->rb_ChirpAtomType->isChecked())
                {
                    if(ui->rb_NoStepScale->isChecked() && ui->rb_NoStepModu->isChecked() && ui->rb_NoStepPhase->isChecked() && ui->rb_NoStepChirp)
                    {
                        QMessageBox::warning(this, tr("Warnung"),QString("Es werden %1 identische  Atome erstellt. Bitte ändern Sie die Atomanzahl auf 1 oder lassen Sie einen Parameter variieren.").arg(ui->spb_AtomCount->value()));
                        return;
                    }
                }
                else if(ui->rb_NoStepScale->isChecked() && ui->rb_NoStepModu->isChecked() && ui->rb_NoStepPhase->isChecked())
                {
                    QMessageBox::warning(this, tr("Warnung"),QString("Es werden %1 identische  Atome erstellt. Bitte ändern Sie die Atomanzahl auf 1 oder lassen Sie einen Parameter variieren.").arg(ui->spb_AtomCount->value()));
                    return;
                }
            }

            qint32 i = 0;
            while (i < atomCount)
            {
                qreal tempScale = 0;
                if(scaleList.length() == 1)    tempScale = scaleList.at(0);
                else                           tempScale = scaleList.at(i);

                qreal tempModu = 0;
                if(moduList.length() ==1)      tempModu = moduList.at(0);
                else                           tempModu = moduList.at(i);

                qreal tempPhase = 0;
                if(phaseList.length() == 1)    tempPhase = phaseList.at(0);
                else                           tempPhase = phaseList.at(i);

                qreal tempChirp = 0;
                if(chirpList.length() == 1)    tempChirp = chirpList.at(0);
                else                           tempChirp = chirpList.at(i);

                resultList = atom->CreateStringValues(ui->spb_AtomLength->value(), tempScale , tempModu, tempPhase, tempChirp, atomType);

                stream << QString("%1_ATOM_%2 \n scale: %3 modu: %4 phase: %5 chrip: %6").arg(partDictName).arg(i).arg(tempScale).arg(tempModu).arg(tempPhase).arg(tempChirp)  << "\n";
                for (QStringList::Iterator it = resultList.begin(); it != resultList.end(); it++)
                    stream << *it << "\n";

                i++;
            }            
        }
        dict.close();
    }

    QListWidgetItem *item = new QListWidgetItem;
    item->setToolTip(QString("%1.pdict").arg(partDictName));
    item->setIcon(QIcon(":images/icons/PartDictIcon.png"));
    item->setText(partDictName);

    ui->list_AllDict->addItem(item);
    if(ui->list_AllDict->count() > 1)   ui->list_AllDict->itemAt(0, 0)->setSelected(true);
}

void EditorWindow::on_btt_ToNewDict_clicked()
{
    QList<QListWidgetItem*> selcItems = ui->list_AllDict->selectedItems();
    for(qint32 i = 0; i < selcItems.length(); i++)
    {
        QListWidgetItem* item = selcItems.at(i);
        ui->list_NewDict->addItem(item->clone());

        item->setSelected(false);
        delete item;
    }
    if(ui->list_AllDict->count() > 0)   ui->list_AllDict->itemAt(0,0)->setSelected(true);
    if(ui->list_NewDict->count() > 0)   ui->btt_SaveDicts->setEnabled(true);
}

void EditorWindow::on_list_AllDict_doubleClicked()
{
    QListWidgetItem* item = ui->list_AllDict->selectedItems().at(0);
    ui->list_NewDict->addItem(item->clone());

    item->setSelected(false);
    delete item;
    if(ui->list_AllDict->count() > 0)   ui->list_AllDict->itemAt(0,0)->setSelected(true);
    if(ui->list_NewDict->count() > 0)   ui->btt_SaveDicts->setEnabled(true);

}

void EditorWindow::on_btt_ToAlldict_clicked()
{
    QList<QListWidgetItem*> selcItems = ui->list_NewDict->selectedItems();
    for(qint32 i = 0; i < selcItems.length(); i++)
    {
        QListWidgetItem* item = selcItems.at(i);
        ui->list_AllDict->addItem(item->clone());

        item->setSelected(false);
        delete item;
    }
    if(ui->list_NewDict->count() > 1)    ui->list_NewDict->itemAt(0,0)->setSelected(true);
    if(ui->list_NewDict->count() == 0)   ui->btt_SaveDicts->setEnabled(false);
}

void EditorWindow::on_list_NewDict_doubleClicked()
{
    QListWidgetItem* item = ui->list_NewDict->selectedItems().at(0);
    ui->list_AllDict->addItem(item->clone());

    item->setSelected(false);
    delete item;
    if(ui->list_NewDict->count() > 1)        ui->list_NewDict->itemAt(0,0)->setSelected(true);
    if(ui->list_NewDict->count() == 0)   ui->btt_SaveDicts->setEnabled(false);
}

void EditorWindow::on_btt_DeleteDict_clicked()
{
    QFile configFile("Matching-Pursuit-Toolbox/Matching-Pursuit-Toolbox.config");
    bool showMsgBox = true;
    QString contents;
    if (configFile.open(QIODevice::ReadWrite | QIODevice::Text))
    {
        while(!configFile.atEnd())
        {
            contents = configFile.readLine(0).constData();
            if(QString::compare("ShowDeleteMessageBox=true;\n", contents) == 0)
                showMsgBox = true;
        }
    }
    configFile.close();

    if(showMsgBox)
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

    qint32 i = 0;
    QList<QListWidgetItem*> delItemsList = ui->list_AllDict->selectedItems();
    while( i < delItemsList.length())
    {
        QFile file(QString("Matching-Pursuit-Toolbox/%1").arg(delItemsList.at(i)->toolTip()));
        file.remove();

        QListWidgetItem* delItem = delItemsList.at(i);
        delItem->setSelected(false);
        delete delItem;
        i++;
    }
}

void EditorWindow::on_list_AllDict_itemSelectionChanged()
{
    if(ui->list_AllDict->selectedItems().length() == 0)
    {
        ui->btt_ToNewDict->setEnabled(false);
        ui->btt_DeleteDict->setEnabled(false);
    }
    else
    {
        ui->btt_ToNewDict->setEnabled(true);
        ui->btt_DeleteDict->setEnabled(true);
    }
}

void EditorWindow::on_list_NewDict_itemSelectionChanged()
{
    if(ui->list_NewDict->selectedItems().length() == 0)
    {
         ui->btt_ToAlldict->setEnabled(false);
    }
    else
    {
         ui->btt_ToAlldict->setEnabled(true);
    }
}

void EditorWindow::on_btt_SaveDicts_clicked()
{
    QList<qint32> atomCountList;

    if(ui->tb_DictName->text().isEmpty())
    {
        QMessageBox::warning(this, tr("Fehler"),
        tr("Es wurde kein Name für das Wörterbuch vergeben."));
        ui->tb_DictName->setFocus();
        return;
    }

    QDir dictDir = QDir("Matching-Pursuit-Toolbox");

    QStringList filterList;
    filterList.append("*.dict");

    QFileInfoList fileList =  dictDir.entryInfoList(filterList);

    for(qint32 i = 0; i < fileList.length(); i++)
    {
        QFileInfo fileInfo = fileList.at(i);
        if(QString::compare(fileInfo.baseName(), ui->tb_DictName->text()) == 0)
        {
                QMessageBox::warning(this, tr("Fehler"),
                tr("Der Name für das Wörterbuch ist schon vergeben."));
                ui->tb_DictName->setFocus();
                ui->tb_DictName->selectAll();
                return;
        }
    }

    QString newpath = QString("Matching-Pursuit-Toolbox/%1.dict").arg(ui->tb_DictName->text());
    QFile newFile(newpath);
    if(!newFile.exists())
    {
        if (newFile.open(QIODevice::ReadWrite | QIODevice::Text))
        newFile.close();
    }

    if (newFile.open (QIODevice::WriteOnly| QIODevice::Append))
    {
        QTextStream stream(&newFile);
        stream << "/n";
        for(qint32 i = 0; i < ui->list_NewDict->count(); i++)
        {
            QString contents;
            QFile file(QString("Matching-Pursuit-Toolbox/%1").arg(ui->list_NewDict->item(i)->toolTip()));
            if (file.open(QIODevice::ReadOnly))
            {
                while(!file.atEnd())
                {
                    contents = file.readLine(0).constData();
                    if(contents.startsWith("atomcount"))
                        atomCountList.append(contents.mid(12).toInt());
                    else
                        stream << contents;
                }
                file.close();
            }
        }

        qint32 sum = 0;
        for(qint32 i = 0; i < atomCountList.length(); i++)
            sum += atomCountList.at(i);

        stream.seek(0);
        stream << QString("atomcount = %1").arg(sum)  << "\n";
    }

    newFile.close();

    ui->list_AllDict->clear();
    readDicts();
    ui->list_NewDict->clear();
    ui->tb_DictName->clear();


}
