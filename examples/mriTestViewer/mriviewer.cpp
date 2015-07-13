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

    // load File -> todo: replace with mri file
    filePath = "D:/Bilder/Freunde/Lorenz_Esch.jpg";
    loadFile(filePath);

    ui->graphicsView->setScene(scene);
    ui->graphicsView->installEventFilter(this);

    resize(QGuiApplication::primaryScreen()->availableSize()*4/5);
}

void MriViewer::loadFile(QString filePath)
{

    mriImage.load(filePath);
    mriPixmapItem = new QGraphicsPixmapItem(QPixmap::fromImage(mriImage));
    mriPixmapItem->setFlag(QGraphicsItem::ItemIsMovable);
    scene->clear();
    scene->addItem(mriPixmapItem);
    scaleSize = 1;
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
    ui->graphicsView->scale(scaleFactor,scaleFactor);
    scaleSize = scaleSize*scaleFactor;
    qDebug() << "scale up to" << scaleSize;
}

void MriViewer::on_zoomOutButton_clicked()
{
    ui->graphicsView->scale(1/scaleFactor,1/scaleFactor);
    scaleSize = scaleSize/scaleFactor;
    qDebug() << "scale down to" << scaleSize;
}

void MriViewer::on_resizeButton_clicked()
{
    ui->graphicsView->scale(1/scaleSize,1/scaleSize);
    scaleSize = 1;
    qDebug() << "resize to original zoom";
}

MriViewer::~MriViewer()
{
    delete ui;
}
