#ifndef QUICKCONTROLWIDGET_H
#define QUICKCONTROLWIDGET_H

#include <QWidget>
#include <QMouseEvent>

#include "ui_quickcontrolwidget.h"

//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE XDISPLIB
//=============================================================================================================

namespace XDISPLIB
{

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================


class QuickControlWidget : public QWidget
{
    Q_OBJECT

public:
    QuickControlWidget(QWidget *parent = 0);
    ~QuickControlWidget();

protected:
    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void resizeEvent(QResizeEvent *event);
    QRegion roundedRect(const QRect& rect, int r);

private:
    QPoint dragPosition;
    Ui::QuickControlWidget *ui;
};

} // NAMESPACE

#endif // QUICKCONTROLWIDGET_H
