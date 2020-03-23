//=============================================================================================================
/**
 * @file     hpisettingsview.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  dev
 * @date     March, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020, Lorenz Esch. All rights reserved.
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
 * @brief    HpiSettingsView class declaration.
 *
 */

#ifndef HPISETTINGSVIEW_H
#define HPISETTINGSVIEW_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp_global.h"

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

namespace Ui {
    class HpiSettingsViewWidget;
}

namespace FIFFLIB {
    class FiffDigPoint;
}

//=============================================================================================================
// DEFINE NAMESPACE DISPLIB
//=============================================================================================================

namespace DISPLIB
{

//=============================================================================================================
/**
 * The HpiSettingsView class provides a QWidget for the HPI controls.
 *
 * @brief The HpiSettingsView class provides a QWidget for the HPI controls.
 */
class DISPSHARED_EXPORT HpiSettingsView : public QWidget
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     * Constructs a HpiSettingsView object.
     *
     * @param[in] pFiffInfo      The FiffInfo.
     * @param[in] parent         The parent widget.
     */
    HpiSettingsView(const QString& sSettingsPath = "",
                    QWidget *parent = 0,
                    Qt::WindowFlags f = Qt::Widget);

    ~HpiSettingsView();

    //=========================================================================================================
    /**
     * Updates the error related labels.
     *
     * @param[in] vError            the new error values.
     * @param[in] dMeanErrorDist    the mean error value.
     */
    void setErrorLabels(const QVector<double>& vError,
                        double dMeanErrorDist);

    //=========================================================================================================
    /**
     * Get the SSP checked status.
     *
     * @return  The current SSP checked status.
     */
    bool getSspStatusChanged();

    //=========================================================================================================
    /**
     * Get the Comp checked status.
     *
     * @return  The current Comp checked status.
     */
    bool getCompStatusChanged();

    //=========================================================================================================
    /**
     * Get the allowed mean error distance.
     *
     * @return  The current allowed mean error distance.
     */
    double getAllowedMeanErrorDistChanged();

protected:    
    //=========================================================================================================
    /**
     * Saves all important settings of this view via QSettings.
     *
     * @param[in] settingsPath        the path to store the settings to.
     */
    void saveSettings(const QString& settingsPath);

    //=========================================================================================================
    /**
     * Loads and inits all important settings of this view via QSettings.
     *
     * @param[in] settingsPath        the path to load the settings from.
     */
    void loadSettings(const QString& settingsPath);

    //=========================================================================================================
    /**
     * Load digitzers from a file.
     */
    void onLoadDigitizers();

    //=========================================================================================================
    /**
     * Called whenever a cell in the frequenc table widget is changed.
     *
     * @param[in] row        the row of the changed cell.
     * @param[in] col        the column of the changed cell.
     */
    void onFrequencyCellChanged(int row,
                                int col);

    //=========================================================================================================
    /**
     * Add coil to frequency table widget.
     */
    void onAddCoil();

    //=========================================================================================================
    /**
     * Remove coil to frequency table widget.
     */
    void onRemoveCoil();

    //=========================================================================================================
    /**
     * Read Polhemus data from fif file.
     */
    QList<FIFFLIB::FiffDigPoint> readPolhemusDig(const QString& fileName);

    Ui::HpiSettingsViewWidget*                  m_ui;                   /**< The HPI dialog. */

    QVector<int>                                m_vCoilFreqs;           /**< Vector contains the HPI coil frequencies. */

    QString                                     m_sSettingsPath;        /**< The settings path to store the GUI settings to. */

signals:
    //=========================================================================================================
    /**
     * Emit this signal whenever the coil frequencies changed.
     *
     * @param[in] vCoilFreqs    The new coil frequencies.
     */
    void coilFrequenciesChanged(const QVector<int>& vCoilFreqs);

    //=========================================================================================================
    /**
     * Emit this signal whenever the user toggled the do HPI check box.
     *
     * @param[in] state    Whether to do continous HPI.
     */
    void continousHPIToggled(bool state);

    //=========================================================================================================
    /**
     * Emit this signal whenever new digitzers were loaded.
     *
     * @param[in] lDigitzers    The new digitzers.
     * @param[in] sFilePath     The file path to the new digitzers.
     */
    void digitizersChanged(const QList<FIFFLIB::FiffDigPoint>& lDigitzers,
                           const QString& sFilePath);

    //=========================================================================================================
    /**
     * Emit this signal whenever a single HPI fit is supposed to be triggered.
     */
    void doSingleHpiFit();

    //=========================================================================================================
    /**
     * Emit this signal whenever SSP checkbox changed.
     *
     * @param[in] bChecked    Whether the SSP check box is checked.
     */
    void sspStatusChanged(bool bChecked);

    //=========================================================================================================
    /**
     * Emit this signal whenever compensator checkbox changed.
     *
     * @param[in] bChecked    Whether the compensator check box is checked.
     */
    void compStatusChanged(bool bChecked);

    //=========================================================================================================
    /**
     * Emit this signal whenever continous HPI checkbox changed.
     *
     * @param[in] bChecked    Whether the continous HPI check box is checked.
     */
    void contHpiStatusChanged(bool bChecked);

    //=========================================================================================================
    /**
     * Emit this signal whenever the allowed error changed.
     *
     * @param[in] dAllowedMeanErrorDist    Allowed mean error in mm.
     */
    void allowedMeanErrorDistChanged(double dAllowedMeanErrorDist);
};

} //NAMESPACE

#endif // HPISETTINGSVIEW_H
