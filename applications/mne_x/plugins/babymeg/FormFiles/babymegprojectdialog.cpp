#include "babymegprojectdialog.h"
#include "ui_babymegprojectdialog.h"

#include <QFileDialog>
#include <QString>


BabyMEGProjectDialog::BabyMEGProjectDialog(QWidget *parent)
: QDialog(parent)
, ui(new Ui::BabyMEGProjectDialog)
{
    ui->setupUi(this);

    //Fiff record file
    connect(ui->m_qPushButtonFiffRecordFile, &QPushButton::released, this, &BabyMEGProjectDialog::pressedFiffRecordFile);

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

    ui->m_qLineEdit_FiffRecordFile->setText(fileName);
}
