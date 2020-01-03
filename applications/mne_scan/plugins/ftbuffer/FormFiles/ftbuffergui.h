#ifndef FTBUFFERGUI_H
#define FTBUFFERGUI_H

#include <QWidget>

namespace Ui {
class FtBufferGUI;
}

class FtBufferGUI : public QWidget
{
    Q_OBJECT

public:
    explicit FtBufferGUI(QWidget *parent = nullptr);
    ~FtBufferGUI();

private:
    Ui::FtBufferGUI *ui;
};
#endif // FTBUFFERGUI_H
