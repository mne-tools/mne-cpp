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
//    void normalSize();
//    void fitToWindow();

    void on_resizeButton_clicked();

private:
    // basic data objects
    Ui::MriViewer *ui;
    QGraphicsScene *scene;
    QGraphicsPixmapItem *mriPixmapItem;
//    QPixmap mriPixmap;
    QImage mriImage;

    // file information
    QString filePath;
    const char *defFileFormat = "JPEG (*.jpg *.jpeg);;"
                            "PNG (*.png)";
//    QString defFileFormat = "MGH (*.mgh *.mgz)";
    void loadFile(QString filePath);

    // gui interaction
    QScrollArea *scrollArea;

    double scaleSize;
    double origXSize;
    double origZSize;

//    QAction *zoomInAct;
//    QAction *zoomOutAct;
//    QAction *zoomInAct;
//    QAction *zoomOutAct;
};

#endif // MRIVIEWER_H
