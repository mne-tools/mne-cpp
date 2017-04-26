#include "dipolefitcontrol.h"
#include "ui_dipolefitcontrol.h"

DipoleFitControl::DipoleFitControl(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DipoleFitControl)
{
    ui->setupUi(this);
}

DipoleFitControl::~DipoleFitControl()
{
    delete ui;
}
