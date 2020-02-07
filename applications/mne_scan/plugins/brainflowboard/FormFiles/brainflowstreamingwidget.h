#ifndef BRAINFLOWSTREAMINGWIDGET_H
#define BRAINFLOWSTREAMINGWIDGET_H

#include <QWidget>

#include "brainflowboard.h"

namespace Ui {
class BrainFlowStreamingWidget;
}

class BrainFlowStreamingWidget : public QWidget
{
    Q_OBJECT

public:
    explicit BrainFlowStreamingWidget(BrainFlowBoard *board, QWidget *parent = nullptr);
    ~BrainFlowStreamingWidget();

    void configureBoard();

private:
    BrainFlowBoard *m_pBrainFlowBoard;
    Ui::BrainFlowStreamingWidget *ui;
};

#endif // BRAINFLOWSTREAMINGWIDGET_H
