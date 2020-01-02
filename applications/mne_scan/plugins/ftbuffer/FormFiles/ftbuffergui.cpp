#include "ftbuffergui.h"
#include "ui_ftbuffergui.h"

ftbuffergui::ftbuffergui(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ftbuffergui)
{
    ui->setupUi(this);
}

ftbuffergui::~ftbuffergui()
{
    delete ui;
}
