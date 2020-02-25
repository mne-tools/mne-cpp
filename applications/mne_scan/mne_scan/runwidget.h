//=============================================================================================================
/**
 * @file     runwidget.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @version  dev
 * @date     February, 2013
 *
 * @section  LICENSE
 *
 * Copyright (C) 2013, Christoph Dinh. All rights reserved.
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
 * @brief    Contains declaration of RunWidget class.
 *
 */

#ifndef RUNDWIDGET_H
#define RUNDWIDGET_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QWidget>
#include <QSharedPointer>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class QTabWidget;
class QScrollArea;

//=============================================================================================================
// DEFINE NAMESPACE MNESCAN
//=============================================================================================================

namespace MNESCAN
{

//=============================================================================================================
/**
 * DECLARE CLASS RunWidget
 *
 * @brief The RunWidget class provides the central widget for the run mode.
 */
class RunWidget : public QWidget //not inherit from QTabWidget cause resizeEvent is slower
{
    Q_OBJECT
public:
    typedef QSharedPointer<RunWidget> SPtr;               /**< Shared pointer type for RunWidget. */
    typedef QSharedPointer<const RunWidget> ConstSPtr;    /**< Const shared pointer type for RunWidget. */

    //=========================================================================================================
    /**
     * Constructs a RunWidget which is a child of parent.
     *
     * @param [in] dispWidget pointer to widget which holds the real time displays.
     * @param [in] parent pointer to parent widget; If parent is 0, the new RunWidget becomes a window. If parent is another widget, RunWidget becomes a child window inside parent. RunWidget is deleted when its parent is deleted.
     */
    RunWidget(QWidget* dispWidget, QWidget* parent = 0);

    //=========================================================================================================
    /**
     * Destroys the RunWidget.
     */
    virtual ~RunWidget();

    //=========================================================================================================
    /**
     * Adds a tab with the given page and label to the RunWidget, and returns the index of the tab in the tab bar.
     *
     * @param [in] page pointer to widget which should be added in a new tab.
     * @param [in] label if the tab's label contains an ampersand, the letter following the ampersand is used as a shortcut for the tab, e.g. if the label is "Bro&wse" then Alt+W becomes a shortcut which will move the focus to this tab.
     * @return the index of the tab in the tab bar.
     */
    int addTab(QWidget* page, const QString& label);

    //=========================================================================================================
    /**
     * Sets zoom of display tab to standard.
     */
    void setStandardZoom();

    //=========================================================================================================
    /**
     * Zooms display tab with given factor in vertical direction.
     *
     * @param [in] factor zoom factor for vertical direction.
     */
    void zoomVert(float factor);

signals:

    //=========================================================================================================
    /**
     * This signal is emitted when RunWidget is closed. Used when full screen mode is terminated.
     */
    void displayClosed();

protected:

    //=========================================================================================================
    /**
     * This event handler is called when RunWidget is resized.
     */
    virtual void resizeEvent(QResizeEvent* );

    //=========================================================================================================
    /**
     * This event handler is called when RunWidget is closed.
     */
    virtual void closeEvent(QCloseEvent* );

private:
    QTabWidget*     m_pTabWidgetMain;   /**< Holds the tab widget. */

    QScrollArea*    m_pScrollArea;      /**< Holds the scroll area inside the display tab. */

};

}//NAMESPACE

#endif // RUNDWIDGET_H
