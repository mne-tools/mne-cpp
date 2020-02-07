#include "brainflowstreamingwidget.h"
#include "ui_brainflowstreamingwidget.h"

BrainFlowStreamingWidget::BrainFlowStreamingWidget(BrainFlowBoard *board, QWidget *parent) :
    QWidget(parent),
    m_pBrainFlowBoard(board),
    ui(new Ui::BrainFlowStreamingWidget)
{
    ui->setupUi(this);
    connect(ui->sendToBoard, &QPushButton::clicked, this, &BrainFlowStreamingWidget::configureBoard);
}

void BrainFlowStreamingWidget::configureBoard()
{
    m_pBrainFlowBoard->configureBoard(ui->stringToSend->text().toStdString());
}

BrainFlowStreamingWidget::~BrainFlowStreamingWidget()
{
    delete ui;
}
