//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2020-2026 MNE-CPP Authors
 *
 * @file     settingscontrollergui.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>
 * @since    0.1.0
 * @date     May, 2020
 * @brief     SettingsControllerGUI class declaration.
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

#include <QObject>
#include <QSharedPointer>
#include <QString>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <filesystem>
#include <string>
#include <vector>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace MNEANONYMIZE
{
class MainWindow;
}

//=============================================================================================================
// DEFINE NAMESPACE MNEANONYMIZE
//=============================================================================================================

namespace MNEANONYMIZE
{

//=============================================================================================================
// MNEANONYMIZE FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * Controller class of mne_anonymize_gui, the GUI version of mne_anonymize. It controls the declaration and management of a fiffanonymizer and a mainwindow object, and controls
 * communication between them.
 *
 * @brief Controller class for the GUI version of mne_anonymize.
 */
class SettingsControllerGui : public QObject, public SettingsControllerCl
{
    Q_OBJECT

public:
    typedef QSharedPointer<SettingsControllerGui> SPtr;            /**< Shared pointer type for SettingsControllerGUI. */
    typedef QSharedPointer<const SettingsControllerGui> ConstSPtr; /**< Const shared pointer type for SettingsControllerGUI. */

    //=========================================================================================================
    /**
     * Constructs a SettingsControllerGUI object, which shows the main window.
     *
     * @param[in] arguments    The command line, arguments[0] being the program, as UTF-8.
     */
    explicit SettingsControllerGui(const std::vector<std::string>& arguments);

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

    //=========================================================================================================
    /**
     * @return @p path as an absolute QString path.
     */
    static QString absoluteFilePath(const std::filesystem::path& path);

private:
    QSharedPointer<MainWindow> m_pWin; /**< A QShared pointer to the address of the MainWindow object containing the GUI.*/
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

} // namespace MNEANONYMIZE

#endif // MNEANONYMIZE_SETTINGSCONTROLLERGUI_H
