//=============================================================================================================
/**
 * @file     controls.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @version  dev
 * @date     January, 2017
 *
 * @section  LICENSE
 *
 * Copyright (C) 2017, Lorenz Esch, Christoph Dinh. All rights reserved.
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
 * @brief    Controls class declaration.
 *
 */

#ifndef CONTROL_H
#define CONTROL_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp_global.h"


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QWidget>


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

QT_BEGIN_NAMESPACE
class QGroupBox;
class QComboBox;
QT_END_NAMESPACE


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE DISPLIB
//=============================================================================================================

namespace DISPLIB
{


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class DeepViewer;


//=============================================================================================================
/**
 * Controls Widget for Deep Viewer
 *
 * @brief Line Plot
 */
class DISPSHARED_EXPORT Controls : public QWidget
{
    Q_OBJECT
public:
    //=========================================================================================================
    /**
    * Constructs the Controls Widget without an attached view which is a child of parent
    *
    * @param [in] parent    The parent widget
    */
    Controls(QWidget *parent = Q_NULLPTR);

    //=========================================================================================================
    /**
    * Constructs the Controls Widget with an attached view which is a child of parent
    *
    * @param [in] v         The view which should be controled by this
    * @param [in] parent    The parent widgetarent widget
    */
    Controls(DeepViewer* v, QWidget *parent = Q_NULLPTR);

    //=========================================================================================================
    /**
    * Sets the associated viewer if none was set before
    *
    * @param [in] v     The viewer to set
    */
    void setDeepViewer(DeepViewer* v);

    //=========================================================================================================
    /**
    * Returns the configuration combo box
    *
    * @return The configuration combo box
    */
    QComboBox* getConfigurationComboBox();

signals:
    //=========================================================================================================
    /**
    * Emmitted when training of the network is requested
    */
    void requestTraining_signal();

private:
    //=========================================================================================================
    /**
    * Create the Network controls
    *
    * @param [in] parent    the group box where the Network controls should be attached to
    */
    void createNetworkControls(QWidget* parent);

    //=========================================================================================================
    /**
    * Create the Appearance controls
    *
    * @param [in] parent    the group box where the Appearance controls should be attached to
    */
    void createAppearanceControls(QWidget* parent);

    //=========================================================================================================
    /**
    * Create the View controls
    *
    * @param [in] parent    the group box where the View controls should be attached to
    */
    void createViewControls(QWidget* parent);

    //=========================================================================================================
    /**
    * Creates the layout
    */
    void createLayout();

    //=========================================================================================================
    /**
    * Request training after training button was pushed
    */
    void requestTraining();



private:
    QComboBox * m_pNetworkConfigs;              /**< The network configurations */
    DeepViewer* m_pDeepViewer;                  /**< The deep view which this control is connected to */
};

//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

} // NAMESPACE

#endif // CONTROL_H
