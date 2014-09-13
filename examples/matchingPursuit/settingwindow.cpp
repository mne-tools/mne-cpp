#include "settingwindow.h"
#include "ui_settingwindow.h"
#include "mainwindow.h"

settingwindow::settingwindow(QWidget *parent) :    QWidget(parent),    ui(new Ui::settingwindow)
{    
    ui->setupUi(this);

    QSettings settings;
    move(settings.value("pos_settings", QPoint(200, 200)).toPoint());

   set_values();
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
    ui->sl_boost->setValue(-1 * (settings.value("boost", 0).toInt()));
    ui->cb_fixphase->setChecked(settings.value("fixPhase", false).toBool());
    ui->cb_show_warnings->setChecked(settings.value("show_warnings", true).toBool());
    ui->sb_adaptive_iteration->setValue(settings.value("adaptive_iterations", 1E3).toInt());
    ui->dsb_adaptive_reflection->setValue(settings.value("adaptive_reflection", 1.00).toDouble());
    ui->dsb_adaptive_expansion->setValue(settings.value("adaptive_expansion", 0.20).toDouble());
    ui->dsb_adaptive_contraction->setValue(settings.value("adaptive_contraction", 0.5).toDouble());
    ui->dsb_adaptive_fullcontraction->setValue(settings.value("adaptive_fullcontraction", 0.50).toDouble());
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
    settings.setValue("boost", abs(ui->sl_boost->value()));
    settings.setValue("fixPhase", ui->cb_fixphase->isChecked());
    settings.setValue("show_warnings", ui->cb_show_warnings->isChecked());
    settings.setValue("adaptive_iterations", ui->sb_adaptive_iteration->value());
    settings.setValue("adaptive_reflection", ui->dsb_adaptive_reflection->value());
    settings.setValue("adaptive_expansion", ui->dsb_adaptive_expansion->value());
    settings.setValue("adaptive_contraction", ui->dsb_adaptive_contraction->value());
    settings.setValue("adaptive_fullcontraction", ui->dsb_adaptive_fullcontraction->value());

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
