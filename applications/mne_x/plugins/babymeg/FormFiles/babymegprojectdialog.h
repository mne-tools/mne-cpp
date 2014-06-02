#ifndef BABYMEGPROJECTDIALOG_H
#define BABYMEGPROJECTDIALOG_H

#include <QDialog>

namespace Ui {
class BabyMEGProjectDialog;
}


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE BabyMEGPlugin
//=============================================================================================================

namespace BabyMEGPlugin
{

//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class BabyMEG;


class BabyMEGProjectDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BabyMEGProjectDialog(BabyMEG* p_pBabyMEG, QWidget *parent = 0);
    ~BabyMEGProjectDialog();

    void pressedFiffRecordFile();   /**< Triggers file dialog to select record file.*/

private:
    BabyMEG* m_pBabyMEG;

    Ui::BabyMEGProjectDialog *ui;
};

} // NAMESPACE

#endif // BABYMEGPROJECTDIALOG_H
