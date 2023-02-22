#include "fl_chassis.h"
#include "ui_fl_chassis.h"


fl_chassis::fl_chassis(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::fl_chassis)
{
    ui->setupUi(this);

    for(int i = 0; i < 16; ++i){
        sensors.push_back(std::make_unique<fl_sensor>());
        sensors.back()->setLabel(QString::number(i+1));
        ui->sensor_frame->layout()->addWidget(sensors.back().get());
    }
    sensors.back()->setBlink(true);
}

fl_chassis::~fl_chassis()
{
    delete ui;
}
