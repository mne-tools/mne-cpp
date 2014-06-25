#include "tmsiimpedancewidget.h"
#include "ui_tmsiimpedancewidget.h"

#include "../tmsi.h"

#include <QFileDialog>
#include <QString>

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace TMSIPlugin;

//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

TmsiImpedanceWidget::TmsiImpedanceWidget(TMSI* p_pTMSI, QWidget *parent)
: m_pTMSI(p_pTMSI)
, QWidget(parent)
, ui(new Ui::TmsiImpedanceWidget)
{
    ui->setupUi(this);

    ui->m_graphicsView_impedanceView->setScene(&m_scene);
}


//*************************************************************************************************************

TmsiImpedanceWidget::~TmsiImpedanceWidget()
{
    delete ui;
}


//*************************************************************************************************************

void TmsiImpedanceWidget::updateGraphicScene(MatrixXf &matValue)
{

}
