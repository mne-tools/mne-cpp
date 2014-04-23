#ifndef BABYMEGPROJECT_H
#define BABYMEGPROJECT_H

#include <QDialog>

namespace Ui {
class BabyMEGProjectDialog;
}

class BabyMEGProjectDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BabyMEGProjectDialog(QWidget *parent = 0);
    ~BabyMEGProjectDialog();

    void pressedFiffRecordFile();   /**< Triggers file dialog to select record file.*/

private:
    Ui::BabyMEGProjectDialog *ui;
};

#endif // BABYMEGPROJECT_H
