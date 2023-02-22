#ifndef FL_RACK_H
#define FL_RACK_H

#include <QWidget>

namespace Ui {
class fl_rack;
}

class fl_rack : public QWidget
{
    Q_OBJECT

public:
    explicit fl_rack(QWidget *parent = nullptr);
    ~fl_rack();

private:
    Ui::fl_rack *ui;
};

#endif // FL_RACK_H
