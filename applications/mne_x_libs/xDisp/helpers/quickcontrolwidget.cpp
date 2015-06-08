#include "quickcontrolwidget.h"


using namespace XDISPLIB;



QuickControlWidget::QuickControlWidget(QWidget *parent) :
    QWidget(parent, Qt::FramelessWindowHint | Qt::WindowSystemMenuHint),
    ui(new Ui::QuickControlWidget)
{
    ui->setupUi(this);

    //this->setWindowFlags(Qt::FramelessWindowHint);
}

QuickControlWidget::~QuickControlWidget()
{
    delete ui;
}

void QuickControlWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        dragPosition = event->globalPos() - frameGeometry().topLeft();
        event->accept();
    }
}

void QuickControlWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton) {
        move(event->globalPos() - dragPosition);
        event->accept();
    }
}


void QuickControlWidget::resizeEvent(QResizeEvent * /* event */)
{
    setMask(roundedRect(QRect(0,0,width(),height()),10));
}

QRegion QuickControlWidget::roundedRect(const QRect& rect, int r)
{
    QRegion region;
    // middle and borders
    region += rect.adjusted(r, 0, -r, 0);
    region += rect.adjusted(0, r, 0, -r);
    // top left
    QRect corner(rect.topLeft(), QSize(r*2, r*2));
    region += QRegion(corner, QRegion::Ellipse);
    // top right
    corner.moveTopRight(rect.topRight());
    region += QRegion(corner, QRegion::Ellipse);
    // bottom left
    corner.moveBottomLeft(rect.bottomLeft());
    region += QRegion(corner, QRegion::Ellipse);
    // bottom right
    corner.moveBottomRight(rect.bottomRight());
    region += QRegion(corner, QRegion::Ellipse);
    return region;
}
