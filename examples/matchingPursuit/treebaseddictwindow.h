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
    QString _treebased_dict_name;

private slots:
    void on_btt_calc_treebased_clicked();
    void on_tb_treebased_dict_name_editingFinished();
    void on_btt_call_tree_creator_clicked();

private:
    Ui::TreebasedDictWindow *ui;
};

#endif // TREEBASEDDICTWINDOW_H
