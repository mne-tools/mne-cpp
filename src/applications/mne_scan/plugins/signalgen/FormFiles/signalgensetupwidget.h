#ifndef SIGNALGENSETUPWIDGET_H
#define SIGNALGENSETUPWIDGET_H

#include <QWidget>

#include "../signalgen.h"

namespace Ui {
class SignalGenSetupWidget;
}

class SignalGenSetupWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SignalGenSetupWidget(QWidget *parent = nullptr);
    ~SignalGenSetupWidget();

    void defineChannelSettings(int min, int max, int def);
    void defineSampleFreqSettings(int min, int max, int def);
    void defineBlockSettings(int min, int max, int def);
    void defineGeneratedFreqSettings(int min, int max, int def);
    void defineMode(SIGNALGENPLUGIN::SignalGen::Mode mode);

signals:
    void numChannelsChanged(int numChannels);
    void sampleFreqChanged(int sampleFreq);
    void blocksPerSecondChanged(int blocksPerSecond);
    void genFreqChanged(int genFreq);
    void modeChanged(SIGNALGENPLUGIN::SignalGen::Mode mode);
private:

    void updateUiState();

    Ui::SignalGenSetupWidget *ui;
};

#endif // SIGNALGENSETUPWIDGET_H
