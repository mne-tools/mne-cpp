
//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "babymegsetupbabymegwidget.h"
#include "ui_babymegsetupbabymeg.h"

#include "babymegsquidcontroldgl.h"

#include "../babymeg.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace BabyMEGPlugin;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

BabyMEGSetupBabyMegWidget::BabyMEGSetupBabyMegWidget(BabyMEG* p_pBabyMEG, QWidget *parent)
: QWidget(parent)
, m_pBabyMEG(p_pBabyMEG)
, ui(new Ui::BabyMEGSetupBabyMegWidget)
{
    ui->setupUi(this);

    //SQUID Control
    connect(ui->m_qPushButton_SquidCtrl, &QPushButton::released, this, &BabyMEGSetupBabyMegWidget::SQUIDControlDialog);

}


//*************************************************************************************************************

BabyMEGSetupBabyMegWidget::~BabyMEGSetupBabyMegWidget()
{
    delete ui;
}


//*************************************************************************************************************

void BabyMEGSetupBabyMegWidget::SQUIDControlDialog()
{
    BabyMEGSQUIDControlDgl SQUIDCtrlDlg(m_pBabyMEG,this);
    SQUIDCtrlDlg.exec();
}
