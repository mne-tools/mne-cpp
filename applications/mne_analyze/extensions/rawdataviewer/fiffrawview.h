//=============================================================================================================
/**
 * @file     fiffrawview.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Simon Heinke <Simon.Heinke@tu-ilmenau.de>;
 *           Gabriel Motta <gbmotta@mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2018
 *
 * @section  LICENSE
 *
 * Copyright (C) 2018, Lorenz Esch, Lars Debor, Simon Heinke, Gabriel Motta. All rights reserved.
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
 * @brief    Declaration of the FiffRawView Class.
 *
 */

#ifndef FIFFRAWVIEW_H
#define FIFFRAWVIEW_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rawdataviewer_global.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QWidget>
#include <QPointer>
#include <QMap>
#include <QMenu>
#include <QScroller>

//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace ANSHAREDLIB {
    class FiffRawViewModel;
}

class QTableView;

//=============================================================================================================
// DEFINE NAMESPACE RAWDATAVIEWEREXTENSION
//=============================================================================================================

namespace RAWDATAVIEWEREXTENSION {

//=============================================================================================================
// RAWDATAVIEWEREXTENSION FORWARD DECLARATIONS
//=============================================================================================================

class FiffRawViewDelegate;

//=============================================================================================================
/**
 * TableView for Fiff data.
 */
class RAWDATAVIEWERSHARED_EXPORT FiffRawView : public QWidget
{
    Q_OBJECT

public:
    typedef QSharedPointer<FiffRawView> SPtr;            /**< Shared pointer type for FiffRawView. */
    typedef QSharedPointer<const FiffRawView> ConstSPtr; /**< Const shared pointer type for FiffRawView. */

    //=========================================================================================================
    /**
     * Constructs a FiffRawView which is a child of parent.
     *
     * @param [in] parent    The parent of widget.
     */
    FiffRawView(QWidget *parent = nullptr);

    //=========================================================================================================
    /**
     * Destructor.
     */
    virtual ~FiffRawView();

    //=========================================================================================================
    /**
     * Setup the model view controller.
     */
    void initMVCSettings(const QSharedPointer<ANSHAREDLIB::FiffRawViewModel> &pModel,
                         const QSharedPointer<FiffRawViewDelegate>& pDelegate);

    //=========================================================================================================
    /**
     * Broadcast channel scaling
     *
     * @param [in] scaleMap QMap with scaling values which is to be broadcasted to the model.
     */
    void setScalingMap(const QMap<qint32, float>& scaleMap);

    //=========================================================================================================
    /**
     * Set the signal color.
     *
     * @param [in] signalColor  The new signal color.
     */
    void setSignalColor(const QColor& signalColor);

    //=========================================================================================================
    /**
     * Broadcast the background color changes made in the QuickControl widget
     *
     * @param [in] backgroundColor  The new background color.
     */
    void setBackgroundColor(const QColor& backgroundColor);

    //=========================================================================================================
    /**
     * Sets new zoom factor
     *
     * @param [in] zoomFac  time window size;
     */
    void setZoom(double zoomFac);

    //=========================================================================================================
    /**
     * Sets new time window size
     *
     * @param [in] T  time window size;
     */
    void setWindowSize(int T);

    //=========================================================================================================
    /**
     * distanceTimeSpacerChanged changes the distance of the time spacers
     *
     * @param value the new distance for the time spacers
     */
    void setDistanceTimeSpacer(int value);

    //=========================================================================================================
    /**
     * Call this slot whenever you want to make a screenshot current view.
     *
     * @param[in] imageType  The current iamge type: png, svg.
     */
    void onMakeScreenshot(const QString& imageType);

    //=========================================================================================================
    /**
     * Brings up a menu for interacting with data annotations
     * @param[in] pos   Position on screen where the menu will show up
     */
    void customContextMenuRequested(const QPoint &pos);

    //=========================================================================================================
    /**
     * Create a new annotation
     *
     * @param[in] con   Whether a new annotaton should be created;
     */
    void addTimeMark(bool con);

    //=========================================================================================================
    /**
     * Controls toggling of annotation data
     *
     * @param[in] iToggle   Sets toggle: 0 - don't show, 1+ - show
     */
    void toggleDisplayEvent(const int& iToggle);

    //=========================================================================================================
    /**
     * Triggers a redraw of the data viewer
     */
    void updateView();

signals:

    //=========================================================================================================
    /**
     * Emits sample number to be added aan annotation
     *
     * @param [in] iSample      sample number to be added
     */
    void sendSamplePos(int iSample);

private:
    //=========================================================================================================
    /**
     * resizeEvent reimplemented virtual function to handle resize events of the data dock window
     */
    void resizeEvent(QResizeEvent* event);

    bool eventFilter(QObject *object, QEvent *event);

    QPointer<QTableView>                                m_pTableView;                   /**< Pointer to table view ui element */

    QSharedPointer<ANSHAREDLIB::FiffRawViewModel>       m_pModel;                       /**< Pointer to associated Model */

    QSharedPointer<FiffRawViewDelegate>                 m_pDelegate;                    /**< Pointer to associated Delegate */

    QMap<qint32,float>                                  m_qMapChScaling;                /**< Channel scaling values. */

    QColor                                              m_backgroundColor;              /**< Current background color. */

    float                                               m_fDefaultSectionSize;          /**< Default row height */
    float                                               m_fZoomFactor;                  /**< Zoom factor */
    float                                               m_fLastClickedPoint;            /**< Stores last clicked point on screen */

    qint32                                              m_iT;                           /**< Display window size in seconds */

    QScroller*                                          m_pKineticScroller;             /**< Used for kinetic scrolling through data view */

signals:
    void tableViewDataWidthChanged(int iWidth);
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

} // NAMESPACE RAWDATAVIEWEREXTENSION

#endif // FIFFRAWVIEW_H
