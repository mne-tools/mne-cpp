//=============================================================================================================
/**
 * @file     bcifeaturewindow.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  1.0
 * @date     December, 2013
 *
 * @section  LICENSE
 *
 * Copyright (C) 2013, Christoph Dinh, Lorenz Esch. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *       following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 *       the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Massachusetts General Hospital nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MASSACHUSETTS GENERAL HOSPITAL BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * @brief    Contains the declaration of the BCIAboutWidget class.
 *
 */

#ifndef BCIFEATUREWINDOW_H
#define BCIFEATUREWINDOW_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QWidget>
#include "../ui_bcifeaturewindow.h"

//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE TMSIPlugin
//=============================================================================================================

namespace BCIPLUGIN
{

//*************************************************************************************************************
//=============================================================================================================
// TypeDefs
//=============================================================================================================

typedef QList< QList<double> > MyQList;

//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class BCI;


//=============================================================================================================
/**
 * DECLARE CLASS BCIFeatureWindow
 *
 * @brief The BCIFeatureWindow class provides a visualization tool for calculated features.
 */
class BCIFeatureWindow : public QWidget
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
    * Constructs a BCIFeatureWindow which is a child of parent.
    *
    * @param [in] parent pointer to parent widget; If parent is 0, the new BCIFeatureWindow becomes a window. If parent is another widget, BCIFeatureWindow becomes a child window inside parent. BCIFeatureWindow is deleted when its parent is deleted.
    * @param [in] pBCI a pointer to the corresponding BCI.
    */
    BCIFeatureWindow(BCI* pBCI, QWidget *parent = 0);

    //=========================================================================================================
    /**
    * Destroys the BCIFeatureWindow.
    * All BCIFeatureWindow's children are deleted first. The application exits if BCIFeatureWindow is the main widget.
    */
    ~BCIFeatureWindow();

    //=========================================================================================================
    /**
    * Initializes the BCI's GUI properties.
    *
    */
    void initGui();

protected:
    double boundaryValue(double x);

    void addBoundaryLineToScene();

    void paintFeaturesToScene(MyQList features, bool bTriggerActivated);

    BCI*                        m_pBCI;         /**< a pointer to corresponding BCI.*/
    QGraphicsScene              m_scene;        /**< QGraphicsScene used to add the features.*/

    double                      m_dFeatureMax;  /**< Max value for featrues - Used to scale the QGraphicsView.*/
    int                         m_iScale;       /**< Scaling value.*/
    Ui::BCIFeatureWindowClass   ui;             /**< the user interface for the BCIFeatureWindow.*/
};

} // NAMESPACE

#endif // BCIFEATUREWINDOW_H
