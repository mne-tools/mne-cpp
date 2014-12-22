//=============================================================================================================
/**
* @file editorwindow.cpp
* @author Martin Henfling <martin.henfling@tu-ilmenau.de>;
* Daniel Knobl <daniel.knobl@tu-ilmenau.de>;
* Sebastian Krause <sebastian.krause@tu-ilmenau.de>
* @version 1.0
* @date July, 2014
*
* @section LICENSE
*
* Copyright (C) 2014, Martin Henfling, Daniel Knobl and Sebastian Krause. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that
* the following conditions are met:
* * Redistributions of source code must retain the above copyright notice, this list of conditions and the
* following disclaimer.
* * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
* the following disclaimer in the documentation and/or other materials provided with the distribution.
* * Neither the name of MNE-CPP authors nor the names of its contributors may be used
* to endorse or promote products derived from this software without specific prior written permission.
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
* @brief Implementation of the EditorWindow class.
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
bool is_loading =  false;

qreal linStepWidthScale = 1;
qreal linStepWidthModu = 1;
qreal linStepWidthPhase = 1;
qreal linStepWidthChirp = 1;

qreal expStepWidthScale = 2;
qreal expStepWidthModu = 2;
qreal expStepWidthPhase = 2;
qreal expStepWidthChirp = 2;

qreal startValueScale = 1;
qreal startValueModu = 0;
qreal startValuePhase = 0;
qreal startValueChirp = 0;

qreal endValueScale = 1;
qreal endValueModu = 0;
qreal endValuePhase = 0;
qreal endValueChirp = 0;

QString partDictName = "";

QList<qreal> scaleList;
QList<qreal> moduList;
QList<qreal> phaseList;
QList<qreal> chirpList;

qint32 atomCount = 1;

EditorWindow::AtomType atomType;

//*************************************************************************************************************
//=============================================================================================================
// MAIN
//=============================================================================================================

//contructor
EditorWindow::EditorWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::EditorWindow)
{   
    this->setAccessibleName("simple");
    ui->setupUi(this);
    QSettings settings;
    move(settings.value("pos_editor", QPoint(200, 200)).toPoint());
    resize(settings.value("size_editor", QSize(606, 948)).toSize());
    this->restoreState(settings.value("editor_state").toByteArray());


    ui->dspb_StartValueScale->setMaximum(ui->spb_AtomLength->value());    
    read_dicts();
}

//*************************************************************************************************************

EditorWindow::~EditorWindow()
{
    delete ui;
}

//*************************************************************************************************************************************

void EditorWindow::closeEvent(QCloseEvent * event)
{
    Q_UNUSED(event);
    QSettings settings;
    if(!this->isMaximized())
    {
        settings.setValue("pos_editor", pos());
        settings.setValue("size_editor", size());
    }
    settings.setValue("editor_state", this->saveState());
}

//*************************************************************************************************************

void EditorWindow::read_dicts()
{
    ui->list_AllDict->clear();
    QDir dictDir = QDir(QDir::homePath() + "/" + "Matching-Pursuit-Toolbox");

    QStringList filterList;
    filterList.append("*.dict");
    filterList.append("*.pdict");

    QFileInfoList fileList = dictDir.entryInfoList(filterList);

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
    if(ui->list_AllDict->count() > 1) ui->list_AllDict->itemAt(0, 0)->setSelected(true);
    update();
}

//*************************************************************************************************************

// calculates number of atoms if "combine all"
void EditorWindow::calc_atom_count_all_combined()
{
    qint32 count = 0;
    qint32 scaleCount = 1;
    qint32 moduCount = 1;
    qint32 phaseCount = 1;
    qint32 chirpCount = 1;

    if(scaleList.length() != 0) scaleCount = scaleList.length();
    if(moduList.length() != 0) moduCount = moduList.length();
    if(phaseList.length() != 0) phaseCount = phaseList.length();
    if(chirpList.length() != 0) chirpCount = chirpList.length();

    if(atomType == EditorWindow::Gauss)
        count = scaleCount * moduCount * phaseCount;
    else if( atomType == EditorWindow::Chirp)
        count = scaleCount * moduCount * phaseCount * chirpCount;

    if(count > 100000000)
    {
        QMessageBox::warning(this, tr("Error"),
        tr("The number of atoms is too large."));
        return;
    }    
    atomCount = count;
    is_loading = true;
    ui->spb_AtomCount->setValue(count);
    is_loading = false;
}

//*************************************************************************************************************

// calculates parameters for linear stepwidth
QList<qreal> EditorWindow::calc_lin_pos_parameters(qreal startValue, qreal linStepValue)
{
    QList<qreal> resultList;
    qint32 i = 0;
    qreal result = 0;

    i = 0;
    while(i < atomCount)
    {
        result = startValue + (i * linStepValue);
        resultList.append(result);
        i++;
    }
    return resultList;
}

//*************************************************************************************************************

// calculates parameters for linear stepwidth (negativ)
QList<qreal> EditorWindow::calc_lin_neg_parameters(qreal startValue, qreal linStepValue)
{
    QList<qreal> resultList;
    qint32 i = 0;
    qreal result = 0;

    i = 0;
    while(i < atomCount)
    {
        result = startValue - (i * linStepValue);
        resultList.append(result);
        i++;
    }
    return resultList;
}

//*************************************************************************************************************

// calculates parameters for exponential stepwidth (positiv)
QList<qreal> EditorWindow::calc_exp_pos_parameters(qreal startValue, qreal expStepValue)
{
    QList<qreal> resultList;
    qint32 i = 0;
    qreal result = 0;

    i = 0;
    while(i < atomCount)
    {
        result = startValue + pow(i, expStepValue);
        resultList.append(result);
        i++;
    }
    return resultList;
}

//*************************************************************************************************************

// calculates parameters for exponential stepwidth (negativ)
QList<qreal> EditorWindow::calc_exp_neg_parameters(qreal startValue, qreal expStepValue)
{
    QList<qreal> resultList;
    qint32 i = 0;
    qreal result = 0;

    i = 0;
    while(i < atomCount)
    {
        result = startValue - pow(i, expStepValue);
        resultList.append(result);
        i++;
    }
    return resultList;
}

//*************************************************************************************************************

// calculates scale and save to list
QList<qreal> EditorWindow::calc_parameter_values_scale(qreal startValue, qreal linStepValue, qreal expStepValue)
{
    QList<qreal> resultList;
    resultList.clear();

    if(ui->rb_NoStepScale->isChecked())
        resultList.append(startValue);
    else if(ui->rb_LinStepScale->isChecked())
    {
        if(ui->rb_PosCountScale->isChecked()) resultList = calc_lin_pos_parameters(startValue, linStepValue);
        else resultList = calc_lin_neg_parameters(startValue, linStepValue);
    }
    else if(ui->rb_ExpStepScale->isChecked())
    {
        if(ui->rb_PosCountScale->isChecked()) resultList = calc_exp_pos_parameters(startValue, expStepValue);
        else resultList = calc_exp_neg_parameters(startValue, expStepValue);
    }
    if(!resultList.isEmpty()) ui->dspb_EndValueScale->setValue(resultList.last());
    return resultList;
}

//*************************************************************************************************************

// calculates scale and save to list (AllCombined)
QList<qreal> EditorWindow::calc_all_comb_parameter_values_scale(qreal startValue, qreal endvalue, qreal linStepValue, qreal expStepValue)
{
    QList<qreal> resultList;
    resultList.clear();
    qreal temp = endvalue - startValue + 1;
    if(ui->rb_NoStepScale->isChecked())
        resultList.append(startValue);
    else if(ui->rb_LinStepScale->isChecked())
    {
        atomCount = temp / linStepValue;
        resultList = calc_lin_pos_parameters(startValue, linStepValue);
    }
    else if(ui->rb_ExpStepScale->isChecked())
    {
        atomCount = pow(temp, (1/ expStepValue));
        resultList = calc_exp_pos_parameters(startValue, expStepValue);
    }
    return resultList;
}

//*************************************************************************************************************

// calculates modulation and save to list
QList<qreal> EditorWindow::calc_parameter_values_modu(qreal startValue, qreal linStepValue, qreal expStepValue)
{
    QList<qreal> resultList;
    resultList.clear();

    if(ui->rb_NoStepModu->isChecked())
        resultList.append(startValue);
    else if(ui->rb_LinStepModu->isChecked())
    {
        if(ui->rb_PosCountModu->isChecked()) resultList = calc_lin_pos_parameters(startValue, linStepValue);
        else resultList =  calc_lin_neg_parameters(startValue, linStepValue);
    }
    else if(ui->rb_ExpStepModu->isChecked())
    {
        if(ui->rb_PosCountModu->isChecked()) resultList =  calc_exp_pos_parameters(startValue, expStepValue);
        else resultList = calc_exp_neg_parameters(startValue, expStepValue);
    }
    if(!resultList.isEmpty()) ui->dspb_EndValueModu->setValue(resultList.last());
    return resultList;
}

//*************************************************************************************************************

// calculates modulation and save to list (AllCombined)
QList<qreal> EditorWindow::calc_all_comb_parameter_values_modu(qreal startValue, qreal endvalue, qreal linStepValue, qreal expStepValue)
{
    QList<qreal> resultList;
    resultList.clear();
    qreal temp = endvalue - startValue + 1;
    if(ui->rb_NoStepModu->isChecked())
        resultList.append(startValue);
    else if(ui->rb_LinStepModu->isChecked())
    {
        atomCount = temp / linStepValue;
        resultList = calc_lin_pos_parameters(startValue, linStepValue);
    }
    if(ui->rb_ExpStepModu->isChecked())
    {
        atomCount = pow(temp, (1.0/ expStepValue));
        resultList = calc_exp_pos_parameters(startValue, expStepValue);
    }
    return resultList;
}

//*************************************************************************************************************

// calculates phase and save to list
QList<qreal> EditorWindow::calc_parameter_values_phase(qreal startValue, qreal linStepValue, qreal expStepValue)
{
    QList<qreal> resultList;
    resultList.clear();

    if(ui->rb_NoStepPhase->isChecked())
        resultList.append(startValue);
    else if(ui->rb_LinStepPhase->isChecked())
    {
        if(ui->rb_PosCountPhase->isChecked()) resultList = calc_lin_pos_parameters(startValue, linStepValue);
        else resultList = calc_lin_neg_parameters(startValue, linStepValue);
    }
    else if(ui->rb_ExpStepPhase->isChecked())
    {
        if(ui->rb_PosCountPhase->isChecked()) resultList = calc_exp_pos_parameters(startValue, expStepValue);
        else resultList = calc_exp_neg_parameters(startValue, expStepValue);
    }
    if(!resultList.isEmpty()) ui->dspb_EndValuePhase->setValue(resultList.last());
    return resultList;
}

//*************************************************************************************************************

// calculates phase and save to list (AllCombined)
QList<qreal> EditorWindow::calc_all_comb_parameter_values_phase(qreal startValue, qreal endvalue, qreal linStepValue, qreal expStepValue)
{
    QList<qreal> resultList;
    resultList.clear();
    qreal temp = endvalue - startValue + 1;
    if(ui->rb_NoStepPhase->isChecked())
        resultList.append(startValue);
    else if(ui->rb_LinStepPhase->isChecked())
    {
        atomCount = temp / linStepValue;
        resultList = calc_lin_pos_parameters(startValue, linStepValue);
    }
    else if(ui->rb_ExpStepPhase->isChecked())
    {
        atomCount = pow(temp, (1/ expStepValue));
        resultList = calc_exp_pos_parameters(startValue, expStepValue);
    }
    return resultList;
}

//*************************************************************************************************************

// calculates chirp and save to list
QList<qreal> EditorWindow::calc_parameter_values_chirp(qreal startValue, qreal linStepValue, qreal expStepValue)
{
    QList<qreal> resultList;
    resultList.clear();

    if(ui->rb_NoStepChirp->isChecked())
        resultList.append(startValue);
    else if(ui->rb_LinStepChirp->isChecked())
    {
        if(ui->rb_PosCountChirp->isChecked()) resultList = calc_lin_pos_parameters(startValue, linStepValue);
        else resultList = calc_lin_neg_parameters(startValue, linStepValue);
    }
    else if(ui->rb_ExpStepChirp->isChecked())
    {
        if(ui->rb_PosCountChirp->isChecked()) resultList = calc_exp_pos_parameters(startValue, expStepValue);
        else resultList = calc_exp_neg_parameters(startValue, expStepValue);
    }
    if(!resultList.isEmpty()) ui->dspb_EndValuePhase->setValue(resultList.last());
    return resultList;
}

//*************************************************************************************************************

// calculates chirp and save to list (AllCombined)
QList<qreal> EditorWindow::calc_all_comb_parameter_values_chirp(qreal startValue, qreal endvalue, qreal linStepValue, qreal expStepValue)
{
    QList<qreal> resultList;
    resultList.clear();
    qreal temp = endvalue - startValue + 1;
    if(ui->rb_NoStepChirp->isChecked())
        resultList.append(startValue);
    else if(ui->rb_LinStepChirp->isChecked())
    {
        atomCount = temp / linStepValue;
        resultList = calc_lin_pos_parameters(startValue, linStepValue);
    }
    else if(ui->rb_ExpStepChirp->isChecked())
    {
        atomCount = pow(temp, (1/ expStepValue));
        resultList = calc_exp_pos_parameters(startValue, expStepValue);
    }
    return resultList;
}

//*************************************************************************************************************

// handle scale
void EditorWindow::calc_scale_value()
{
    if(allCombined)
    {
        ui->dspb_EndValueScale->setDisabled(ui->rb_NoStepScale->isChecked());
        if(ui->rb_LinStepScale->isChecked()) ui->dspb_EndValueScale->setMinimum(startValueScale + linStepWidthScale);
        else if(ui->rb_ExpStepScale->isChecked()) ui->dspb_EndValueScale->setMinimum(startValueScale + 1);
        else
        {
            ui->dspb_EndValueScale->setMinimum(startValueScale);
            ui->dspb_EndValueScale->setValue(startValueScale);
        }
        scaleList = calc_all_comb_parameter_values_scale(startValueScale, endValueScale, linStepWidthScale, expStepWidthScale);
        calc_atom_count_all_combined();
    }
    else
    {
        ui->dspb_EndValueScale->setMinimum(0.05);
        scaleList = calc_parameter_values_scale(startValueScale, linStepWidthScale, expStepWidthScale);
    }
}

//*************************************************************************************************************

// handle modulation
void EditorWindow::calc_modu_value()
{
    if(allCombined)
    {
        ui->dspb_EndValueModu->setDisabled(ui->rb_NoStepModu->isChecked());
        if(ui->rb_LinStepModu->isChecked()) ui->dspb_EndValueModu->setMinimum(startValueModu + linStepWidthModu);
        else if(ui->rb_ExpStepModu->isChecked()) ui->dspb_EndValueModu->setMinimum(startValueModu + 1);
        else
        {
            ui->dspb_EndValueModu->setMinimum(startValueModu);
            ui->dspb_EndValueModu->setValue(startValueModu);
        }
        moduList = calc_all_comb_parameter_values_modu(startValueModu, endValueModu, linStepWidthModu, expStepWidthModu);
        calc_atom_count_all_combined();
    }
    else
        moduList = calc_parameter_values_modu(startValueModu, linStepWidthModu, expStepWidthModu);
}

//*************************************************************************************************************

// handle phase
void EditorWindow::calc_phase_value()
{
    if(allCombined)
    {
        ui->dspb_EndValuePhase->setDisabled(ui->rb_NoStepPhase->isChecked());
        if(ui->rb_LinStepPhase->isChecked()) ui->dspb_EndValuePhase->setMinimum(startValuePhase + linStepWidthPhase);
        else if(ui->rb_ExpStepPhase->isChecked()) ui->dspb_EndValuePhase->setMinimum(startValuePhase + 1);
        else
        {
            ui->dspb_EndValuePhase->setMinimum(startValuePhase);
            ui->dspb_EndValuePhase->setValue(startValuePhase);
        }
        phaseList = calc_all_comb_parameter_values_phase(startValuePhase, endValuePhase, linStepWidthPhase, expStepWidthPhase);
        calc_atom_count_all_combined();
    }
    else
        phaseList = calc_parameter_values_phase(startValuePhase, linStepWidthPhase, expStepWidthPhase);
}

//*************************************************************************************************************

// handle chirp
void EditorWindow::calc_chirp_value()
{
    if(allCombined)
    {
        ui->dspb_EndValueChirp->setDisabled(ui->rb_NoStepChirp->isChecked());
        if(ui->rb_LinStepChirp->isChecked()) ui->dspb_EndValueChirp->setMinimum(startValueChirp + linStepWidthChirp);
        else if(ui->rb_ExpStepChirp->isChecked()) ui->dspb_EndValueChirp->setMinimum(startValueChirp + 1);
        else
        {
            ui->dspb_EndValueChirp->setMinimum(startValueChirp);
            ui->dspb_EndValueChirp->setValue(startValueChirp);
        }
        chirpList = calc_all_comb_parameter_values_chirp(startValueChirp, endValueChirp, linStepWidthChirp, expStepWidthChirp);
        calc_atom_count_all_combined();
    }
    else
        chirpList = calc_parameter_values_chirp(startValueChirp, linStepWidthChirp, expStepWidthChirp);
}

//*************************************************************************************************************

// access if namechange of PartDictName
void EditorWindow::on_tb_PartDictName_editingFinished()
{
    partDictName = ui->tb_PartDictName->text();
}

//*************************************************************************************************************

// access if "All combined"
void EditorWindow::on_chb_CombAllPara_toggled(bool checked)
{
    allCombined = checked;

    ui->spb_AtomCount->setDisabled(checked);
    ui->dspb_EndValueScale->setEnabled(true);
    ui->dspb_EndValueModu->setEnabled(true);
    ui->dspb_EndValuePhase->setEnabled(true);
    ui->dspb_EndValueChirp->setEnabled(true);

    ui->dspb_LinStepScale->setEnabled(ui->rb_LinStepScale->isChecked() && ui->spb_AtomCount->value() > 1);
    ui->dspb_LinStepModu->setEnabled(ui->rb_LinStepModu->isChecked() && ui->spb_AtomCount->value() > 1);
    ui->dspb_LinStepPhase->setEnabled(ui->rb_LinStepPhase->isChecked() && ui->spb_AtomCount->value() > 1);
    ui->dspb_LinStepChirp->setEnabled(ui->rb_LinStepChirp->isChecked() && ui->spb_AtomCount->value() > 1);

    ui->dspb_ExpStepScale->setEnabled(ui->rb_ExpStepScale->isChecked() && ui->spb_AtomCount->value() > 1);
    ui->dspb_ExpStepModu->setEnabled(ui->rb_ExpStepModu->isChecked() && ui->spb_AtomCount->value() > 1);
    ui->dspb_ExpStepPhase->setEnabled(ui->rb_ExpStepPhase->isChecked() && ui->spb_AtomCount->value() > 1);
    ui->dspb_ExpStepChirp->setEnabled(ui->rb_ExpStepChirp->isChecked() && ui->spb_AtomCount->value() > 1);

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

    if(ui->rb_LinStepScale->isChecked()) ui->dspb_EndValueScale->setMinimum(startValueScale + linStepWidthScale);
    else if(ui->rb_ExpStepScale->isChecked()) ui->dspb_EndValueScale->setMinimum(startValueScale + 1);
    else ui->dspb_EndValueScale->setValue(startValueScale);

    if(ui->rb_LinStepModu->isChecked()) ui->dspb_EndValueModu->setMinimum(startValueModu + linStepWidthModu);
    else if(ui->rb_ExpStepModu->isChecked()) ui->dspb_EndValueModu->setMinimum(startValueModu + 1);
    else ui->dspb_EndValueModu->setValue(startValueModu);

    if(ui->rb_LinStepPhase->isChecked()) ui->dspb_EndValuePhase->setMinimum(startValuePhase + linStepWidthPhase);
    else if(ui->rb_ExpStepPhase->isChecked()) ui->dspb_EndValuePhase->setMinimum(startValuePhase + 1);
    else ui->dspb_EndValuePhase->setValue(startValuePhase);

    if(ui->rb_LinStepChirp->isChecked()) ui->dspb_EndValueChirp->setMinimum(startValueChirp + linStepWidthChirp);
    else if(ui->rb_ExpStepChirp->isChecked()) ui->dspb_EndValueChirp->setMinimum(startValueChirp + 1);
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
    }

    ui->rb_NoStepScale->setEnabled(checked);
    ui->rb_NoStepModu->setEnabled(checked);
    ui->rb_NoStepPhase->setEnabled(checked);
    ui->rb_NoStepChirp->setEnabled(checked);

    ui->lb_StepDefScale ->setEnabled(checked);
    ui->lb_StepDefModu ->setEnabled(checked);
    ui->lb_StepDefPhase ->setEnabled(checked);
    ui->lb_StepDefChirp ->setEnabled(checked);

    ui->rb_LinStepScale->setEnabled(checked);
    ui->rb_LinStepModu->setEnabled(checked);
    ui->rb_LinStepPhase->setEnabled(checked);
    ui->rb_LinStepChirp->setEnabled(checked);

    ui->rb_ExpStepScale->setEnabled(checked);
    ui->rb_ExpStepModu->setEnabled(checked);
    ui->rb_ExpStepPhase->setEnabled(checked);
    ui->rb_ExpStepChirp->setEnabled(checked);

    ui->lb_LinNScale->setEnabled(checked);
    ui->lb_LinNModu->setEnabled(checked);
    ui->lb_LinNPhase->setEnabled(checked);
    ui->lb_LinNChirp->setEnabled(checked);

    ui->lb_ExpNScale->setEnabled(checked);
    ui->lb_ExpNModu->setEnabled(checked);
    ui->lb_ExpNPhase->setEnabled(checked);
    ui->lb_ExpNChirp->setEnabled(checked);

    endValueScale = ui->dspb_EndValueScale->value();
    endValueModu = ui->dspb_EndValueModu->value();
    endValuePhase = ui->dspb_EndValuePhase->value();
    endValueChirp = ui->dspb_EndValueChirp->value();

    calc_scale_value();
    calc_modu_value();
    calc_phase_value();
    calc_chirp_value();



    // update ui
    is_loading = true;
    ui->spb_AtomCount->setValue(ui->spb_AtomCount->value() + 1);
    ui->spb_AtomCount->setValue(ui->spb_AtomCount->value() - 1);
    is_loading = false;
    //calc_atom_count_all_combined();

}

//*************************************************************************************************************

void EditorWindow::on_spb_AtomLength_editingFinished()
{
    // set max startvalue of scale
    ui->dspb_StartValueScale->setMaximum(ui->spb_AtomLength->value());
    ui->dspb_StartValueModu->setMaximum(ui->spb_AtomLength->value() / 2);
    ui->dspb_StartValuePhase->setMaximum(ui->spb_AtomLength->value());
    ui->dspb_StartValueChirp->setMaximum(ui->spb_AtomLength->value() / 2);

    ui->dspb_EndValueScale->setMaximum(ui->spb_AtomLength->value());
    ui->dspb_EndValueModu->setMaximum(ui->spb_AtomLength->value() / 2);
    ui->dspb_EndValuePhase->setMaximum(ui->spb_AtomLength->value());
    ui->dspb_EndValueChirp->setMaximum(ui->spb_AtomLength->value() / 2);

    ui->dspb_LinStepScale->setMaximum(ui->spb_AtomLength->value());
    ui->dspb_LinStepModu->setMaximum(ui->spb_AtomLength->value() / 2);
    ui->dspb_LinStepPhase->setMaximum(ui->spb_AtomLength->value());
    ui->dspb_LinStepChirp->setMaximum(ui->spb_AtomLength->value() / 2);
}

//*************************************************************************************************************

// set number of atoms (recalculate stopvalues)
void EditorWindow::on_spb_AtomCount_valueChanged(int arg1)
{
    if(is_loading) return;

    atomCount = arg1;
    bool oneAtom = true;
    if(atomCount != 1  || atomCount == 1 && allCombined)oneAtom = false;

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

    calc_scale_value();
    calc_modu_value();
    calc_phase_value();
    calc_chirp_value();
}

//*************************************************************************************************************

// for scale
void EditorWindow::on_dspb_StartValueScale_editingFinished()
{
    startValueScale = ui->dspb_StartValueScale->value();
    calc_scale_value();
}

//*************************************************************************************************************

void EditorWindow::on_dspb_EndValueScale_editingFinished()
{
    endValueScale = ui->dspb_EndValueScale->value();
    calc_scale_value();
}

//*************************************************************************************************************

void EditorWindow::on_rb_NoStepScale_toggled(bool checked)
{
    if(checked) ui->lb_StartValueScale->setText("value:");
    else ui->lb_StartValueScale->setText("start value:");

    ui->dspb_LinStepScale->setEnabled(false);
    ui->dspb_ExpStepScale->setEnabled(false);

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
    calc_scale_value();
}

//*************************************************************************************************************

void EditorWindow::on_rb_LinStepScale_toggled(bool checked)
{
    if(checked) linStepWidthScale = ui->dspb_LinStepScale->value();

    ui->dspb_LinStepScale->setEnabled(true);
    ui->dspb_ExpStepScale->setEnabled(false);

    calc_scale_value();
}

//*************************************************************************************************************

void EditorWindow::on_rb_ExpStepScale_toggled(bool checked)
{
    if(checked) expStepWidthScale = ui->dspb_ExpStepScale->value();

    ui->dspb_ExpStepScale->setEnabled(true);
    ui->dspb_LinStepScale->setEnabled(false);

    calc_scale_value();
}

//*************************************************************************************************************

void EditorWindow::on_dspb_LinStepScale_editingFinished()
{
    linStepWidthScale = ui->dspb_LinStepScale->value();
    calc_scale_value();
}

//*************************************************************************************************************

void EditorWindow::on_dspb_ExpStepScale_editingFinished()
{
    expStepWidthScale = ui->dspb_ExpStepScale->value();
    calc_scale_value();
}

//*************************************************************************************************************

void EditorWindow::on_rb_PosCountScale_toggled()
{
    if(!allCombined)
        scaleList = calc_parameter_values_scale(startValueScale, linStepWidthScale, expStepWidthScale);
}

//*************************************************************************************************************

void EditorWindow::on_rb_NegCountScale_toggled()
{
    if(!allCombined)
        scaleList = calc_parameter_values_scale(startValueScale, linStepWidthScale, expStepWidthScale);
}

//*************************************************************************************************************

// for modulation
void EditorWindow::on_dspb_StartValueModu_editingFinished()
{
    startValueModu = ui->dspb_StartValueModu->value();
    calc_modu_value();
}

//*************************************************************************************************************

void EditorWindow::on_dspb_EndValueModu_editingFinished()
{
    endValueModu = ui->dspb_EndValueModu->value();
    calc_modu_value();
}

//*************************************************************************************************************

void EditorWindow::on_rb_NoStepModu_toggled(bool checked)
{
    if(checked) ui->lb_StartValueModu->setText("value:");
    else ui->lb_StartValueModu->setText("start value:");

    ui->dspb_LinStepModu->setEnabled(false);
    ui->dspb_ExpStepModu->setEnabled(false);

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
    calc_modu_value();
}

//*************************************************************************************************************

void EditorWindow::on_rb_LinStepModu_toggled(bool checked)
{
    if(checked) linStepWidthModu = ui->dspb_LinStepModu->value();
    ui->dspb_LinStepModu->setEnabled(true);
    ui->dspb_ExpStepModu->setEnabled(false);

    calc_modu_value();
}

//*************************************************************************************************************

void EditorWindow::on_rb_ExpStepModu_toggled(bool checked)
{
    if(checked) expStepWidthModu = ui->dspb_ExpStepModu->value();
    ui->dspb_ExpStepModu->setEnabled(true);
    ui->dspb_LinStepModu->setEnabled(false);
    calc_modu_value();
}

//*************************************************************************************************************

void EditorWindow::on_dspb_LinStepModu_editingFinished()
{
    linStepWidthModu = ui->dspb_LinStepModu->value();
    calc_modu_value();
}

//*************************************************************************************************************

void EditorWindow::on_dspb_ExpStepModu_editingFinished()
{
    expStepWidthModu = ui->dspb_ExpStepModu->value();
    calc_modu_value();
}

//*************************************************************************************************************

void EditorWindow::on_rb_PosCountModu_toggled()
{
    if(allCombined)
        moduList = calc_parameter_values_modu(startValueModu, linStepWidthModu, expStepWidthModu);
}

//*************************************************************************************************************

void EditorWindow::on_rb_NegCountModu_toggled()
{
    if(allCombined)
        moduList = calc_parameter_values_modu(startValueModu, linStepWidthModu, expStepWidthModu);
}

//*************************************************************************************************************

// for phase
void EditorWindow::on_dspb_StartValuePhase_editingFinished()
{
    startValuePhase = ui->dspb_StartValuePhase->value();
    calc_phase_value();
}

//*************************************************************************************************************

void EditorWindow::on_dspb_EndValuePhase_editingFinished()
{
    endValuePhase = ui->dspb_EndValuePhase->value();
    calc_phase_value();
}

//*************************************************************************************************************

void EditorWindow::on_rb_NoStepPhase_toggled(bool checked)
{
    if(checked) ui->lb_StartValuePhase->setText("value:");
    else ui->lb_StartValuePhase->setText("start value:");

    ui->dspb_ExpStepPhase->setEnabled(false);
    ui->dspb_LinStepPhase->setEnabled(false);

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
    calc_phase_value();
}

//*************************************************************************************************************

void EditorWindow::on_rb_LinStepPhase_toggled(bool checked)
{
    if(checked) linStepWidthPhase = ui->dspb_LinStepPhase->value();

    ui->dspb_LinStepPhase->setEnabled(true);
    ui->dspb_ExpStepPhase->setEnabled(false);

    calc_phase_value();
}

//*************************************************************************************************************

void EditorWindow::on_rb_ExpStepPhase_toggled(bool checked)
{
    if(checked) expStepWidthPhase = ui->dspb_ExpStepPhase->value();

    ui->dspb_ExpStepPhase->setEnabled(true);
    ui->dspb_LinStepPhase->setEnabled(false);

    calc_phase_value();
}

//*************************************************************************************************************

void EditorWindow::on_dspb_LinStepPhase_editingFinished()
{
    linStepWidthPhase = ui->dspb_LinStepPhase->value();
    calc_phase_value();
}

//*************************************************************************************************************

void EditorWindow::on_dspb_ExpStepPhase_editingFinished()
{
    expStepWidthPhase = ui->dspb_ExpStepPhase->value();
    calc_phase_value();
}

//*************************************************************************************************************

void EditorWindow::on_rb_PosCountPhase_toggled()
{
    if(allCombined)
        phaseList = calc_parameter_values_phase(startValuePhase, linStepWidthPhase, expStepWidthPhase);
}

//*************************************************************************************************************

void EditorWindow::on_rb_NegCountPhase_toggled()
{
    if(allCombined)
        phaseList = calc_parameter_values_phase(startValuePhase, linStepWidthPhase, expStepWidthPhase);
}

//*************************************************************************************************************

// for chirp
void EditorWindow::on_dspb_StartValueChirp_editingFinished()
{
    startValueChirp = ui->dspb_StartValueChirp->value();
    calc_chirp_value();
}

//*************************************************************************************************************

void EditorWindow::on_dspb_EndValueChirp_editingFinished()
{
    endValueChirp = ui->dspb_EndValueChirp->value();
    calc_chirp_value();
}

//*************************************************************************************************************

void EditorWindow::on_rb_NoStepChirp_toggled(bool checked)
{
    if(checked) ui->lb_StartValueChirp->setText("value:");
    else ui->lb_StartValueChirp->setText("start value:");

    ui->dspb_ExpStepChirp->setEnabled(false);
    ui->dspb_LinStepChirp->setEnabled(false);

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
    calc_chirp_value();
}

//*************************************************************************************************************

void EditorWindow::on_rb_LinStepChirp_toggled(bool checked)
{
     if(checked) linStepWidthChirp = ui->dspb_LinStepChirp->value();

     ui->dspb_LinStepChirp->setEnabled(true);
     ui->dspb_ExpStepChirp->setEnabled(false);

     calc_chirp_value();
}

//*************************************************************************************************************

void EditorWindow::on_rb_ExpStepChirp_toggled(bool checked)
{
    if(checked) expStepWidthChirp = ui->dspb_ExpStepChirp->value();

    ui->dspb_ExpStepChirp->setEnabled(true);
    ui->dspb_LinStepChirp->setEnabled(false);

    calc_chirp_value();
}

//*************************************************************************************************************

void EditorWindow::on_dspb_LinStepChirp_editingFinished()
{
    linStepWidthChirp = ui->dspb_LinStepChirp->value();
    calc_chirp_value();
}

//*************************************************************************************************************

void EditorWindow::on_dspb_ExpStepChirp_editingFinished()
{
    expStepWidthChirp = ui->dspb_ExpStepChirp->value();
    calc_chirp_value();
}

//*************************************************************************************************************

void EditorWindow::on_rb_PosCountChirp_toggled()
{
    if(allCombined)
        chirpList = calc_parameter_values_chirp(startValueChirp, linStepWidthChirp, expStepWidthChirp);
}

//*************************************************************************************************************

void EditorWindow::on_rb_NegCountChirp_toggled()
{
    if(allCombined)
        chirpList = calc_parameter_values_chirp(startValueChirp, linStepWidthChirp, expStepWidthChirp);
}

//*************************************************************************************************************

// check whether gauss or chirp
void EditorWindow::on_rb_GaussAtomType_toggled(bool checked)
{
    if(checked) atomType = EditorWindow::Gauss;
}

//*************************************************************************************************************

void EditorWindow::on_rb_ChirpAtomType_toggled(bool checked)
{
    if(checked) atomType = EditorWindow::Chirp;

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
    }
    else if(checked)
    {
        ui->lb_EndValueChirp->setDisabled(ui->rb_NoStepChirp->isChecked());
        ui->rb_PosCountChirp->setDisabled(ui->rb_NoStepChirp->isChecked());
        ui->rb_NegCountChirp->setDisabled(ui->rb_NoStepChirp->isChecked());
        ui->lb_EndValueChirp->setDisabled(!allCombined);
    }
}

//*************************************************************************************************************

// calc all atoms with choosen parameters and save to list and to drive
void EditorWindow::on_btt_CalcAtoms_clicked()
{
    QStringList resultList;
    resultList.clear();



    if(partDictName.isEmpty())
    {
        QMessageBox::warning(this, tr("Error"),
        tr("It was not assigned a name."));
        ui->tb_PartDictName->setFocus();
        return;
    }

    qint32 k = 0;
    while(k < ui->list_AllDict->count())
    {
        if(QString::compare(partDictName, ui->list_AllDict->item(k)->text()) == 0)
        { QMessageBox::warning(this, tr("Error"),
                tr("The name is already taken."));

                ui->tb_PartDictName->setFocus();
                ui->tb_PartDictName->selectAll();
                return;
        }
        k++;
    }

    QString save_path_xml = QString(QDir::homePath() + "/" + "Matching-Pursuit-Toolbox/%1.pdict").arg(partDictName);
    QFile xml_file(save_path_xml);   
    if (xml_file.open (QIODevice::WriteOnly))
    {
        QXmlStreamWriter xmlWriter(&xml_file);
        xmlWriter.setAutoFormatting(true);
        xmlWriter.writeStartDocument();

        xmlWriter.writeStartElement("COUNT");
        xmlWriter.writeAttribute("of_atoms", QString::number(ui->spb_AtomCount->value()));
        xmlWriter.writeStartElement("built_Atoms");

        if(atomType == EditorWindow::Chirp)
            xmlWriter.writeAttribute("formula", "Chirpatom");
        else if(atomType == EditorWindow::Gauss)
            xmlWriter.writeAttribute("formula", "Gaboratom");
        xmlWriter.writeAttribute("sample_count", QString::number(ui->spb_AtomLength->value()));
        xmlWriter.writeAttribute("atom_count", QString::number(atomCount));
        xmlWriter.writeAttribute("source_dict", partDictName);

        ChirpAtom *cAtom = new ChirpAtom();
        GaborAtom *gAtom = new GaborAtom();

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
                            qreal tempScale = ui->dspb_StartValueScale->value();
                            if(scaleList.length() > 0 && scaleCount < scaleList.length()) tempScale = scaleList.at(scaleCount);
                            qreal tempModu = ui->dspb_StartValueModu->value();
                            if(moduList.length() > 0 && moduCount < moduList.length()) tempModu = moduList.at(moduCount);
                            qreal tempPhase = 2 * PI * ui->dspb_StartValuePhase->value() / tempScale;
                            if(phaseList.length() > 0 && phaseCount < phaseList.length()) tempPhase = 2 * PI * phaseList.at(phaseCount) / tempScale;
                            qreal tempChirp = ui->dspb_StartValueChirp->value();
                            if(chirpList.length() > 0 && chirpCount < chirpList.length()) tempChirp = chirpList.at(chirpCount);

                            if(atomType == EditorWindow::Chirp)
                             {
                                resultList = cAtom->create_string_values(ui->spb_AtomLength->value(), tempScale, ui->spb_AtomLength->value() / 2, tempModu, tempPhase, tempChirp);
                                xmlWriter.writeStartElement("ATOM");
                                xmlWriter.writeAttribute("ID", QString::number(atomIndex));
                                xmlWriter.writeAttribute("scale", QString::number(tempScale));
                                xmlWriter.writeAttribute("modu", QString::number(tempModu));
                                xmlWriter.writeAttribute("phase", QString::number(tempPhase));
                                xmlWriter.writeAttribute("chirp", QString::number(tempChirp));

                            }
                            else if(atomType == EditorWindow::Gauss)
                            {
                                resultList = gAtom->create_string_values(ui->spb_AtomLength->value(), tempScale, ui->spb_AtomLength->value() / 2, tempModu, tempPhase);
                                xmlWriter.writeStartElement("ATOM");                                
                                xmlWriter.writeAttribute("ID", QString::number(atomIndex));                                
                                xmlWriter.writeAttribute("scale", QString::number(tempScale));
                                xmlWriter.writeAttribute("modu", QString::number(tempModu));
                                xmlWriter.writeAttribute("phase", QString::number(tempPhase));
                            }

                            xmlWriter.writeStartElement("samples");
                            QString samples_to_xml;
                            for (qint32 it = 0; it < resultList.length(); it++)
                            {
                                samples_to_xml.append(resultList.at(it));
                                samples_to_xml.append(":");
                            }
                            xmlWriter.writeAttribute("samples", samples_to_xml);
                            xmlWriter.writeEndElement();

                            xmlWriter.writeEndElement();

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
        else //not all params combined
        {
            if(ui->spb_AtomCount->value() != 1)
            {
                if(ui->rb_ChirpAtomType->isChecked())
                {
                    if(ui->rb_NoStepScale->isChecked() && ui->rb_NoStepModu->isChecked() && ui->rb_NoStepPhase->isChecked() && ui->rb_NoStepChirp)
                    {
                        QMessageBox::warning(this, tr("Warning"),QString("It created %1 identical atoms. Please change the number of atoms on one or let you vary a parameter.").arg(ui->spb_AtomCount->value()));
                        return;
                    }
                }
                else if(ui->rb_NoStepScale->isChecked() && ui->rb_NoStepModu->isChecked() && ui->rb_NoStepPhase->isChecked())
                {
                    QMessageBox::warning(this, tr("Warning"),QString("It created %1 identical atoms. Please change the number of atoms on one or let you vary a parameter.").arg(ui->spb_AtomCount->value()));
                    return;
                }
            }

            qint32 i = 0;
            while (i < atomCount)
            {
                qreal tempScale = ui->dspb_StartValueScale->value();
                if(scaleList.length() > 0 && i < scaleList.length()) tempScale = scaleList.at(i);
                qreal tempModu = ui->dspb_StartValueModu->value();
                if(moduList.length() > 0 && i < moduList.length()) tempModu = moduList.at(i);
                qreal tempPhase = 2 * PI * ui->dspb_StartValuePhase->value() / tempScale;
                if(phaseList.length() > 0 && i < phaseList.length()) tempPhase = 2 * PI * phaseList.at(i) / tempScale;
                qreal tempChirp = ui->dspb_StartValueChirp->value();
                if(chirpList.length() > 0 && i < chirpList.length()) tempChirp = chirpList.at(i);

                if(atomType == EditorWindow::Chirp)
                {
                    resultList = cAtom->create_string_values(ui->spb_AtomLength->value(), tempScale, ui->spb_AtomLength->value() / 2, tempModu, tempPhase, tempChirp);
                    xmlWriter.writeStartElement("ATOM");
                    xmlWriter.writeAttribute("ID", QString::number(i));
                    xmlWriter.writeAttribute("scale", QString::number(tempScale));
                    xmlWriter.writeAttribute("modu", QString::number(tempModu));
                    xmlWriter.writeAttribute("phase", QString::number(tempPhase));
                    xmlWriter.writeAttribute("chirp", QString::number(tempChirp));
                }
                else if(atomType == EditorWindow::Gauss)
                {                    
                    resultList = gAtom->create_string_values(ui->spb_AtomLength->value(), tempScale, ui->spb_AtomLength->value() / 2, tempModu, tempPhase);
                    xmlWriter.writeStartElement("ATOM");
                    xmlWriter.writeAttribute("ID", QString::number(i));
                    xmlWriter.writeAttribute("scale", QString::number(tempScale));
                    xmlWriter.writeAttribute("modu", QString::number(tempModu));
                    xmlWriter.writeAttribute("phase", QString::number(tempPhase));
                }               

                xmlWriter.writeStartElement("samples");
                QString samples_to_xml;
                for (qint32 it = 0; it < resultList.length(); it++)
                {
                    samples_to_xml.append(resultList.at(it));
                    samples_to_xml.append(":");
                }
                xmlWriter.writeAttribute("samples", samples_to_xml);
                xmlWriter.writeEndElement();    //samples
                xmlWriter.writeEndElement();    //ATOM

                i++;
            }
        }
        delete cAtom;
        delete gAtom;

        xmlWriter.writeEndElement();    //build_Atoms
        xmlWriter.writeEndElement();    //COUNT
        xmlWriter.writeEndDocument();
    }
    xml_file.close();
    read_dicts();

    if(ui->list_AllDict->count() > 1) ui->list_AllDict->itemAt(0, 0)->setSelected(true);
}

//*************************************************************************************************************

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
    if(ui->list_AllDict->count() > 0) ui->list_AllDict->itemAt(0,0)->setSelected(true);
    if(ui->list_NewDict->count() > 0) ui->btt_SaveDicts->setEnabled(true);
}

//*************************************************************************************************************

void EditorWindow::on_list_AllDict_doubleClicked()
{
    QListWidgetItem* item = ui->list_AllDict->selectedItems().at(0);
    ui->list_NewDict->addItem(item->clone());

    item->setSelected(false);
    delete item;
    if(ui->list_AllDict->count() > 0) ui->list_AllDict->itemAt(0,0)->setSelected(true);
    if(ui->list_NewDict->count() > 0) ui->btt_SaveDicts->setEnabled(true);

}

//*************************************************************************************************************

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
    if(ui->list_NewDict->count() > 1) ui->list_NewDict->itemAt(0,0)->setSelected(true);
    if(ui->list_NewDict->count() == 0) ui->btt_SaveDicts->setEnabled(false);
}

//*************************************************************************************************************

void EditorWindow::on_list_NewDict_doubleClicked()
{
    QListWidgetItem* item = ui->list_NewDict->selectedItems().at(0);
    ui->list_AllDict->addItem(item->clone());

    item->setSelected(false);
    delete item;
    if(ui->list_NewDict->count() > 1) ui->list_NewDict->itemAt(0,0)->setSelected(true);
    if(ui->list_NewDict->count() == 0) ui->btt_SaveDicts->setEnabled(false);
}

//*************************************************************************************************************

void EditorWindow::on_btt_DeleteDict_clicked()
{    
    deleteDicts();
}

//*************************************************************************************************************

void EditorWindow::deleteDicts()
{
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

    qint32 i = 0;
    QList<QListWidgetItem*> delItemsList = ui->list_AllDict->selectedItems();
    while( i < delItemsList.length())
    {
        QFile file(QString(QDir::homePath() + "/" + "Matching-Pursuit-Toolbox/%1").arg(delItemsList.at(i)->toolTip()));
        file.remove();

        QListWidgetItem* delItem = delItemsList.at(i);
        delItem->setSelected(false);
        delete delItem;
        i++;
    }
    emit dict_saved();
}

//*************************************************************************************************************

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

//*************************************************************************************************************

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

//*************************************************************************************************************

void EditorWindow::on_btt_SaveDicts_clicked()
{
    QList<qint32> atomCountList;
    if(ui->tb_DictName->text().isEmpty())
    {
        QMessageBox::warning(this, tr("Error"),
        tr("There was no name for the dictionary awarded."));
        ui->tb_DictName->setFocus();
        return;
    }

    QStringList filterList;
    filterList.append("*.dict");
    QDir dictDir = QDir(QDir::homePath() + "/" + "Matching-Pursuit-Toolbox");
    QFileInfoList fileList = dictDir.entryInfoList(filterList);

    for(qint32 i = 0; i < fileList.length(); i++)
    {
        QFileInfo fileInfo = fileList.at(i);
        if(QString::compare(fileInfo.baseName(), ui->tb_DictName->text()) == 0)
        {
                QMessageBox::warning(this, tr("Error"),
                tr("The name for the dictionary is already taken."));
                ui->tb_DictName->setFocus();
                ui->tb_DictName->selectAll();
                return;
        }
    }

    qint32 summarize_atoms = 0;
    QDomDocument xml_dict;
    QDomElement header = xml_dict.createElement("COUNT");
    for(qint32 i = 0; i < ui->list_NewDict->count(); i++)
    {
        QFile xml_file(QString(QDir::homePath() + "/" + "Matching-Pursuit-Toolbox/%1").arg(ui->list_NewDict->item(i)->toolTip()));

        QDomDocument xml_pdict;
        xml_pdict.setContent(&xml_file);
        QDomElement xml_root= xml_pdict.documentElement();

        QDomNodeList node_list = xml_root.childNodes();
        qint32 count = node_list.count();
        for(qint32 ii = 0; ii < count; ii++)
        {
            bool hasElement = false;
            QDomElement built = node_list.at(0).toElement();
            if(built.hasChildNodes())
            {
                QDomNodeList write_list = header.elementsByTagName("built_Atoms");
                qint32 count_2 = write_list.count();
                for(qint32 k = 0; k < count_2; k++)
                {
                    QDomElement write_element = write_list.at(k).toElement();
                    if((built.attribute("source_dict", built.text())) == (write_element.attribute("source_dict", write_element.text())))
                    {
                        hasElement = true;
                        break;
                    }
                }
                if(!hasElement)
                {
                    summarize_atoms += (built.attribute("atom_count", built.text())).toInt();
                    header.appendChild(built);
                    QDomElement atom = built.firstChild().toElement();
                    while(!atom.isNull())
                    {
                        built.appendChild(atom);
                        atom = atom.nextSibling().toElement();
                    }
                }
            }
        }
    }

    QString xml_new_path = QString(QDir::homePath() + "/" + "Matching-Pursuit-Toolbox/%1_xml.dict").arg(ui->tb_DictName->text());
    QFile xml_new_file(xml_new_path);
    if(xml_new_file.open(QIODevice::WriteOnly))
    {
        QTextStream xml_stream(&xml_new_file);
        QXmlStreamWriter writer(&xml_new_file);
        writer.setAutoFormatting(true);
        writer.writeStartDocument();
        xml_stream << "\n";
        header.setAttribute("of_atoms", QString::number(summarize_atoms));
        xml_dict.appendChild(header);
        xml_dict.save(xml_stream, 4);
        writer.writeEndDocument();
    }
    xml_new_file.close();

    read_dicts();
    ui->list_NewDict->clear();
    ui->tb_DictName->clear();

    emit dict_saved();
}

//*************************************************************************************************************

void EditorWindow::keyReleaseEvent(QKeyEvent *event)
{
    if(event->key() == Qt::Key_Delete && ui->list_AllDict->hasFocus() && ui->list_AllDict->selectedItems().count() > 0)
    {
        deleteDicts();
    }
}

void EditorWindow::on_save_dicts()
{
    read_dicts();
}
