//=============================================================================================================
/**
 * @file     dummyyourwidget.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Viktor Klueber <Viktor.Klueber@tu-ilmenau.de>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     January, 2016
 *
 * @section  LICENSE
 *
 * Copyright (C) 2016, Christoph Dinh, Viktor Klueber, Lorenz Esch. All rights reserved.
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
 * @brief    Contains the declaration of the DummyYourWidget class.
 *
 */

#ifndef DUMMYYOURWIDGET_H
#define DUMMYYOURWIDGET_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QWidget>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace Ui{
    class DummyYourWidgetGui;
}

//=============================================================================================================
// DEFINE NAMESPACE DUMMYTOOLBOXPLUGIN
//=============================================================================================================

namespace DUMMYTOOLBOXPLUGIN
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * DECLARE CLASS DummyYourWidget
 *
 * @brief The DummyToolbox class provides a dummy widget.
 */
class DummyYourWidget : public QWidget
{
    Q_OBJECT

public:
    typedef QSharedPointer<DummyYourWidget> SPtr;         /**< Shared pointer type for DummyYourWidget. */
    typedef QSharedPointer<DummyYourWidget> ConstSPtr;    /**< Const shared pointer type for DummyYourWidget. */

    //=========================================================================================================
    /**
     * Constructs a DummyToolbox.
     */
    explicit DummyYourWidget(const QString& sSettingsPath = "",
                             QWidget *parent = 0);

    //=========================================================================================================
    /**
     * Destroys the DummyToolbox.
     */
    ~DummyYourWidget();

private:
    //=========================================================================================================
    /**
     * Saves all important settings of this view via QSettings.
     */
    void saveSettings();

    //=========================================================================================================
    /**
     * Loads and inits all important settings of this view via QSettings.
     */
    void loadSettings();

    Ui::DummyYourWidgetGui*     m_pUi;              /**< The UI class specified in the designer. */
    QString                     m_sSettingsPath;    /**< The settings path to store the GUI settings to. */

};
}   //namespace

#endif // DUMMYYOURWIDGET_H
