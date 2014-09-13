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
    // ToDo seb ctor
    explicit settingwindow(QWidget *parent = 0);
    ~settingwindow();
    void set_values();
private slots:
    void on_btt_close_clicked();
    void on_btt_cancel_clicked();
    void on_btt_max_it_default_clicked();
    void on_btt_reflection_default_clicked();
    void on_btt_expansion_default_clicked();
    void on_btt_contraction_default_clicked();
    void on_btt_full_contraction_default_clicked();
    // end

private:
    Ui::settingwindow *ui;
    void closeEvent(QCloseEvent * event);
};

#endif // SETTINGWINDOW_H
