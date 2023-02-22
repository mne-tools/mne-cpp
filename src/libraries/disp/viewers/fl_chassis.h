#ifndef FL_CHASSIS_H
#define FL_CHASSIS_H

#include <QWidget>
#include <memory>
#include <vector>
#include "fl_sensor.h"

namespace Ui {
class fl_chassis;
}

class fl_chassis : public QWidget
{
    Q_OBJECT

public:
    explicit fl_chassis(QWidget *parent = nullptr);
    ~fl_chassis();

private:
    Ui::fl_chassis *ui;
    std::vector<std::unique_ptr<fl_sensor>> sensors;
};

#endif // FL_CHASSIS_H
