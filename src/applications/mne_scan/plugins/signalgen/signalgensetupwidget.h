#ifndef SIGNALGENSETUPWIDGET_H
#define SIGNALGENSETUPWIDGET_H

#include <QWidget>

namespace Ui {
class SignalGenSetupWidget;
}

class SignalGenSetupWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SignalGenSetupWidget(QWidget *parent = nullptr);
    ~SignalGenSetupWidget();

private:
    Ui::SignalGenSetupWidget *ui;
};

#endif // SIGNALGENSETUPWIDGET_H
