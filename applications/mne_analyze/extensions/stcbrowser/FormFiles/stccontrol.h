#ifndef STCCONTROL_H
#define STCCONTROL_H

#include <QWidget>

namespace Ui {
class STCControl;
}

class STCControl : public QWidget
{
    Q_OBJECT

public:
    explicit STCControl(QWidget *parent = 0);
    ~STCControl();

private:
    Ui::STCControl *ui;
};

#endif // STCCONTROL_H
