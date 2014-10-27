//=============================================================================================================
/**
* @file     averagewindow.h
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     October, 2014
*
* @section  LICENSE
*
* Copyright (C) 2014, Lorenz Esch, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Contains the declaration of the AverageWindow class.
*
*/

#ifndef AVERAGEWINDOW_H
#define AVERAGEWINDOW_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ui_averagewindow.h"
#include "utils/layoutloader.h"             //MNE-CPP utils
#include "../Utils/averagescene.h"          //MNE Browse Raw QT utils
#include "../Models/averagemodel.h"         //MNE Browse Raw QT utils
#include "../Delegates/averagedelegate.h"   //MNE Browse Raw QT utils
#include "../Utils/channelsceneitem.h"      //MNE Browse Raw QT utils
#include "../Utils/types.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDockWidget>
#include <QFileDialog>
#include <QStandardPaths>
#include <QSvgGenerator>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE MNEBrowseRawQt
//=============================================================================================================

namespace MNEBrowseRawQt
{


//*************************************************************************************************************
//=============================================================================================================
// DEFINE FORWARD DECLARATIONS
//=============================================================================================================

class LayoutScene;


/**
* DECLARE CLASS AverageWindow
*
* @brief The AverageWindow class provides a dock window for plotting averages.
*/
class AverageWindow : public QDockWidget
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
    * Constructs a AverageWindow which is a child of parent.
    *
    * @param [in] parent pointer to parent widget; If parent is 0, the new AverageWindow becomes a window. If parent is another widget, AverageWindow becomes a child window inside parent. AverageWindow is deleted when its parent is deleted.
    */
    AverageWindow(QWidget *parent = 0, QFile &file = QFile());

    //=========================================================================================================
    /**
    * Destroys the AverageWindow.
    * All AverageWindow's children are deleted first. The application exits if AverageWindow is the main widget.
    */
    ~AverageWindow();

    //=========================================================================================================
    /**
    * Returns the AverageModel of this window
    */
    AverageModel* getAverageModel();

    //=========================================================================================================
    /**
    * call this whenever the external channel selection manager changes
    *
    * * @param [in] selectedChannelItems list of selected graphic items
    */
    void channelSelectionManagerChanged(const QList<QGraphicsItem *> &selectedChannelItems);

    //=========================================================================================================
    /**
    * Scales the averaged data according to scaleMap
    *
    * @param [in] scaleMap map with all channel types and their current scaling value
    */
    void scaleAveragedData(const QMap<QString,double> &scaleMap);

private:
    //=========================================================================================================
    /**
    * Inits all QPushButtons in this window
    */
    void initButtons();

    //=========================================================================================================
    /**
    * inits the model view controller paradigm of this window
    *
    * @param [in] file holds the file which is to be loaded on startup
    */
    void initMVC(QFile &file);

    //=========================================================================================================
    /**
    * inits the table widgets of this window
    */
    void initTableViewWidgets();

    //=========================================================================================================
    /**
    * inits the average scene of this window
    */
    void initAverageSceneView();

    //=========================================================================================================
    /**
    * call this function whenever a selection was made in teh evoked data set list
    */
    void onSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);

    //=========================================================================================================
    /**
    * saves the current layout average plot as png or svg to file
    */
    void exportAverageLayoutPlot();

    Ui::AverageWindow*      ui;                     /**< Pointer to the qt designer generated ui class.*/

    AverageModel*           m_pAverageModel;        /**< the QAbstractTable average model being part of the model/view framework of Qt. */
    AverageDelegate*        m_pAverageDelegate;     /**< the QItemDelegateaverage delegate being part of the model/view framework of Qt. */
    AverageScene*           m_pAverageScene;        /**< holds the pointer to the average scene. */
};

} // NAMESPACE MNEBrowseRawQt

#endif // AVERAGEWINDOW_H
