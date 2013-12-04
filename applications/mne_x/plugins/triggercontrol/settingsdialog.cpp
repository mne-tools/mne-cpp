#include "settingsdialog.h"
#include "ui_settingsdialog.h"

Settingsdialog::Settingsdialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Settingsdialog)
{
    ui->setupUi(this);
}

Settingsdialog::~Settingsdialog()
{
    delete ui;
}
