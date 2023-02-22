#include "fl_sensor.h"
#include "ui_fl_sensor.h"
#

fl_sensor::fl_sensor(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::fl_sensor)
{
    color = Qt::red;

    blink_brush = QBrush(Qt::transparent);
    on_brush = QBrush(color);

    ui->setupUi(this);
    ui->graphicsView->setStyleSheet("background:transparent");
    ui->label->setText("Hello");
    m_pScene = std::make_unique<QGraphicsScene>();
    ui->graphicsView->setScene(m_pScene.get());

    circle = m_pScene->addEllipse(0,0,this->width()/3,this->width()/3, QPen(Qt::black), on_brush);
    blinkstate = false;

    blinktime = 200;
}

fl_sensor::~fl_sensor()
{
    delete ui;
}

void fl_sensor::setLabel(const QString& label)
{
    ui->label->setText(label);
}


void fl_sensor::resizeEvent(QResizeEvent *event)
{
    auto bounds = m_pScene->itemsBoundingRect();
    bounds.setWidth(bounds.width() * 1.2);
    bounds.setHeight(bounds.height() * 1.2);
    ui->graphicsView->fitInView(bounds, Qt::KeepAspectRatio);

    QWidget::resizeEvent(event);
}

void fl_sensor::setBlink(bool state)
{
    if(state){
        turnOnBlink();
    } else {
        turnOffBlink();
    }
}


void fl_sensor::turnOnBlink()
{
    connect(&t, &QTimer::timeout, this, &fl_sensor::handleBlink, Qt::UniqueConnection);
    t.start(blinktime);
}

void fl_sensor::turnOffBlink()
{
    t.stop();
    circle->setBrush(on_brush);
}

void fl_sensor::handleBlink()
{
    if(blinkstate){
        circle->setBrush(blink_brush);
        blinkstate = false;
    } else {
        circle->setBrush(on_brush);
        blinkstate = true;
    }
}
