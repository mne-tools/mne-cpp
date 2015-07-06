#include "mriviewer.h"
#include "ui_mriviewer.h"

#include <QFileDialog>

MriViewer::MriViewer(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MriViewer)
{
    ui->setupUi(this);

    filePath = "D:/Bilder/Freunde/Lorenz_Esch.jpg";
    loadFile(filePath);
}

void MriViewer::on_pushButton_clicked()
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

    scene = new QGraphicsScene(this);
    scene->addPixmap(mriPixmap);
    scene->setSceneRect(mriPixmap.rect());

    ui->graphicsView->setScene(scene);
}

MriViewer::~MriViewer()
{
    delete ui;
}
