#include "invmnecontrol.h"
#include "ui_invmnecontrol.h"

InvMNEControl::InvMNEControl(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::InvMNEControl)
{
    ui->setupUi(this);
}

InvMNEControl::~InvMNEControl()
{
    delete ui;
}
