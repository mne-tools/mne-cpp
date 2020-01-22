#ifndef BRAINFLOWSETUPWIDGET_H
#define BRAINFLOWSETUPWIDGET_H

#include <QWidget>

#include "brainflowboard.h"

namespace Ui {
class BrainFlowSetupWidget;
}

class BrainFlowSetupWidget : public QWidget
{
    Q_OBJECT

public:
    explicit BrainFlowSetupWidget(BrainFlowBoard *board, QWidget *parent = nullptr);
    ~BrainFlowSetupWidget();

    void prepareSession();
    void releaseSession();

private:
    BrainFlowBoard *brainFlowBoard;
    Ui::BrainFlowSetupWidget *ui;
};

#endif // BRAINFLOWSETUPWIDGET_H
