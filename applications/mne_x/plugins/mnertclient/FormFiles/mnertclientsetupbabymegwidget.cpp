#include "mnertclientsetupbabymegwidget.h"
#include "ui_mnertclientsetupbabymeg.h"


using namespace MneRtClientPlugin;

MneRtClientSetupBabyMegWidget::MneRtClientSetupBabyMegWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MneRtClientSetupBabyMegWidget)
{
    ui->setupUi(this);
}

MneRtClientSetupBabyMegWidget::~MneRtClientSetupBabyMegWidget()
{
    delete ui;
}
