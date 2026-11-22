//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2015-2026 MNE-CPP Authors
 *
 * @file     gusbampsetupwidget.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Viktor Klueber <Viktor.Klueber@tu-ilmenau.de>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     November, 2015
 * @brief    Contains the declaration of the GUSBAmpSetupWidget class.
 */

#ifndef GUSBAMPSETUPWIDGET_H
#define GUSBAMPSETUPWIDGET_H

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QWidget>

//=============================================================================================================
// DEFINE NAMESPACE GUSBAMPPLUGIN
//=============================================================================================================

namespace GUSBAMPPLUGIN
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class GUSBAmp;

//=============================================================================================================
/**
 * @brief Settings of the GUSBAmp plugin: serial numbers, inputs, rate, block size, trigger line and simulation.
 */
class GUSBAmpSetupWidget : public QWidget
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     * Shows the settings of @p pGUSBAmp and writes every change back to it.
     *
     * @param[in] pGUSBAmp   The plugin.
     * @param[in] parent     Parent widget.
     */
    explicit GUSBAmpSetupWidget(GUSBAmp* pGUSBAmp, QWidget* parent = nullptr);
};

} // NAMESPACE GUSBAMPPLUGIN

#endif // GUSBAMPSETUPWIDGET_H
