//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2016-2026 MNE-CPP Authors
 *
 * @file     brainampsetupprojectwidget.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Viktor Klueber <Viktor.Klueber@tu-ilmenau.de>
 * @since    0.1.0
 * @date     October, 2016
 * @brief    Contains the declaration of the BrainAMPSetupProjectWidget class.
 */

#ifndef BRAINAMPSETUPPROJECTWIDGET_H
#define BRAINAMPSETUPPROJECTWIDGET_H

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
    class BrainAMPSetupProjectWidget;
}

//=============================================================================================================
// DEFINE NAMESPACE BRAINAMPPLUGIN
//=============================================================================================================

namespace BRAINAMPPLUGIN
{

//=============================================================================================================
// BRAINAMPPLUGIN FORWARD DECLARATIONS
//=============================================================================================================

class BrainAMP;

//=============================================================================================================
/**
 * DECLARE CLASS BrainAMPSetupProjectWidget
 *
 * @brief The BrainAMPSetupProjectWidget class provides the BrainAMPSetupProjectWidget configuration window.
 */
class BrainAMPSetupProjectWidget : public QWidget
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     * Constructs a BrainAMPSetupProjectWidget which is a child of parent.
     *
     * @param[in] parent pointer to parent widget; If parent is 0, the new BrainAMPSetupProjectWidget becomes a window. If parent is another widget, BrainAMPSetupWidget becomes a child window inside parent. BrainAMPSetupWidget is deleted when its parent is deleted.
     * @param[in] pBrainAMP a pointer to the corresponding ECGSimulator.
     */
    explicit BrainAMPSetupProjectWidget(BrainAMP* pBrainAMP,
                                        QWidget *parent = 0);

    //=========================================================================================================
    /**
     * Destructs a BrainAMPSetupProjectWidget which is a child of parent.
     *
     */
    ~BrainAMPSetupProjectWidget();

    //=========================================================================================================
    /**
     * Inits the GUI
     *
     */
    void initGui();

private:
    BrainAMP*                           m_pBrainAMP;        /**< a pointer to corresponding BrainAMP.*/
    Ui::BrainAMPSetupProjectWidget*     ui;                 /**< the user interface for the BrainAMPSetupWidget.*/

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
     * Changes the EEG cap and file path variables in the BrainAMP class
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
    void cardinalPointsChanged(const QString& sLPA,
                               double dLPA,
                               const QString& sRPA,
                               double dRPA,
                               const QString& sNasion,
                               double dNasion);
};
} // NAMESPACE

#endif // BRAINAMPSETUPPROJECTWIDGET_H
