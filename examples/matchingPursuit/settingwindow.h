#ifndef SETTINGWINDOW_H
#define SETTINGWINDOW_H

#include <QWidget>

namespace Ui {
class settingwindow;
}

class settingwindow : public QWidget
{
    Q_OBJECT

public:
    explicit settingwindow(QWidget *parent = 0);
    ~settingwindow();

private slots:
    void on_btt_close_clicked();

private:
    Ui::settingwindow *ui;
};

#endif // SETTINGWINDOW_H
