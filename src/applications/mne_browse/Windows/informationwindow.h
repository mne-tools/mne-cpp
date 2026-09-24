//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2014-2026 MNE-CPP Authors
 *
 * @file     informationwindow.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @date     August, 2014
 * @version  2.1.0
 * @brief    Contains the declaration of the InformationWindow class.
 */

#ifndef INFORMATIONWINDOW_H
#define INFORMATIONWINDOW_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ui_informationwindow.h"
#include "../Utils/info.h"

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDockWidget>
#include <QScrollBar>

#include <memory>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE MNEBROWSE
//=============================================================================================================

namespace MNEBROWSE
{

//*************************************************************************************************************
//=============================================================================================================
// DEFINE FORWARD DECLARATIONS
//=============================================================================================================

/**
 * DECLARE CLASS InformationWindow
 *
 * @brief The InformationWindow class provides a dockable InformationWindow window.
 */
class InformationWindow : public QDockWidget
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     * Constructs a InformationWindow dialog which is a child of parent.
     *
     * @param [in] parent pointer to parent widget; If parent is 0, the new InformationWindow becomes a window. If parent is another widget, InformationWindow becomes a child window inside parent. InformationWindow is deleted when its parent is deleted.
     */
    InformationWindow(QWidget *parent = 0);

    //=========================================================================================================
    /**
     * Destroys the InformationWindow.
     * All InformationWindow's children are deleted first. The application exits if InformationWindow is the main widget.
     */
    ~InformationWindow();

    //=========================================================================================================
    /**
     * Writes to MainWindow log.
     *
     * @param [in] logMsg message
     * @param [in] lgknd message kind; Message is formated depending on its kind.
     * @param [in] lglvl message level; Message is displayed depending on its level.
     */
    void writeToLog(const QString& logMsg, LogKind lgknd, LogLevel lglvl);

    //=========================================================================================================
    /**
     * Sets the log level
     *
     * @param [in] lvl message level; Message is displayed depending on its level.
     */
    void setLogLevel(LogLevel lvl);

private:
    std::unique_ptr<Ui::InformationWindowWidget> ui;        /**< Pointer to the qt designer generated ui class.*/

    //Log
    QTextBrowser*           m_pTextBrowser_Log;         /** A textbox being part of the log feature. */
    LogLevel                m_eLogLevelCurrent;         /**< Holds the current log level.*/

};

} // NAMESPACE MNEBROWSE

#endif // INFORMATIONWINDOW_H
