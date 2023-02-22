#include "fl_rack.h"
#include "ui_fl_rack.h"

#include "fl_chassis.h"

fl_rack::fl_rack(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::fl_rack)
{
    ui->setupUi(this);

    ui->frame->layout()->addWidget(new fl_chassis());
}

fl_rack::~fl_rack()
{
    delete ui;
}
