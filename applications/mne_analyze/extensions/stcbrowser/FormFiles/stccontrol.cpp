#include "stccontrol.h"
#include "ui_stccontrol.h"

STCControl::STCControl(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::STCControl)
{
    ui->setupUi(this);
}

STCControl::~STCControl()
{
    delete ui;
}
