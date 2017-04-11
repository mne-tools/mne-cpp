#ifndef MUSICCONTROL_H
#define MUSICCONTROL_H

#include <QWidget>

namespace Ui {
class MusicControl;
}

class MusicControl : public QWidget
{
    Q_OBJECT

public:
    explicit MusicControl(QWidget *parent = 0);
    ~MusicControl();

private:
    Ui::MusicControl *ui;
};

#endif // MUSICCONTROL_H
