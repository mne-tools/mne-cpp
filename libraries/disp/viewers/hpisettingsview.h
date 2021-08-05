//=============================================================================================================
/**
 * @file     hpisettingsview.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 *           Ruben Dörfel <ruben.doerfel@tu-ilmenau.de>
 * @since    0.1.0
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
#include "abstractview.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QJsonDocument>

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
    class FiffDigPointSet;
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
class DISPSHARED_EXPORT HpiSettingsView : public AbstractView
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
     * Updates the movement refered to last reference head position.
     *
     * @param[in] dMovement         the movement refered to last fit.
     * @param[in] dRotation         the rotation refered to last fit.
     */
    void setMovementResults(double dMovement,
                            double dRotation);

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

    //=========================================================================================================
    /**
     * Get the allowed head movement.
     *
     * @return  The current allowed head movement.
     */
    double getAllowedMovementChanged();

    //=========================================================================================================
    /**
     * Get the allowed head rotation.
     *
     * @return  The current allowed head rotation.
     */
    double getAllowedRotationChanged();

    //=========================================================================================================
    /**
     * Get number of fits per second to do when performing continuous hpi
     *
     * @return  Number of fits per second
     */
    int getFittingWindowSize();

    //=========================================================================================================
    /**
     * Display digitizer metadata bsed on input pointList
     *
     * @param[in] pointList     list of digitizer points
     */
    void newDigitizerList(QList<FIFFLIB::FiffDigPoint> pointList);

    //=========================================================================================================
    /**
     * Load coil presets from json file at the provided path
     *
     * @param[in] sFilePath     PAth to json file with coil preset data
     */
    void loadCoilPresets(const QString& sFilePath);

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
    QList<FIFFLIB::FiffDigPoint> readDigitizersFromFile(const QString& fileName);

    void setupCoilPresets(int iNumCoils);

    void populatePresetGUI(QJsonArray presetData);

    void populateCoilGUI();

    void selectCoilPreset(int iCoilPresetIndex);

    void addCoilFreqToGUI(int iCoilFreq);

    void addCoilErrorToGUI();

    //=========================================================================================================
    /**
     * Resets GUI display tables and empties all rows
     */
    void resetCoilGUI();

    /**
     * @UpdateGUI information with data from input digitizer set.
     *
     * @param[in] digSet    Digigtizer set from which data metadata will be displayed.
     */
    void updateDigitizerInfoGUI(const FIFFLIB::FiffDigPointSet& digSet);


    Ui::HpiSettingsViewWidget*                  m_pUi;                  /**< The HPI dialog. */

    QVector<int>                                m_vCoilFreqs;           /**< Vector contains the HPI coil frequencies. */

    QString                                     m_sSettingsPath;        /**< The settings path to store the GUI settings to. */

    QJsonDocument                               m_CoilPresets;

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
     * Emit this signal whenever new digitzers were loaded.
     *
     * @param[in] lDigitzers    The new digitzers.
     * @param[in] sFilePath     The file path to the new digitzers.
     */
    void digitizersChanged(const QList<FIFFLIB::FiffDigPoint>& lDigitzers,
                           const QString& sFilePath);

    //=========================================================================================================
    /**
     * Emit this signal whenever the frequency ordering is supposed to be triggered.
     */
    void doFreqOrder();

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
     * Emit this signal when 'fits per second' control gets updated.
     *
     * @param[in] iFitsPerSecond    How many fits per second we should do.
     */
    void fittingWindowSizeChanged(int iFitsPerSecond);

    //=========================================================================================================
    /**
     * Emit this signal whenever the allowed error changed.
     *
     * @param[in] dAllowedMeanErrorDist    Allowed mean error in mm.
     */
    void allowedMeanErrorDistChanged(double dAllowedMeanErrorDist);

    //=========================================================================================================
    /**
     * Emit this signal whenever the allowed head movement threshold changed.
     *
     * @param[in] dAllowedMeanErrorDist    Allowed movement threshold.
     */
    void allowedMovementChanged(double dAllowedMovement);

    //=========================================================================================================
    /**
     * Emit this signal whenever the allowed head rotation threshold changed.
     *
     * @param[in] dAllowedMeanErrorDist    Allowed rotation in degree.
     */
    void allowedRotationChanged(double dAllowedRotation);
};

} //NAMESPACE

#endif // HPISETTINGSVIEW_H
