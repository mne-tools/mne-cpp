//=============================================================================================================
/**
 * @file     settingscontrollerGUI.h
 * @author   Juan GPC <juangpc@gmail.com>
 * @since    0.1.0
 * @date     May, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020, Juan GPC. All rights reserved.
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
 * @brief     SettingsControllerGUI class declaration.
 *
 */

#ifndef MNEANONYMIZE_SETTINGSCONTROLLERGUI_H
#define MNEANONYMIZE_SETTINGSCONTROLLERGUI_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QMainWindow>
#include <QObject>
#include <QSharedPointer>
#include <QWidget>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
// DEFINE NAMESPACE MNEANONYMIZE
//=============================================================================================================

namespace MNEANONYMIZE {


//=============================================================================================================
// MNEANONYMIZE FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * Description of what this class is intended to do (in detail).
 *
 * @brief Brief description of this class.
 */
class SettingsControllerGUI : public QObject
{
    Q_OBJECT

public:
    typedef QSharedPointer<SettingsControllerGUI> SPtr;            /**< Shared pointer type for SettingsControllerGUI. */
    typedef QSharedPointer<const SettingsControllerGUI> ConstSPtr; /**< Const shared pointer type for SettingsControllerGUI. */

    //=========================================================================================================
    /**
    * Constructs a SettingsControllerGUI object.
    */
    explicit SettingsControllerGUI(QObject *parent = 0);

protected:

private:

signals:
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================


} // namespace MNEANONYMIZE

#endif // MNEANONYMIZE_SETTINGSCONTROLLERGUI_H

