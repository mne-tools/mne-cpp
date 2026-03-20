//=============================================================================================================
/**
 * @file     settingscontrollergui.h
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

#include "settingscontrollercl.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================
namespace MNEANONYMIZE
{
    class MainWindow;
    class SettingsControllerCl;
}
//=============================================================================================================
// DEFINE NAMESPACE MNEANONYMIZE
//=============================================================================================================

namespace MNEANONYMIZE {

//=============================================================================================================
// MNEANONYMIZE FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * Controller class for mne_anonymize application when the GUI version of the application is called. This is generally
 * done by default. It controls the declaration and management of a fiffanonymizer and a mainwindow object, and controls
 * communication between them.
 *
 * @brief Controller class for the GUI version of mne_anonymize.
 */
class SettingsControllerGui : public SettingsControllerCl
{
    Q_OBJECT

public:
    typedef QSharedPointer<SettingsControllerGui> SPtr;            /**< Shared pointer type for SettingsControllerGUI. */
    typedef QSharedPointer<const SettingsControllerGui> ConstSPtr; /**< Const shared pointer type for SettingsControllerGUI. */

    //=========================================================================================================
    /**
    * Constructs a SettingsControllerGUI object.
    */
    explicit SettingsControllerGui(const QStringList& arguments);

public slots:
    //=========================================================================================================
    /**
     *
     * @brief Manages the event of the input file path (through the QLineEdit or throught he fileMenu) has been updated.
     *
     * @param[in] str String containing the path of the input file.
     *
     */
    void fileInChanged(const QString& strInFile);

    //=========================================================================================================
    /**
     *
     * @brief Manages the event of the input file path (through the QLineEdit or throught he fileMenu) has been updated.
     *
     * @param[in] str String containing the path of the input file.
     *
     */
    void fileOutChanged(const QString& strOutFile);

    //=========================================================================================================
    /**
     *
     * @brief Manages the event of the anonymize input file button being clicked.
     *
     */
    void executeAnonymizer();

    //=========================================================================================================
    /**
     * Runs the anonymizer.
     */
    int run() override;

private:
    //=========================================================================================================
    /**
     *
     * @brief Manages the event of the read input file, button being clicked.
     *
     */
    void readData();

    //=========================================================================================================
    /**
     *
     * @brief Sets the state of the Option controls according to the text command line call to open the gui.
     *
     */
    void initializeOptionsState();

    //=========================================================================================================
    /**
     *
     * @brief Sets up communication between this controller and the model (fiffanonymizer class) and the view (mainwindow class).
     * And also the communication between them.
     *
     */
    void setupCommunication();

private:
    QSharedPointer<MainWindow> m_pWin;      /**< A QShared pointer to the address of the MainWindow object containing the GUI.*/
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

} // namespace MNEANONYMIZE

#endif // MNEANONYMIZE_SETTINGSCONTROLLERGUI_H

