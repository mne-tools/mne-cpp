#ifndef FTBUFFERGUI_H
#define FTBUFFERGUI_H

#include <QWidget>

namespace Ui {
class ftbuffergui;
}

class ftbuffergui : public QWidget
{
    Q_OBJECT

public:
    explicit ftbuffergui(QWidget *parent = nullptr);
    ~ftbuffergui();

private:
    Ui::ftbuffergui *ui;
};

#endif // FTBUFFERGUI_H
