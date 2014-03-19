//MATCHING PURSUIT

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWidget>
#include <QLabel>
#include <QTableView>

class MainWindow : public QWidget
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = 0);
    QLabel * label;
    QTableView * tableView;

signals:

public slots:

};

#endif // MAINWINDOW_H
