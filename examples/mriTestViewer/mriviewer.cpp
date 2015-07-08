#include "mriviewer.h"
#include "ui_mriviewer.h"

#include <QFileDialog>

MriViewer::MriViewer(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MriViewer)
{
    ui->setupUi(this);
    scene = new QGraphicsScene(this);

    // load File
    filePath = "D:/Bilder/Freunde/Lorenz_Esch.jpg";
    loadFile(filePath);

    // set scrollArea
//    scrollArea = new QScrollArea;
//    scrollArea->setBackgroundRole(QPalette::Dark);
//    scene->addWidget(scrollArea);

    ui->graphicsView->setScene(scene);
    resize(QGuiApplication::primaryScreen()->availableSize()*4/5);
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

void MriViewer::loadFile(QString filePath)
{
    mriImage = new QImage();
    mriImage->load(filePath);

    mriPixmap = QPixmap::fromImage(*mriImage);

    scene->addPixmap(mriPixmap);
//    scene->setSceneRect(mriPixmap.rect());
}

void MriViewer::on_zoomInButton_clicked()
{
    ui->graphicsView->scale(1.5,1.5);
}

void MriViewer::on_zoomOutButton_clicked()
{
    ui->graphicsView->scale(0.75,0.75);
}

MriViewer::~MriViewer()
{
    delete ui;
}
