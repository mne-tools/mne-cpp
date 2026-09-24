//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     realtimespectrumwidget.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     February, 2013
 * @brief    Declaration of the RealTimeSpectrumWidget Class.
 */

#ifndef FREQUENCYSPECTRUMWIDGET_H
#define FREQUENCYSPECTRUMWIDGET_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "scdisp_global.h"
#include "measurementwidget.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QPointer>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class QTime;

namespace SCMEASLIB {
    class RealTimeSpectrum;
}

namespace DISPLIB {
    class SpectrumSettingsView;
    class SpectrumView;
}

//=============================================================================================================
// DEFINE NAMESPACE SCDISPLIB
//=============================================================================================================

namespace SCDISPLIB
{

//=============================================================================================================
// SCDISPLIB FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * DECLARE CLASS RealTimeSpectrumWidget
 *
 * @brief The RealTimeSpectrumWidget class provides a equalizer display
 */
class SCDISPSHARED_EXPORT RealTimeSpectrumWidget : public MeasurementWidget
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     * Constructs a RealTimeSpectrumWidget which is a child of parent.
     *
     * @param[in] pNE           pointer to noise estimation measurement.
     * @param[in] pTime         pointer to application time.
     * @param[in] parent        pointer to parent widget; If parent is 0, the new NumericWidget becomes a window. If parent is another widget, NumericWidget becomes a child window inside parent. NumericWidget is deleted when its parent is deleted.
     */
    RealTimeSpectrumWidget(QSharedPointer<SCMEASLIB::RealTimeSpectrum> pNE,
                           QSharedPointer<QTime> &pTime,
                           QWidget* parent = 0);

    //=========================================================================================================
    /**
     * Destroys the RealTimeSpectrumWidget.
     */
    ~RealTimeSpectrumWidget();

    //=========================================================================================================
    /**
     * Initialise the MeasurementWidget.
     */
    virtual void init(){}

    //=========================================================================================================
    /**
     * Is called when new data are available.
     *
     * @param[in] pMeasurement  pointer to measurement -> not used because its direct attached to the measurement.
     */
    virtual void update(SCMEASLIB::Measurement::SPtr pMeasurement);

    //=========================================================================================================
    /**
     * Is called when new data are available.
     */
    virtual void getData();

    //=========================================================================================================
    /**
     * Initialise the SettingsWidget.
     */
    void initSettingsWidget();

    bool eventFilter(QObject *object, QEvent *event);

private:
    //=========================================================================================================
    /**
     * Initialise the display control widgets to be shown in the QuickControlView.
     */
    void initDisplayControllWidgets();

    //=========================================================================================================
    /**
     * Broadcast settings of frequency spectrum settings widget
     */
    void broadcastSettings();

    //=========================================================================================================
    /**
     * Show the frequency spectrum settings widget
     */
    void showSpectrumSettingsView();

    QPointer<QAction>                                           m_pActionFrequencySettings;         /**< Frequency spectrum settings action. */
    QPointer<DISPLIB::SpectrumView>                             m_pSpectrumView;                    /**< Frequency spectrum view. */

    QSharedPointer<DISPLIB::SpectrumSettingsView>               m_pSpectrumSettingsView;            /**< Frequency spectrum settings modality widget. */
    QSharedPointer<SCMEASLIB::RealTimeSpectrum>                 m_pFS;                              /**< The frequency spectrum measurement. */

    float m_fLowerFrqBound;         /**< Lower frequency bound. */
    float m_fUpperFrqBound;         /**< Upper frequency bound. */
};
} // NAMESPACE

#endif // FREQUENCYSPECTRUMWIDGET_H
