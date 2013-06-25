
//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mnertclientsetupbabymegwidget.h"
#include "ui_mnertclientsetupbabymeg.h"

#include "mnertclientsquidcontroldgl.h"

#include "../mnertclient.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MneRtClientPlugin;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MneRtClientSetupBabyMegWidget::MneRtClientSetupBabyMegWidget(MneRtClient* p_pMneRtClient, QWidget *parent)
: m_pMneRtClient(p_pMneRtClient)
, QWidget(parent)
, ui(new Ui::MneRtClientSetupBabyMegWidget)
{
    ui->setupUi(this);

    //SQUID Control
    connect(ui->m_qPushButton_SquidCtrl, &QPushButton::released, this, &MneRtClientSetupBabyMegWidget::SQUIDControlDialog);

}


//*************************************************************************************************************

MneRtClientSetupBabyMegWidget::~MneRtClientSetupBabyMegWidget()
{
    delete ui;
}


//*************************************************************************************************************

void MneRtClientSetupBabyMegWidget::SQUIDControlDialog()
{
    mnertclientSQUIDControlDgl SQUIDCtrlDlg(m_pMneRtClient,this);
    SQUIDCtrlDlg.exec();
}
