#include "babymegprojectdialog.h"
#include "ui_babymegprojectdialog.h"

#include "../babymeg.h"

#include <QFileDialog>
#include <QString>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace BabyMEGPlugin;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

BabyMEGProjectDialog::BabyMEGProjectDialog(BabyMEG* p_pBabyMEG, QWidget *parent)
: m_pBabyMEG(p_pBabyMEG)
, QDialog(parent)
, ui(new Ui::BabyMEGProjectDialog)
{
    ui->setupUi(this);

    //Fiff record file
    connect(ui->m_qPushButtonFiffRecordFile, &QPushButton::released, this, &BabyMEGProjectDialog::pressedFiffRecordFile);

    ui->m_qLineEditFiffRecordFile->setText(m_pBabyMEG->m_sRecordFile);

}


//*************************************************************************************************************

BabyMEGProjectDialog::~BabyMEGProjectDialog()
{
    delete ui;
}


//*************************************************************************************************************

void BabyMEGProjectDialog::pressedFiffRecordFile()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save Fiff Record File"), "", tr("Fiff Record File (*.fif)"));

    m_pBabyMEG->m_sRecordFile = fileName;

    ui->m_qLineEditFiffRecordFile->setText(fileName);
}
