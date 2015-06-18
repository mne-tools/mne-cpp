#include "tfplotwidget.h"
#include "ui_tfplotwidget.h"

tfplotwidget::tfplotwidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::tfplotwidget)
{
    ui->setupUi(this);
}

tfplotwidget::~tfplotwidget()
{
    delete ui;
}
