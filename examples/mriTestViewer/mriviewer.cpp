#include "mriviewer.h"
#include "ui_mriviewer.h"

#include <QFileDialog>
#include <QGraphicsPixmapItem>

MriViewer::MriViewer(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MriViewer)
{
    ui->setupUi(this);
    scene = new QGraphicsScene(this);

    // load File
    filePath = "D:/Bilder/Freunde/Lorenz_Esch.jpg";
    loadFile(filePath);

    ui->graphicsView->setScene(scene);

    resize(QGuiApplication::primaryScreen()->availableSize()*4/5);
}

void MriViewer::loadFile(QString filePath)
{

    mriImage.load(filePath);
    mriPixmapItem = new QGraphicsPixmapItem(QPixmap::fromImage(mriImage));
    mriPixmapItem->setFlag(QGraphicsItem::ItemIsMovable);
    scene->addItem(mriPixmapItem);

    scaleSize = 1;

//    scene->setSceneRect(mriPixmapItem->pixmap().rect());
}

void MriViewer::on_openButton_clicked()
{
    filePath = QFileDialog::getOpenFileName(
                this,
                tr("Open File"),
                "",
                tr(defFileFormat)
                );

    loadFile(filePath);
}

void MriViewer::on_zoomInButton_clicked()
{
    double scaleUpFactor = 1.5;
    ui->graphicsView->scale(scaleUpFactor,scaleUpFactor);
    scaleSize = scaleUpFactor*scaleSize;
    qDebug() << "scale up to" << scaleSize;
}

void MriViewer::on_zoomOutButton_clicked()
{
    double scaleDownFactor = 0.75;
    ui->graphicsView->scale(scaleDownFactor,scaleDownFactor);
    scaleSize = scaleDownFactor*scaleSize;
    qDebug() << "scale down to" << scaleSize;
}

void MriViewer::on_resizeButton_clicked()
{
//    ui->graphicsView->resize();
    scaleSize = 1;
    qDebug() << "resize to normal";
}

MriViewer::~MriViewer()
{
    delete ui;
}

