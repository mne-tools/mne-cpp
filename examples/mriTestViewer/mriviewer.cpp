#include "mriviewer.h"
#include "ui_mriviewer.h"

MriViewer::MriViewer(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MriViewer)
{
    ui->setupUi(this);

    scene = new QGraphicsScene(this);
    ui->graphicsView->setScene(scene);

    mriImage = new QImage();
    mriImage->load("D:/Bilder/Freunde/Lorenz_Esch.jpg");

    mriPixmap = QPixmap::fromImage(*mriImage);
    scene->addPixmap(mriPixmap);
}

MriViewer::~MriViewer()
{
    delete ui;
}
