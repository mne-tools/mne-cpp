#ifndef FL_SENSOR_H
#define FL_SENSOR_H

#include <QWidget>
#include <QGraphicsScene>
#include <memory>
#include <QTimer>
#include <QGraphicsEllipseItem>

namespace Ui {
class fl_sensor;
}

class fl_sensor : public QWidget
{
    Q_OBJECT

public:
    explicit fl_sensor(QWidget *parent = nullptr);
    ~fl_sensor();

    void setLabel(const QString& label);

    void setBlink(bool state);

protected:
    virtual void resizeEvent(QResizeEvent *event);
private:
    void turnOnBlink();
    void turnOffBlink();
    void handleBlink();


    int blinktime;
    Ui::fl_sensor *ui;
    std::unique_ptr<QGraphicsScene> m_pScene;
    QGraphicsEllipseItem* circle;
    QTimer t;
    QColor color;
    QBrush blink_brush;
    QBrush on_brush;
    bool blinkstate;
};

#endif // FL_SENSOR_H
