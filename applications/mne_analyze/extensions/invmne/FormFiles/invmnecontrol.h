#ifndef INVMNECONTROL_H
#define INVMNECONTROL_H

#include <QWidget>

namespace Ui {
class InvMNEControl;
}

class InvMNEControl : public QWidget
{
    Q_OBJECT

public:
    explicit InvMNEControl(QWidget *parent = 0);
    ~InvMNEControl();

private:
    Ui::InvMNEControl *ui;
};

#endif // INVMNECONTROL_H
