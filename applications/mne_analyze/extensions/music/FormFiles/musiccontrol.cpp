#include "musiccontrol.h"
#include "ui_musiccontrol.h"

MusicControl::MusicControl(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MusicControl)
{
    ui->setupUi(this);
}

MusicControl::~MusicControl()
{
    delete ui;
}
