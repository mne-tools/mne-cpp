#ifndef CUSTOMVIEW_H
#define CUSTOMVIEW_H

#include <QGraphicsView>
#include <QtWidgets>
#include <QWidget>

class CustomView : public QGraphicsView
{
public:
    CustomView(QWidget* parent=0);
    ~CustomView();
protected:
    virtual void wheelEvent(QWheelEvent *event);
};

#endif // CUSTOMVIEW_H
