#include "brainflowstreamingwidget.h"
#include "ui_brainflowstreamingwidget.h"

BrainFlowStreamingWidget::BrainFlowStreamingWidget(BrainFlowBoard *board, QWidget *parent) :
    QWidget(parent),
    brainFlowBoard(board),
    ui(new Ui::BrainFlowStreamingWidget)
{
    ui->setupUi(this);

    // notch filter
    ui->notchFilter->addItem("None");
    ui->notchFilter->addItem("50");
    ui->notchFilter->addItem("60");
    ui->notchFilter->setCurrentIndex(1);
    // bandpass
    ui->bandPassFilter->addItem("None");
    ui->bandPassFilter->addItem("1-50");
    ui->bandPassFilter->addItem("1-30");
    ui->bandPassFilter->addItem("1-4");
    ui->bandPassFilter->addItem("4-8");
    ui->bandPassFilter->addItem("8-13");
    ui->bandPassFilter->addItem("13-30");
    ui->bandPassFilter->setCurrentIndex(2);
    // filter type
    ui->filterType->addItem("BUTTERWORTH");
    ui->filterType->addItem("CHEBYSHEV_TYPE_1");
    ui->filterType->addItem("BESSEL");
    ui->filterType->setCurrentIndex(0);
    // filter order
    ui->filterOrder->setRange(1, 7);
    ui->filterOrder->setValue(4);
    // ripple
    ui->ripple->setText("0.5");

    connect(ui->sendToBoard, &QPushButton::clicked, this, &BrainFlowStreamingWidget::configureBoard);
    connect(ui->submitFilters, &QPushButton::clicked, this, &BrainFlowStreamingWidget::applyFilters);
}

void BrainFlowStreamingWidget::configureBoard()
{
    brainFlowBoard->configureBoard(ui->stringToSend->text().toStdString());
}

void BrainFlowStreamingWidget::applyFilters()
{
    int notchFreq = 0;
    int bandStart = 0;
    int bandStop = 0;
    int filterType = ui->filterType->currentIndex();
    int order = ui->filterOrder->text().toInt();
    double ripple = ui->ripple->text().toDouble();
    if (ui->notchFilter->currentIndex() != 0)
    {
        notchFreq = ui->notchFilter->currentText().toInt();
    }
    if (ui->bandPassFilter->currentIndex() != 0)
    {
        QStringList splitted = ui->bandPassFilter->currentText().split("-");
        bandStart = splitted[0].toInt();
        bandStop = splitted[1].toInt();
    }
    brainFlowBoard->applyFilters(notchFreq, bandStart, bandStop, filterType, order, ripple);
}

BrainFlowStreamingWidget::~BrainFlowStreamingWidget()
{
    delete ui;
}
