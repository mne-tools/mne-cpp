//=============================================================================================================
/**
* @file     mriviewer.h
* @author   Carsten Boensel <carsten.boensel@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     July, 2015
*
* @section  LICENSE
*
* Copyright (C) 2015, Carsten Boensel and Matti Hamalainen. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that
* the following conditions are met:
*     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
*       following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
*       the following disclaimer in the documentation and/or other materials provided with the distribution.
*     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
*       to endorse or promote products derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*
* @brief    Implementation of the MriViewer Class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mriviewer.h"
#include "ui_mriviewer.h"

//*******

using namespace FSLIB;
using namespace Eigen;
using namespace DISPLIB;

//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MriViewer::MriViewer(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MriViewer)
{
    ui->setupUi(this);
    scene = new QGraphicsScene(this);

    // load File -> todo: replace with mri file
    filePath = "D:/Bilder/Freunde/Lorenz_Esch.jpg";
    loadImageFile(filePath);

    //------------

    // initialize vars to call loadMGH function
    QString fName = "D:/Repos/mne-cpp/bin/MNE-sample-data/subjects/sample/mri/orig/001.mgh";

    VectorXi slices(3); // indices of the sclices (z dimension) to read
    slices << 99, 100, 101;

    int frame = 0; // time frame index, negativ values are vectors
    bool headerOnly = false;

    Mri mri = Mgh::loadMGH(fName, slices, frame, headerOnly);

    qDebug() << "\nRead" << mri.slices.size() << "slices.\n";
    quint16 sliceIdx = 150;
    MatrixXd mat = mri.slices[sliceIdx]; // chosen slice index

    ImageSc imagesc(mat);
    imagesc.setColorMap("Bone");
    QPixmap mriSlice = imagesc.getPixmap();

    mriPixmapItem = new QGraphicsPixmapItem(mriSlice);
    mriPixmapItem->setFlag(QGraphicsItem::ItemIsMovable);
    scene->addItem(mriPixmapItem);

    //------------

    ui->graphicsView->setScene(scene);
    ui->graphicsView->installEventFilter(this);

    resize(QGuiApplication::primaryScreen()->availableSize()*4/5);
}

//*************************************************************************************************************

void MriViewer::loadImageFile(QString filePath)
{
    mriImage.load(filePath);
    mriPixmapItem = new QGraphicsPixmapItem(QPixmap::fromImage(mriImage));
    mriPixmapItem->setFlag(QGraphicsItem::ItemIsMovable);
    scene->addItem(mriPixmapItem);
}

//*************************************************************************************************************

void MriViewer::loadMriFile(QString filePath)
{
    mriImage.load(filePath);

    QString fName = "D:/Repos/mne-cpp/bin/MNE-sample-data/subjects/sample/mri/orig/001.mgh";

    VectorXi slices(3); // indices of the sclices (z dimension) to read
    slices << 99, 100, 101;

    int frame = 0; // time frame index, negativ values are vectors
    bool headerOnly = false;

    Mri mri = Mgh::loadMGH(fName, slices, frame, headerOnly);

    qDebug() << "\nRead" << mri.slices.size() << "slices.\n";
    quint16 sliceIdx = 150;
    MatrixXd mat = mri.slices[sliceIdx]; // chosen slice index

    ImageSc imagesc(mat);
    imagesc.setColorMap("Bone");
    QPixmap mriSlice = imagesc.getPixmap();

    mriPixmapItem = new QGraphicsPixmapItem(mriSlice);
    mriPixmapItem->setFlag(QGraphicsItem::ItemIsMovable);
    scene->addItem(mriPixmapItem);
}

//*************************************************************************************************************

void MriViewer::on_openButton_clicked()
{
    filePath = QFileDialog::getOpenFileName(
                this,
                tr("Open File"),
                "",
                tr(defFileFormat)
                );

    loadImageFile(filePath);
}

//*************************************************************************************************************

void MriViewer::on_zoomInButton_clicked()
{
    ui->graphicsView->scale(scaleFactor,scaleFactor);
    scaleSize = scaleSize*scaleFactor;
    qDebug() << "scale up to" << scaleSize;
}

//*************************************************************************************************************

void MriViewer::on_zoomOutButton_clicked()
{
    ui->graphicsView->scale(1/scaleFactor,1/scaleFactor);
    scaleSize = scaleSize/scaleFactor;
    qDebug() << "scale down to" << scaleSize;
}

//*************************************************************************************************************

void MriViewer::on_resizeButton_clicked()
{
    ui->graphicsView->scale(1/scaleSize,1/scaleSize);
    scaleSize = 1;
    qDebug() << "resize to original zoom";
}

//*************************************************************************************************************

void MriViewer::on_clearButton_clicked()
{
    scene->clear();
    qDebug() << "clear scene from items";
}

//*************************************************************************************************************

MriViewer::~MriViewer()
{
    delete ui;
}
