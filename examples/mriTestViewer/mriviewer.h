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
* @brief     MriViewer class declaration.
*
*/

#ifndef MRIVIEWER_H
#define MRIVIEWER_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fs/fs_global.h>
#include <fs/blendian.h>
#include <fs/mri.h>
#include <fs/mgh.h>

#include <disp/imagesc.h>
#include <disp/plot.h>
#include <disp/rtplot.h>

#include <viewervars.h>

//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QWidget>
#include <QtCore>
#include <QtGui>
#include <QGraphicsScene>
#include <QPixmap>
#include <QImage>
#include <QString>
#include <QScrollArea>
#include <QFileDialog>
#include <QGraphicsPixmapItem>

//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE YOURNAMESPACE
//=============================================================================================================

namespace Ui {
    class MriViewer;
}

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace ViewerVars;

//=============================================================================================================
/**
* This class provides a GUI widget to display mri data as a qImage in a qGraphicsScene.
*
* @brief display mri data in widget.
*/
class MriViewer : public QWidget
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
    * Constructs a MriViewer object.
    */
    explicit MriViewer(QWidget *parent = 0);

    //=========================================================================================================
    /**
    * Destroys the MriViewer object.
    */
    ~MriViewer();

private slots:
    void on_openButton_clicked();
    void on_zoomInButton_clicked();
    void on_zoomOutButton_clicked();
    void on_resizeButton_clicked();

    void on_clearButton_clicked();

private:
    Ui::MriViewer *ui;
    QGraphicsScene *scene;
    QGraphicsPixmapItem *mriPixmapItem;
    QImage mriImage; /**< qImage holding the mri slice which should be visualized */
    QString filePath; /**< absolute file path of mri file which is loaded */
    const char *defFileFormat = "JPEG (*.jpg *.jpeg);;"
                                "PNG (*.png);;"
                                "MGH (*.mgh)"; /**< suffix definition of loadable file formats */
//    QString defFileFormat = "MGH (*.mgh *.mgz)"; // mgz not working yet
    void loadImageFile(QString filePath);
    void loadMriFile(QString filePath);

};

#endif // MRIVIEWER_H
