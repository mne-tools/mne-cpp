//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2014-2026 MNE-CPP Authors
 *
 * @file     eegosportssetupprojectwidget.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Viktor Klueber <Viktor.Klueber@tu-ilmenau.de>
 * @since    0.1.0
 * @date     July 2014
 * @brief    Contains the declaration of the EEGoSportsSetupProjectWidget class.
 */

#ifndef EEGOSPORTSSETUPPROJECTWIDGET_H
#define EEGOSPORTSSETUPPROJECTWIDGET_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QWidget>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace Ui {
    class EEGoSportsSetupProjectWidget;
}

//=============================================================================================================
// DEFINE NAMESPACE EEGOSPORTSPLUGIN
//=============================================================================================================

namespace EEGOSPORTSPLUGIN
{

//=============================================================================================================
// EEGOSPORTSPLUGIN FORWARD DECLARATIONS
//=============================================================================================================

class EEGoSports;

//=============================================================================================================
/**
 * DECLARE CLASS EEGoSportsSetupProjectWidget
 *
 * @brief The EEGoSportsSetupProjectWidget class provides the EEGoSportsSetupProjectWidget configuration window.
 */
class EEGoSportsSetupProjectWidget : public QWidget
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     * Constructs a EEGoSportsSetupProjectWidget which is a child of parent.
     *
     * @param[in] parent pointer to parent widget; If parent is 0, the new EEGoSportsSetupProjectWidget becomes a window. If parent is another widget, EEGoSportsSetupWidget becomes a child window inside parent. EEGoSportsSetupWidget is deleted when its parent is deleted.
     * @param[in] pEEGoSports a pointer to the corresponding ECGSimulator.
     */
    explicit EEGoSportsSetupProjectWidget(EEGoSports* pEEGoSports, QWidget *parent = 0);

    //=========================================================================================================
    /**
     * Destructs a EEGoSportsSetupProjectWidget which is a child of parent.
     *
     */
    ~EEGoSportsSetupProjectWidget();

    //=========================================================================================================
    /**
     * Inits the GUI
     *
     */
    void initGui();

private:
    EEGoSports*                           m_pEEGoSports;        /**< a pointer to corresponding EEGoSports.*/

    Ui::EEGoSportsSetupProjectWidget*     m_pUi;                /**< the user interface for the EEGoSportsSetupWidget.*/

    //=========================================================================================================
    /**
     * Sets the project dir
     */
    void changeCardinalMode(const QString &text);

    //=========================================================================================================
    /**
     * Sets the project dir
     */
    void onCardinalComboBoxChanged();

    //=========================================================================================================
    /**
     * Sets the project dir
     *
     * @param[in] sPath  The file path of the 3D layout file.
     */
    void updateCardinalComboBoxes(const QString& sPath);

    //=========================================================================================================
    /**
     * Sets the dir where the eeg cap file is located
     */
    void changeCap();

    //=========================================================================================================
    /**
     * Sets the dir where the cardinal file is located
     */
    void changeCardinalFile();

    //=========================================================================================================
    /**
     * Changes the EEG cap and file path variables in the EEGoSports class
     */
    void changeQLineEdits();

signals:
    //=========================================================================================================
    /**
     * Emit this signal whenever the cardinal points changed.
     *
     * @param[in] sLPA       The channel name to take as the LPA.
     * @param[in] dLPA       The amount (in m) to translate the LPA channel position on the z axis.
     * @param[in] sRPA       The channel name to take as the RPA.
     * @param[in] dRPA       The amount (in m) to translate the RPA channel position on the z axis.
     * @param[in] sNasion    The channel name to take as the Nasion.
     * @param[in] dNasion    The amount (in m) to translate the Nasion channel position on the z axis.
     */
    void cardinalPointsChanged(const QString& sLPA, double dLPA, const QString& sRPA, double dRPA, const QString& sNasion, double dNasion);
};
} // NAMESPACE

#endif // EEGOSPORTSSETUPPROJECTWIDGET_H
