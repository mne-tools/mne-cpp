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
    struct Config{
        int minChannels;
        int maxChannels;
        int defChannels;
        int minSampFreq;
        int maxSampFreq;
        int defSampFreq;
    };

    explicit SignalGenSetupWidget(QWidget *parent = nullptr);
    ~SignalGenSetupWidget();

    void setConfig(const Config& config);
signals:
    void numChannelsChanged(int numChannels);
    void sampleFreqChanged(int sampleFreq);
private:
    Ui::SignalGenSetupWidget *ui;
};

#endif // SIGNALGENSETUPWIDGET_H
