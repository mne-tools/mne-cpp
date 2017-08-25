#ifndef DIPOLEFITCONTROL_H
#define DIPOLEFITCONTROL_H

#include <QWidget>

namespace Ui {
class DipoleFitControl;
}

class DipoleFitControl : public QWidget
{
    Q_OBJECT

public:
    explicit DipoleFitControl(QWidget *parent = 0);
    ~DipoleFitControl();

private:
    Ui::DipoleFitControl *ui;
};

#endif // DIPOLEFITCONTROL_H
