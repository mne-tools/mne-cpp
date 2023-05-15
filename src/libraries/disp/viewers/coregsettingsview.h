//=============================================================================================================
/**
 * @file     coregsettingsview.h
 * @author   Ruben Dörfel <doerfelruben@aol.com>
 * @since    0.1.6
 * @date     August, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020, Ruben Dörfel. All rights reserved.
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
 * @brief     CoregSettingsView class declaration.
 *
 */

#ifndef COREGSETTINGSVIEW_H
#define COREGSETTINGSVIEW_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp_global.h"
#include "abstractview.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QVector3D>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace Ui {
    class CoregSettingsViewWidget;
}

namespace FIFFLIB {
    class FiffCoordTrans;
}
//=============================================================================================================
// DEFINE NAMESPACE NAMESPACE
//=============================================================================================================

namespace DISPLIB
{

//=============================================================================================================
// NAMESPACE FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * This class provides the interface to the coregistration setting view.
 *
 */
class DISPSHARED_EXPORT CoregSettingsView : public AbstractView
{
    Q_OBJECT

public:
    typedef QSharedPointer<CoregSettingsView> SPtr;            /**< Shared pointer type for DISPLIB. */
    typedef QSharedPointer<const CoregSettingsView> ConstSPtr; /**< Const shared pointer type for DISPLIB. */

    //=========================================================================================================
    /**
    * Constructs a CoregSettingsView object.
    */
    explicit CoregSettingsView(const QString& sSettingsPath = "",
                               QWidget *parent = 0,
                               Qt::WindowFlags f = Qt::Widget);

    ~CoregSettingsView();

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
     * Clear/Emüpty the selection menu for bem models.
     *
     */
    void clearSelectionBem();

    //=========================================================================================================
    /**
     * Add a new Bem to the drop down menu.
     *
     * @param[in] sBemName     The name of the bem.
     */
    void addSelectionBem(const QString& sBemName);

    //=========================================================================================================
    /**
     * Return the currently selected bem.
     *
     * @return The name of the currently selected bem.
     */
    QString getCurrentSelectedBem();

    //=========================================================================================================
    /**
     * Get the checked radio box for fiducial picking.
     *
     * @return  The currently selected fiducial.
     */
    int getCurrentFiducial();

    //=========================================================================================================
    /**
     * Set the position of the fiducials.
     *
     * @param[in] vecAxialPosition     The vector containig x-,y- and x- Position for LPA, Nasion and RPA.
     */
    void setFiducials(const QVector3D vecAxialPosition);

    //=========================================================================================================
    /**
     * Get the maximum number of icp iterations.
     *
     * @return  The maximum number of iterations.
     */
    int getMaxIter();

    //=========================================================================================================
    /**
     * Get the onvergence value.
     *
     * @return  The convergence value.
     */
    float getConvergence();

    //=========================================================================================================
    /**
     * Get the auto scale state. If activated, unform scaling is applied for any icp iteration.
     *
     * @return  The auto scale state.
     */
    bool getAutoScale();

    //=========================================================================================================
    /**
     * Get the the maximum distance for digitizers from the surface.
     *
     * @return  The maximum distance in m.
     */
    float getOmmitDistance();

    //=========================================================================================================
    /**
     * Set the the number of omitted points.
     *
     * @param[in] iN  The number of omitted digitizer points.
     */
    void setOmittedPoints(const int iN);

    //=========================================================================================================
    /**
     * Set the the Root-Mean-Square-Error (Digitizer-Surface) after icp.
     *
     * @param[in] fRMSE     The RMSE in m.
     */
    void setRMSE(const float fRMSE);

    //=========================================================================================================
    /**
     * Get the weights for the digitizer types.
     *
     * @return  The weight.
     */
    float getWeightLPA();
    float getWeightRPA();
    float getWeightNAS();
    float getWeightEEG();
    float getWeightHSP();
    float getWeightHPI();

    //=========================================================================================================
    /**
     * Get the types of digitizers to use for coregistration.
     *
     * @return  The list containing the digitizer types to use for coregistration.
     */
    QList<int> getDigitizerCheckState();

    //=========================================================================================================
    /**
     * Get the parameters for the new transformation to build from the adjustment values.
     *
     * @param[out] vecRot       The rotation angle vector in rad (x,y,z).
     * @param[out] vecTrans     The traslation vector (x,y,z).
     * @param[out] vecScale     The vector with the scaling parameters (due to euler transformation: z,y,x).
     *
     */
    void getTransParams(Eigen::Vector3f& vecRot,
                        Eigen::Vector3f& vecTrans,
                        Eigen::Vector3f& vecScale);

    //=========================================================================================================
    /**
     * Set the transformation received from the ICP  algorithm.
     *
     * @param[in] vecRot        The rotation angle vector in rad (x,y,z).
     * @param[in] vecTrans      The traslation vector (x,y,z).
     * @param[in] vecScale      The vector with the scaling parameters (due to euler transformation: z,y,x).
     *
     */
    void setTransParams(const Eigen::Vector3f& vecTrans,
                        const Eigen::Vector3f& vecRot,
                        const Eigen::Vector3f& vecScale);

    //=========================================================================================================
    /**
     * Clears the view
     */
    void clearView();

private:
    //=========================================================================================================
    /**
     * Load fiducial from file
     */
    void onLoadFidFile();

    //=========================================================================================================
    /**
     * Pick fiducials is activated
     */
    void onPickingStatus();

    //=========================================================================================================
    /**
     * Display the location of the current selected Fiducial
     */
    void onFiducialChanged();
    //=========================================================================================================
    /**
     * Fit the fiducials
     */
    void onFitFiducials();

    //=========================================================================================================
    /**
     * Fit the ICP.
     */
    void onFitICP();

    //=========================================================================================================
    /**
     * Store fiducial to file.
     */
    void onStoreFidFile();

    //=========================================================================================================
    /**
     * Load digitizer from file
     */
    void onLoadDigFile();

    //=========================================================================================================
    /**
     * Load the transformation.
     */
    void onLoadTrans();

    //=========================================================================================================
    /**
     * Store the transformation.
     */
    void onStoreTrans();

    //=========================================================================================================
    /**
     * Change the scaling mode.
     */
    void onScalingModeChanges();

    //=========================================================================================================
    /**
     * Set the toolTip information for all buttons.
     */
    void setToolTipInfo();

    Ui::CoregSettingsViewWidget*    m_pUi;                  /**< The CoregSettingsViewWidget.*/
    QString                         m_sSettingsPath;        /**< The settings path to store the GUI settings to. */

    QVector3D                       m_vecLPA;               /**< contains LPA position. */
    QVector3D                       m_vecNAS;               /**< contains Nasion position. */
    QVector3D                       m_vecRPA;               /**< contains RPA position. */

signals:
    //=========================================================================================================
    /**
     * Emit this signal whenever new fiducials were loaded.
     *
     * @param[in] sFilePath    The file path to the fiduical file.
     */
    void fidFileChanged(const QString& sFilePath);

    //=========================================================================================================
    /**
     * Emit this signal whenever the picked fiducial changed.
     *
     * @param[in] iFiducial    The picked fiducial.
     */
    void fiducialChanged(const int iFiducial);

    //=========================================================================================================
    /**
     * Emit this signal whenever the file to store the fiducials changed.
     *
     * @param[in] sFilePath    The file path to the stored fiducials.
     */
    void fidStoreFileChanged(const QString& sFilePath);

    //=========================================================================================================
    /**
     * Emit this signal whenever new digitizers were loaded.
     *
     * @param[in] sFilePath    The file path to the digitizers.
     */
    void digFileChanged(const QString& sFilePath);

    //=========================================================================================================
    /**
     * Emit this signal whenever fiducial alignment is requested.
     */
    void fitFiducials();

    //=========================================================================================================
    /**
     * Emit this signal whenever icp alignment is requested.
     */
    void fitICP();

    //=========================================================================================================
    /**
     * Emit this signal whenever the transformation should be stored.
     *
     * @param[in] sFilePath    The file path to store the transformation.
     */
    void storeTrans(const QString& sFilePath);

    //=========================================================================================================
    /**
     * Emit this signal whenever the transformation should be loaded.
     *
     * @param[in] sFilePath    The file path to load the transformation.
     */
    void loadTrans(const QString& sFilePath);

    //=========================================================================================================
    /**
     * Emit this signal whenever the selected Bem changed.
     *
     * @param[in] sText    The file name of the currently selected Bem.
     */
    void changeSelectedBem(const QString &sText);

    //=========================================================================================================
    /**
     * Emit this signal whenever the scaling, translation or rotation parmeters have changed.
     */
    void transParamChanged();

    //=========================================================================================================
    /**
     * Emit this signal whenever fiducial picking from 3DView should be activated.
     */
    void pickFiducials(bool bActivatePicking);
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

} // NAMESPACE

#endif // COREGSETTINGSVIEW_H

