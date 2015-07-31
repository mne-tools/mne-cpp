#ifndef TFPLOTWIDGET_H
#define TFPLOTWIDGET_H

#include <QWidget>
#include <QGraphicsScene>

namespace Ui {
class tfplotwidget;
}

class tfplotwidget : public QWidget
{
    Q_OBJECT

public:
    explicit tfplotwidget(QWidget *parent = 0);
    ~tfplotwidget();

    Ui::tfplotwidget *ui;
    QGraphicsScene tf_scene;
};

#endif // TFPLOTWIDGET_H
