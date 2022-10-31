//=============================================================================================================
/**
 * @file     spectrumview.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2018
 *
 * @section  LICENSE
 *
 * Copyright (C) 2018, Lorenz Esch. All rights reserved.
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
 * @brief    Declaration of the SpectrumView Class.
 *
 */

#ifndef SPECTRUMVIEW_H
#define SPECTRUMVIEW_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp_global.h"
#include "abstractview.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QPointer>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class QTableView;

namespace FIFFLIB {
    class FiffInfo;
}

//=============================================================================================================
// DEFINE NAMESPACE DISPLIB
//=============================================================================================================

namespace DISPLIB
{

//=============================================================================================================
// DISPLIB FORWARD DECLARATIONS
//=============================================================================================================

class FrequencySpectrumDelegate;
class FrequencySpectrumModel;

//=============================================================================================================
/**
 * DECLARE CLASS SpectrumView
 *
 * @brief The SpectrumView class provides a spectrum view
 */
class DISPSHARED_EXPORT SpectrumView : public AbstractView
{    
    Q_OBJECT

public:
    typedef QSharedPointer<SpectrumView> SPtr;              /**< Shared pointer type for SpectrumView. */
    typedef QSharedPointer<const SpectrumView> ConstSPtr;   /**< Const shared pointer type for SpectrumView. */

    //=========================================================================================================
    /**
     * Constructs a SpectrumView which is a child of parent.
     *
     * @param[in] parent    parent of widget.
     */
    SpectrumView(const QString& sSettingsPath = "",
                 QWidget* parent = 0,
                 Qt::WindowFlags f = Qt::Widget);

    //=========================================================================================================
    /**
     * Destroys the ArtifactSettingsView.
     */
    ~SpectrumView();

    //=========================================================================================================
    /**
     * Initializes the view based on the FiffInfo and scale type.
     *
     * @param[in] info          The FiffInfo.
     * @param[in] iScaleType    The scale type.
     */
    void init(QSharedPointer<FIFFLIB::FiffInfo> &info,
              int iScaleType);

    //=========================================================================================================
    /**
     * Adds data to the underlying model.
     *
     * @param[in] data          The new data.
     */
    void addData(const Eigen::MatrixXd &data);

    //=========================================================================================================
    /**
     * Sets the boundaries.
     *
     * @param[in] iLower    The lower boundary.
     * @param[in] iUpper    The upper boundary.
     */
    void setBoundaries(int iLower,
                       int iUpper);

    //=========================================================================================================
    /**
     * The event filter
     *
     * @param[in] watched.
     * @param[in] event.
     */
    virtual bool eventFilter(QObject* watched,
                             QEvent* event);

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

    //=========================================================================================================
    /**
     * Clears the view
     */
    void clearView();

protected:
    //=========================================================================================================
    /**
     * Update the views GUI based on the set GuiMode (Clinical=0, Research=1).
     *
     * @param[in] mode     The new mode (Clinical=0, Research=1).
     */
    void updateGuiMode(GuiMode mode);

    //=========================================================================================================
    /**
     * Update the views GUI based on the set ProcessingMode (RealTime=0, Offline=1).
     *
     * @param[in] mode     The new mode (RealTime=0, Offline=1).
     */
    void updateProcessingMode(ProcessingMode mode);

    QPointer<QTableView>                                m_pTableView;           /**< The QTableView being part of the model/view framework of Qt. */
    QPointer<DISPLIB::FrequencySpectrumDelegate>        m_pFSDelegate;          /**< Frequency spectrum delegate. */
    QPointer<DISPLIB::FrequencySpectrumModel>           m_pFSModel;             /**< Frequency spectrum model. */

signals:
    //=========================================================================================================
    /**
     * Signals for sending the mouse location to the delegate
     */
    void sendMouseLoc(int row,
                      int x,
                      int y,
                      QRect visRect);
};
} // NAMESPACE

#endif // SPECTRUMVIEW_H
