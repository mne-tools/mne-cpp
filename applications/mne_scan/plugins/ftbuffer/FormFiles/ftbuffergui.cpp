#include "ftbuffergui.h"
#include "ui_ftbuffergui.h"

FtBufferGUI::FtBufferGUI(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FtBufferGUI)
{
    ui->setupUi(this);
}

FtBufferGUI::~FtBufferGUI()
{
    delete ui;
}
