//=============================================================================================================
/**
* @file     settingswindow.cpp
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
* @brief    Definition of the DeleteMesssageBox Class.
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================
#include "settingwindow.h"
#include "ui_settingwindow.h"
#include "mainwindow.h"
#include "QToolTip"

//*************************************************************************************************************************************

settingwindow::settingwindow(QWidget *parent) :    QWidget(parent),    ui(new Ui::settingwindow)
{    
    ui->setupUi(this);

    QSettings settings;
    move(settings.value("pos_settings", QPoint(200, 200)).toPoint());
    this->cb_model = new QStandardItemModel;
    connect(this->cb_model, SIGNAL(dataChanged ( const QModelIndex&, const QModelIndex&)), this, SLOT(cb_selection_changed(const QModelIndex&, const QModelIndex&)));

    set_values();

    cb_item = nullptr;
}

//*****************************************************************************************************************

settingwindow::~settingwindow()
{
    delete ui;
}

//*****************************************************************************************************************

void settingwindow::set_values()
{
    QSettings settings;
    fill_signal_type_combobox();
    ui->chb_fixphase->setChecked(settings.value("fixPhase", false).toBool());
    ui->chb_show_warnings->setChecked(settings.value("show_warnings", true).toBool());
    ui->chb_show_infos->setChecked(settings.value("show_infos", true).toBool());
    ui->chb_sort_results->setChecked(settings.value("sort_results", true).toBool());
    ui->sb_adaptive_iteration->setValue(settings.value("adaptive_iterations", 1E3).toInt());
    ui->dsb_adaptive_reflection->setValue(settings.value("adaptive_reflection", 1.00).toDouble());
    ui->dsb_adaptive_expansion->setValue(settings.value("adaptive_expansion", 0.20).toDouble());
    ui->dsb_adaptive_contraction->setValue(settings.value("adaptive_contraction", 0.5).toDouble());
    ui->dsb_adaptive_fullcontraction->setValue(settings.value("adaptive_fullcontraction", 0.50).toDouble());
    ui->sb_div_dict->setValue(settings.value("pdict_count", 8).toInt());
    ui->chb_color_scheme->setChecked(settings.value("pastell_colors", false).toBool());
    ui->dsb_delta_energy->setValue(settings.value("delta_energy", 0.0005).toDouble());
    ui->sl_boost_fixDict->setValue(-1 * (settings.value("boost_fixDict", 0).toInt()));
    ui->sl_boost->setValue(-1 * (settings.value("boost", 0).toInt()));
    ui->chb_phys_params->setChecked(settings.value("show_phys_params", false).toBool());
    ui->chb_trial_separation->setChecked(settings.value("trial_separation", false).toBool());

    if(settings.value("boost").toInt()== 0)
        ui->sl_boost->setToolTip("only 1 channel consulted");
    else
        ui->sl_boost->setToolTip(QString("consulted channels: %1% ").arg(settings.value("boost").toInt()));

    if(settings.value("boost_fixDict").toInt()== 0)
        ui->sl_boost_fixDict->setToolTip("only 1 channel consulted");
    else
        ui->sl_boost_fixDict->setToolTip(QString("consulted channels: %1% ").arg(settings.value("boost_fixDict").toInt()));
}

//*************************************************************************************************************************************

void settingwindow::closeEvent(QCloseEvent * event)
{
    Q_UNUSED(event);
    QSettings settings;
    settings.setValue("pos_settings", pos());
}

//*****************************************************************************************************************

void settingwindow::on_btt_close_clicked()
{
    QSettings settings;
    settings.setValue("channel_names", sel_signal_type_map);
    settings.setValue("boost", std::abs(ui->sl_boost->value()));
    settings.setValue("boost_fixDict", std::abs(ui->sl_boost_fixDict->value()));
    settings.setValue("fixPhase", ui->chb_fixphase->isChecked());
    settings.setValue("adaptive_iterations", ui->sb_adaptive_iteration->value());
    settings.setValue("adaptive_reflection", ui->dsb_adaptive_reflection->value());
    settings.setValue("adaptive_expansion", ui->dsb_adaptive_expansion->value());
    settings.setValue("adaptive_contraction", ui->dsb_adaptive_contraction->value());
    settings.setValue("adaptive_fullcontraction", ui->dsb_adaptive_fullcontraction->value());
    settings.setValue("delta_energy", ui->dsb_delta_energy->value());
    settings.setValue("show_infos", ui->chb_show_infos->isChecked());
    settings.setValue("show_warnings", ui->chb_show_warnings->isChecked());
    settings.setValue("sort_results", ui->chb_sort_results->isChecked());
    settings.setValue("pdict_count", ui->sb_div_dict->value());
    settings.setValue("pastell_colors", ui->chb_color_scheme->isChecked());
    settings.setValue("show_phys_params", ui->chb_phys_params->isChecked());
    settings.setValue("trial_separation", ui->chb_trial_separation->isChecked());

    emit change_info_label();

    close();
}

//*****************************************************************************************************************

void settingwindow::on_btt_cancel_clicked()
{
    close();
}

//*****************************************************************************************************************

void settingwindow::on_btt_max_it_default_clicked()
{
    ui->sb_adaptive_iteration->setValue(1000);
}

//*****************************************************************************************************************

void settingwindow::on_btt_reflection_default_clicked()
{
    ui->dsb_adaptive_reflection->setValue(1);
}

//*****************************************************************************************************************

void settingwindow::on_btt_expansion_default_clicked()
{
    ui->dsb_adaptive_expansion->setValue(0.2);
}

//*****************************************************************************************************************

void settingwindow::on_btt_contraction_default_clicked()
{
    ui->dsb_adaptive_contraction->setValue(0.5);
}

//*****************************************************************************************************************

void settingwindow::on_btt_full_contraction_default_clicked()
{
    ui->dsb_adaptive_fullcontraction->setValue(0.5);
}

//*****************************************************************************************************************

void settingwindow::on_sl_boost_valueChanged(int value)
{
    if(value == 0)
    {
        QToolTip::showText(ui->sl_boost->mapToGlobal(QPoint(-20 , -35)), "only 1 channel consulted");
        ui->sl_boost->setToolTip("only 1 channel is consulted");
    }
    else
    {
        QToolTip::showText(ui->sl_boost->mapToGlobal(QPoint(-20, -35)), QString("consulted channels: %1% ").arg(-1 * value));
        ui->sl_boost->setToolTip(QString("consulted channels: %1% ").arg(-1 * value));
    }
}

//*****************************************************************************************************************

void settingwindow::on_sl_boost_sliderPressed()
{
    if(ui->sl_boost->value() == 0)
        QToolTip::showText(ui->sl_boost->mapToGlobal(QPoint(-20, -35)), "only 1 channel consulted");
    else
        QToolTip::showText(ui->sl_boost->mapToGlobal(QPoint(-20, -35)), QString("consulted channels: %1% ").arg(-1 *ui->sl_boost->value()));
}

//*****************************************************************************************************************

void settingwindow::on_sl_boost_fixDict_valueChanged(int value)
{
    if(value == 0)
    {
        QToolTip::showText(ui->sl_boost_fixDict->mapToGlobal(QPoint(-20 , -35)), "only 1 channel consulted");
        ui->sl_boost_fixDict->setToolTip("only 1 channel is consulted");
    }
    else
    {
        QToolTip::showText(ui->sl_boost_fixDict->mapToGlobal(QPoint(-20, -35)), QString("consulted channels: %1% ").arg(-1 * value));
        ui->sl_boost_fixDict->setToolTip(QString("consulted channels: %1% ").arg(-1 * value));
    }
}

//*****************************************************************************************************************

void settingwindow::on_sl_boost_fixDict_sliderPressed()
{
    if(ui->sl_boost_fixDict->value() == 0)
        QToolTip::showText(ui->sl_boost_fixDict->mapToGlobal(QPoint(-20, -35)), "only 1 channel consulted");
    else
        QToolTip::showText(ui->sl_boost_fixDict->mapToGlobal(QPoint(-20, -35)), QString("consulted channels: %1% ").arg(-1 *ui->sl_boost_fixDict->value()));
}

//*****************************************************************************************************************

void settingwindow::on_btt_delta_energy_default_clicked()
{
    ui->dsb_delta_energy->setValue(0.0005);
}

//*****************************************************************************************************************

void settingwindow::fill_signal_type_combobox()
{
    this->cb_items.clear();
    this->cb_model->clear();

    QSettings settings;
    QMap<QString, QVariant> chn_name_map;
    for(qint32 m = 0; m < 4; m++)
        chn_name_map.insert(QString("MEG;EEG;STI;EOG").split(';').at(m), true);
    sel_signal_type_map = chn_name_map = settings.value("channel_names", chn_name_map).toMap();
    qint32 j = 0;
    QMapIterator<QString, QVariant> i(chn_name_map);
    while (i.hasNext())
    {
        i.next();
        //channel item
        this->cb_item = new QStandardItem;
        this->cb_item->setText(QString(i.key()));
        this->cb_item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
        if(i.value().toBool())
            this->cb_item->setData(Qt::Checked, Qt::CheckStateRole);
        else
            this->cb_item->setData(Qt::Unchecked, Qt::CheckStateRole);
        this->cb_items.push_back(this->cb_item);
        this->cb_model->appendRow(this->cb_item);
        j++;
    }

    ui->cb_signal_types->setModel(this->cb_model);
}

//*****************************************************************************************************************

void settingwindow::cb_selection_changed(const QModelIndex& topLeft, const QModelIndex& bottomRight)
{
    Q_UNUSED(bottomRight);

    QStandardItem* cb_item = this->cb_items[topLeft.row()];
    //QMap<QString, QVariant> chn_name_map;

    if(cb_item->checkState() == Qt::Checked)
        sel_signal_type_map[cb_item->text()].setValue(true);
    else
        sel_signal_type_map[cb_item->text()].setValue(false);

    //sel_signal_type_map = chn_name_map;
}

//*****************************************************************************************************************

void settingwindow::on_pushButton_clicked()
{
    QSettings settings;
    settings.clear();
    set_values();
}

//*****************************************************************************************************************
