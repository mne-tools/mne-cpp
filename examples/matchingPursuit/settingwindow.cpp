#include "settingwindow.h"
#include "ui_settingwindow.h"
#include "mainwindow.h"

settingwindow::settingwindow(QWidget *parent) :    QWidget(parent),    ui(new Ui::settingwindow)
{    
    ui->setupUi(this);


    QSettings settings;
    move(settings.value("pos_settings", QPoint(200, 200)).toPoint());


    ui->cb_boost->setChecked(settings.value("isBoost", true).toBool());
    ui->cb_fixphase->setChecked(settings.value("fixPhase", false).toBool());
    ui->cb_show_warnings->setChecked(settings.value("show_warnings", true).toBool());

    ui->sb_adaptive_iteration->setValue(settings.value("adaptive_iterations", 1E3).toInt());
    //ui->sb_fix_iteration->setValue(settings.value("fix_iterations", 1E3).toInt());

    ui->dsb_adaptive_reflection->setValue(settings.value("adaptive_reflection", 1.00).toDouble());
    //ui->dsb_fix_reflection->setValue(settings.value("fix_reflection", 1.00).toDouble());

    ui->dsb_adaptive_expansion->setValue(settings.value("adaptive_expansion", 0.20).toDouble());
    //ui->dsb_fix_expansion->setValue(settings.value("fix_expansion", 0.20).toDouble());

    ui->dsb_adaptive_contraction->setValue(settings.value("adaptive_contraction", 0.5).toDouble());
    //ui->dsb_fix_contraction->setValue(settings.value("fix_contraction", 0.50).toDouble());

    ui->dsb_adaptive_fullcontraction->setValue(settings.value("adaptive_fullcontraction", 0.50).toDouble());
    //ui->dsb_fix_fullcontraction->setValue(settings.value("fix_fullcontraction", 0.50).toDouble());
}

//*****************************************************************************************************************

settingwindow::~settingwindow()
{
    delete ui;
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
    settings.setValue("isBoost", ui->cb_boost->isChecked());
    settings.setValue("fixPhase", ui->cb_fixphase->isChecked());
    settings.setValue("show_warnings", ui->cb_show_warnings->isChecked());

    settings.setValue("adaptive_iterations", ui->sb_adaptive_iteration->value());
    //settings.setValue("fix_iterations", ui->sb_fix_iteration->value());

    settings.setValue("adaptive_reflection", ui->dsb_adaptive_reflection->value());
    //settings.setValue("fix_reflection", ui->dsb_fix_reflection->value());

    settings.setValue("adaptive_expansion", ui->dsb_adaptive_expansion->value());
    //settings.setValue("fix_expansion", ui->dsb_fix_expansion->value());

    settings.setValue("adaptive_contraction", ui->dsb_adaptive_contraction->value());
    //settings.setValue("fix_contraction", ui->dsb_fix_contraction->value());

    settings.setValue("adaptive_fullcontraction", ui->dsb_adaptive_fullcontraction->value());
    //settings.setValue("fix_fullcontraction", ui->dsb_fix_fullcontraction->value());

    close();
}

//*****************************************************************************************************************

void settingwindow::on_btt_cancel_clicked()
{
    close();
}
