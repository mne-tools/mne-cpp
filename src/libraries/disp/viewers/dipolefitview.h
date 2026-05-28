//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2020-2026 MNE-CPP Authors
 *
 * @file     dipolefitview.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.8
 * @date     November 2020
 * @brief    Input / output panel for the single-equivalent-current-dipole fit tool.
 *
 * DipoleFitView gathers all files and parameters required by
 * @c mne_dipole_fit (measurement, BEM, noise covariance, mri trans,
 * time window, baseline, channel selection) and shows the fitted
 * dipole position, orientation, magnitude and goodness-of-fit in a
 * result table once the underlying @ref INVERSELIB::DipoleFit job has
 * finished.
 */

#ifndef DIPOLEFITVIEW_H
#define DIPOLEFITVIEW_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp_global.h"
#include "abstractview.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QWidget>
#include <QPointer>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace Ui {
    class DipoleFitViewWidget;
}

//=============================================================================================================
// DEFINE NAMESPACE DISPLIB
//=============================================================================================================

namespace DISPLIB
{

//=============================================================================================================
/**
 * @brief Single-equivalent-current-dipole-fit input / result panel.
 *
 * Collects measurement / BEM / covariance / trans inputs and a time
 * window, launches the underlying @c INVERSELIB::DipoleFit job, then
 * renders the fitted position, orientation, magnitude and
 * goodness-of-fit in a result table.
 */
class DISPSHARED_EXPORT DipoleFitView : public AbstractView
{
    Q_OBJECT

public:
    DipoleFitView(QWidget *parent = 0,
                  Qt::WindowFlags f = Qt::Widget);

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
     * Updates GUI to show selected BEM file
     *
     * @param[in] sFileName.
     */
    void addBem(const QString& sFileName);

    //=========================================================================================================
    /**
     * Updates GUI to show selected MRI file
     *
     * @param[in] sFileName.
     */
    void addMri(const QString& sFileName);

    //=========================================================================================================
    /**
     * Updates GUI to show selecte Noise file
     *
     * @param[in] sFileName.
     */
    void addNoise(const QString& sFileName);

    //=========================================================================================================
    /**
     * Updates GUI to show selected measurement file
     *
     * @param[in] sFileName.
     */
    void addMeas(const QString& sFileName);

    //=========================================================================================================
    /**
     * Removes model from view
     *
     * @param[in] sModelName   name of model to be removed.
     * @param[in] iType        type of model (1-measurement, 2-BEM, 3-MRI, 4-Cov).
     */
    void removeModel(const QString& sModelName, int iType);

    //=========================================================================================================
    /**
     * Sends updated signals for parameters (excluding selected models)
     */
    void requestParams();

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

private:

    //=========================================================================================================
    /**
     * Sets up Gui slot/signal connections and initializes item group box selections
     */
    void initGui();

    Ui::DipoleFitViewWidget*        m_pUi;              /**< Holds GUI for view. */

signals:
    //=========================================================================================================
    /**
     * Triggers dipole fit calculation
     */
    void performDipoleFit(const QString& sFitName);

    //=========================================================================================================
    /**
     * Send new modality selection
     *
     * @param[in] bEEG     whether to use EEG (true = yes, false = no).
     * @param[in] bMEG     whether to use MEG (true = yes, false = no).
     */
    void modalityChanged(bool bEEG,
                         bool bMEG);

    //=========================================================================================================
    /**
     * Send new time values
     *
     * @param[in] iMin     new minimum in milliseconds.
     * @param[in] iMax     new maximum in milliseconds.
     * @param[in] iStep    new step value in milliseconds.
     * @param[in] iInt     new integration value in milliseconds.
     */
    void timeChanged(int iMin,
                     int iMax,
                     int iStep,
                     int iInt);

    //=========================================================================================================
    /**
     * Send new fitting values
     *
     * @param[in] iMinDistance     distance to inner skull in millimeters.
     * @param[in] iSize        radisu size guess in millimeters.
     */
    void fittingChanged(int iMinDistance,
                        int iSize);

    //=========================================================================================================
    /**
     * Send new basline values
     *
     * @param[in] iBMin    new minimum in milliseconds.
     * @param[in] iBMax    new maximum in milliseconds.
     */
    void baselineChanged(int iBMin,
                         int iBMax);

    //=========================================================================================================
    /**
     * Set new manual noise parameters
     *
     * @param[in] dGrad    gradiometer value.
     * @param[in] dMag     magnetometer value.
     * @param[in] dEeg     eeg value.
     */
    void noiseChanged(double dGrad,
                      double dMag,
                      double dEeg);

    //=========================================================================================================
    /**
     * Set new regularization parameters
     *
     * @param[in] iReg         overall regularization parameter.
     * @param[in] iRegGrad     gradiatometer regularizaation parameter.
     * @param[in] iRegMag      magnetometer regularization parameter.
     * @param[in] iRegEeg      eeg regularization parameter.
     */
    void regChanged(double dRegGrad,
                    double dRegMag,
                    double dRegEeg);

    //=========================================================================================================
    /**
     * Select set from measurement to use
     *
     * @param[in] iSet.
     */
    void setChanged(int iSet);

    //=========================================================================================================
    /**
     * Set new spherer model parameters
     *
     * @param[in] dX           x position in millimeters.
     * @param[in] dY           y position in millimeters.
     * @param[in] dZ           z position in millimeters.
     * @param[in] dRadius      radius in millimeters.
     */
    void sphereChanged(double dX,
                       double dY,
                       double dZ,
                       double dRadius);

    //=========================================================================================================
    /**
     * Set new Bem model
     *
     * @param[in] sName    file name.
     */
    void selectedBem(const QString& sName);

    //=========================================================================================================
    /**
     * Set new Mri model
     *
     * @param[in] sName    file name.
     */
    void selectedMri(const QString& sName);

    //=========================================================================================================
    /**
     * Set new Noise model
     *
     * @param[in] sName    file name.
     */
    void selectedNoise(const QString& sName);

    //=========================================================================================================
    /**
     * Set new measurement model
     *
     * @param[in] sName    file name.
     */
    void selectedMeas(const QString& sName);

};
}//namespace

#endif // DIPOLEFITVIEW_H
