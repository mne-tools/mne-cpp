#ifndef MRIVIEWER_H
#define MRIVIEWER_H

#include <QWidget>
#include <QtCore>
#include <QtGui>
#include <QGraphicsScene>
#include <QPixmap>
#include <QImage>
#include <QString>
#include <QScrollArea>

#include <viewervars.h>
using namespace viewerVars;

namespace Ui {
    class MriViewer;
}

class MriViewer : public QWidget
{
    Q_OBJECT

public:
    explicit MriViewer(QWidget *parent = 0);
    ~MriViewer();

private slots:
    void on_openButton_clicked();
    void on_zoomInButton_clicked();
    void on_zoomOutButton_clicked();
    void on_resizeButton_clicked();

private:
    Ui::MriViewer *ui;
    QGraphicsScene *scene;
    QGraphicsPixmapItem *mriPixmapItem;
    QImage mriImage;
    QString filePath;
    const char *defFileFormat = "JPEG (*.jpg *.jpeg);;"
                            "PNG (*.png)";
//    QString defFileFormat = "MGH (*.mgh *.mgz)";
    void loadFile(QString filePath);

};

#endif // MRIVIEWER_H
