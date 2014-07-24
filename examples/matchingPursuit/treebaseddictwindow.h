#ifndef TREEBASEDDICTWINDOW_H
#define TREEBASEDDICTWINDOW_H

#include <QWidget>

namespace Ui {
class TreebasedDictWindow;
}

class TreebasedDictWindow : public QWidget
{
    Q_OBJECT

public:
    explicit TreebasedDictWindow(QWidget *parent = 0);
    ~TreebasedDictWindow();

private slots:
    void on_btt_calc_treebased_clicked();

private:
    Ui::TreebasedDictWindow *ui;
};

#endif // TREEBASEDDICTWINDOW_H
