//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     covariancesetupwidget.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     February, 2013
 * @brief    Contains the declaration of the CovarianceSetupWidget class.
 */

#ifndef COVARIANCESETUPWIDGET_H
#define COVARIANCESETUPWIDGET_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ui_covariancesetupwidget.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>

//=============================================================================================================
// DEFINE NAMESPACE COVARIANCEPLUGIN
//=============================================================================================================

namespace COVARIANCEPLUGIN
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class Covariance;

//=============================================================================================================
/**
 * DECLARE CLASS CovarianceSetupWidget
 *
 * @brief The CovarianceSetupWidget class provides the CovarianceToolbox configuration window.
 */
class CovarianceSetupWidget : public QWidget
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     * Constructs a CovarianceSetupWidget which is a child of parent.
     *
     * @param[in] toolbox a pointer to the corresponding Covariance toolbox.
     * @param[in] parent pointer to parent widget; If parent is 0, the new CovarianceSetupWidget becomes a window. If parent is another widget, CovarianceSetupWidget becomes a child window inside parent. CovarianceSetupWidget is deleted when its parent is deleted.
     */
    CovarianceSetupWidget(Covariance* toolbox, QWidget *parent = 0);

    //=========================================================================================================
    /**
     * Destroys the CovarianceSetupWidget.
     * All CovarianceSetupWidget's children are deleted first. The application exits if CovarianceSetupWidget is the main widget.
     */
    ~CovarianceSetupWidget();

private:

    Covariance* m_pCovariance;        /**< Holds a pointer to corresponding Covariance.*/

    Ui::CovarianceSetupWidgetClass ui;   /**< Holds the user interface for the CovarianceSetupWidget.*/
};
} // NAMESPACE

#endif // COVARIANCESETUPWIDGET_H
