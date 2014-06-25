#ifndef PROCESSDURATIONMESSAGEBOX_H
#define PROCESSDURATIONMESSAGEBOX_H

#include <QDialog>

namespace Ui {
class processdurationmessagebox;
}

class processdurationmessagebox : public QDialog
{
    Q_OBJECT
    
public:
    explicit processdurationmessagebox(QWidget *parent = 0);
    ~processdurationmessagebox();
    
private slots:
    void on_pushButton_clicked();

    void on_chb_NoMessageBox_toggled(bool checked);

private:
    Ui::processdurationmessagebox *ui;
};

#endif // PROCESSDURATIONMESSAGEBOX_H
