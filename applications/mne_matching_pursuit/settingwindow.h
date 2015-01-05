#ifndef SETTINGWINDOW_H
#define SETTINGWINDOW_H

#include <QWidget>
#include <QStandardItem>

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
    //end
signals:
    void change_info_label();
private slots:
    void on_btt_close_clicked();
    void on_btt_cancel_clicked();
    void on_btt_max_it_default_clicked();
    void on_btt_reflection_default_clicked();
    void on_btt_expansion_default_clicked();
    void on_btt_contraction_default_clicked();
    void on_btt_full_contraction_default_clicked();
    void on_sl_boost_valueChanged(int value);
    void on_sl_boost_sliderPressed();
    void on_sl_boost_fixDict_valueChanged(int value);
    void on_sl_boost_fixDict_sliderPressed();
    void on_btt_delta_energy_default_clicked();
    void cb_selection_changed(const QModelIndex& topLeft, const QModelIndex& bottomRight);

    void on_pushButton_clicked();

private:
    Ui::settingwindow *ui;
    QStandardItem* cb_item;
    QStandardItemModel* cb_model;
    std::vector<QStandardItem*> cb_items;
    QMap<QString, QVariant> sel_signal_type_map;
    void fill_signal_type_combobox();
    void closeEvent(QCloseEvent * event);
};

#endif // SETTINGWINDOW_H
