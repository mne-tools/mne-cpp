#ifndef MRIVIEWER_H
#define MRIVIEWER_H

#include <QWidget>
#include <QtCore>
#include <QtGui>
#include <QGraphicsScene>
#include <QPixmap>
#include <QImage>

namespace Ui {
class MriViewer;
}

class MriViewer : public QWidget
{
    Q_OBJECT

public:
    explicit MriViewer(QWidget *parent = 0);
    ~MriViewer();

private:
    Ui::MriViewer *ui;
    QGraphicsScene *scene;
    QPixmap mriPixmap;
    QImage *mriImage;
};

#endif // MRIVIEWER_H
