#include "settingswidget.h"
#include "ui_settingswidget.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace TriggerControlPlugin;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

SettingsWidget::SettingsWidget(QWidget *parent)
: QDialog(parent)
, ui(new Ui::SettingsWidget)
{
    ui->setupUi(this);

    connect(ui->m_qPushButton_apply, &QPushButton::released,  this, &SettingsWidget::close);
 //   connect(ui->pushButton_apply, SIGNAL(released()),this, SLOT(apply()));

 //   connect(ui->comboBox_port, SIGNAL(currentIndexChanged(int)),this, SLOT(showPortInfo(int)));
 //   connect(ui->baudRateBox, SIGNAL(currentIndexChanged(int)),
  //          this, SLOT(checkCustomBaudRatePolicy(int)));

  //  fillPortsParameters();
 //   fillPortsInfo();

 //   updateSettings();
}

SettingsWidget::~SettingsWidget()
{
    delete ui;
}
