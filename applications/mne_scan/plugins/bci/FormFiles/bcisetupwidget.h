//=============================================================================================================
/**
 * @file     bcisetupwidget.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Viktor Klueber <Viktor.Klueber@tu-ilmenau.de>
 * @version  dev
 * @date     December, 2013
 *
 * @section  LICENSE
 *
 * Copyright (C) 2013, Christoph Dinh, Lorenz Esch, Viktor Klueber. All rights reserved.
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
 * @brief    Contains the declaration of the BCISetupWidget class.
 *
 */

#ifndef BCISETUPWIDGET_H
#define BCISETUPWIDGET_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================
#include <utils/filterTools/filterdata.h>
#include <Eigen/Dense>

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>
#include "../ui_bcisetup.h"


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE TMSIPlugin
//=============================================================================================================
using namespace Eigen;

namespace BCIPLUGIN
{


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class BCI;


//=============================================================================================================
/**
 * DECLARE CLASS TMSISetupWidget
 *
 * @brief The TMSISetupWidget class provides the TMSI configuration window.
 */
class BCISetupWidget : public QWidget
{
    Q_OBJECT
public:

    //=========================================================================================================
    /**
    * Constructs a BCISetupWidget which is a child of parent.
    *
    * @param [in] parent pointer to parent widget; If parent is 0, the new BCISetupWidget becomes a window. If parent is another widget, BCISetupWidget becomes a child window inside parent. BCISetupWidget is deleted when its parent is deleted.
    * @param [in] pBCI a pointer to the corresponding BCI.
    */
    BCISetupWidget(BCI* pBCI, QWidget *parent = 0);

    //=========================================================================================================
    /**
    * Destroys the BCISetupWidget.
    * All BCISetupWidget's children are deleted first. The application exits if BCISetupWidget is the main widget.
    */
    ~BCISetupWidget();

    //=========================================================================================================
    /**
    * Initializes the BCI's GUI properties.
    *
    */
    void initGui();

protected:
    //=========================================================================================================
    /**
    * Filters specified objects for wanted events -> intercepts events
    *
    */
    bool eventFilter(QObject *object, QEvent *event);

private:

    //=========================================================================================================
    /**
    * Shows the About Dialog
    *
    */
    void showAboutDialog();

    //=========================================================================================================
    /**
    * Sets general options made by the user
    *
    */
    void setGeneralOptions();

    //=========================================================================================================
    /**
    * Sets processing options made by the user
    *
    */
    void setProcessingOptions();

    //=========================================================================================================
    /**
    * Loads classification boundary for source level
    *
    */
    void changeLoadSourceBoundary();

    //=========================================================================================================
    /**
    * Loads classification boundary for sensor level
    *
    */
    void changeLoadSensorBoundary();

    //=========================================================================================================
    /**
    * Loads classification boundary for sensor level
    * [in] path location of the boundary file
    * [out] QVector<VectorXd> boundary coeff
    */
    QVector<VectorXd> readBoundaryInformation(QString path);

    //=========================================================================================================
    /**
    * Init selected feature list on sensor level
    *
    */
    void initSelectedFeaturesSensor();

    //=========================================================================================================
    /**
    * Sets feature selections made by the user on source and sensor level
    *
    */
    void setFeatureSelection();

    //=========================================================================================================
    /**
    * Sets filter options
    *
    */
    void setFilterOptions();

    //=========================================================================================================
    /**
    * Sets classification options
    *
    */
    void setClassificationOptions();


    BCI* m_pBCI;                                    /**< a pointer to corresponding BCI.*/

    QStringList m_vAvailableFeaturesSensor;         /**< QStringList holding available features to select on sensor level (electrodes).*/

    Ui::BCISetupClass ui;                           /**< the user interface for the BCISetupWidget.*/
};

} // NAMESPACE

#endif // TMSISETUPWIDGET_H
