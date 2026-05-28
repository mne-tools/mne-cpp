//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *   Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 *   Gabriel Motta <gabrielbenmotta@gmail.com>
 *   Juan GPC <jgarciaprieto@mgh.harvard.edu>
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file spectrumview.h
 * @since July 2018
 * @brief Per-channel frequency-spectrum heat-map TableView (rows = channels, columns = frequencies).
 *
 * SpectrumView wraps a @c QTableView around a
 * @ref FrequencySpectrumModel and paints each row with
 * @ref FrequencySpectrumDelegate so the live FFT amplitudes appear as
 * a rolling colour band per channel. The visible frequency range is
 * controlled by @ref SpectrumSettingsView.
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
 * @brief Per-channel frequency-spectrum heat-map QTableView (rows = channels, columns = freq bins).
 *
 * Wraps a @c QTableView around a @ref FrequencySpectrumModel and
 * paints each row with @ref FrequencySpectrumDelegate so the live
 * FFT amplitudes appear as a rolling colour band per channel.
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
