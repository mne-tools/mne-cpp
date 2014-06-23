#ifndef DELETEMESSAGEBOX_H
#define DELETEMESSAGEBOX_H

#include <QDialog>

namespace Ui {
class DeleteMessageBox;
}

class DeleteMessageBox : public QDialog
{
    Q_OBJECT
    
public:
    explicit DeleteMessageBox(QWidget *parent = 0);
    ~DeleteMessageBox();
    
private slots:
    void on_btt_yes_clicked();

    void on_btt_No_clicked();

    void on_chb_NoMessageBox_toggled(bool checked);

private:
    Ui::DeleteMessageBox *ui;
};

#endif // DELETEMESSAGEBOX_H
