//=============================================================================================================
/**
 * @file     averagewindow.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  2.1.0
 * @date     October, 2014
 *
 * @section  LICENSE
 *
 * Copyright (C) 2014, Christoph Dinh, Lorenz Esch. All rights reserved.
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
 * @brief    Contains the declaration of the AverageWindow class.
 *
 */

#ifndef AVERAGEWINDOW_H
#define AVERAGEWINDOW_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ui_averagewindow.h"
#include "../Utils/butterflyscene.h"
#include "../Utils/types.h"
#include "../Models/averagemodel.h"
#include "../Delegates/averagedelegate.h"

#include <utils/layoutloader.h>
#include <disp/viewers/helpers/averagescene.h>
#include <disp/viewers/helpers/selectionsceneitem.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDockWidget>
#include <QAction>
#include <QFileDialog>
#include <QStandardPaths>
#include <QSvgGenerator>

#include <memory>

class QDoubleSpinBox;

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE MNEBROWSE
//=============================================================================================================

namespace MNEBROWSE
{


//*************************************************************************************************************
//=============================================================================================================
// DEFINE FORWARD DECLARATIONS
//=============================================================================================================

class LayoutScene;


/**
 * DECLARE CLASS AverageWindow
 *
 * @brief The AverageWindow class provides a dock window for plotting averages.
 */
class AverageWindow : public QDockWidget
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     * Constructs a AverageWindow which is a child of parent.
     *
     * @param [in] parent pointer to parent widget; If parent is 0, the new AverageWindow becomes a window. If parent is another widget, AverageWindow becomes a child window inside parent. AverageWindow is deleted when its parent is deleted.
     * @param [in] file default file used to read the evoked data from.
     */
    AverageWindow(QWidget *parent, QFile &file);

    //=========================================================================================================
    /**
     * Constructs a AverageWindow which is a child of parent.
     *
     * @param [in] parent pointer to parent widget; If parent is 0, the new AverageWindow becomes a window. If parent is another widget, AverageWindow becomes a child window inside parent. AverageWindow is deleted when its parent is deleted.
     */
    AverageWindow(QWidget *parent);

    //=========================================================================================================
    /**
     * Constructs a AverageWindow which is a child of parent.
     */
//    AverageWindow();

    //=========================================================================================================
    /**
     * Destroys the AverageWindow.
     * All AverageWindow's children are deleted first. The application exits if AverageWindow is the main widget.
     */
    ~AverageWindow();

    //=========================================================================================================
    /**
     * Returns the AverageModel of this window
     */
    AverageModel* getAverageModel();

    //=========================================================================================================
    /**
     * call this whenever the external channel selection manager changes
     *
     * * @param [in] selectedChannelItems list of selected graphic items
     */
    void channelSelectionManagerChanged(const QList<QGraphicsItem *> &selectedChannelItems);

    //=========================================================================================================
    /**
     * Scales the averaged data according to scaleMap
     *
     * @param [in] scaleMap map with all channel types and their current scaling value
     */
    void scaleAveragedData(const QMap<QString,double> &scaleMap);

    //=========================================================================================================
    /**
     * Scales the averaged data according to scaleMap
     *
     * @param [in] mappedChannelNames all mapped channel names
     */
    void setMappedChannelNames(QStringList mappedChannelNames);

    //=========================================================================================================
    /**
     * Set the covariance used for whitening butterfly plots.
     *
     * @param covariance the active noise covariance.
     */
    void setNoiseCovariance(const FIFFLIB::FiffCov& covariance);

    //=========================================================================================================
    /**
     * Clear the covariance used for whitening butterfly plots.
     */
    void clearNoiseCovariance();

    //=========================================================================================================
    /**
     * Update whitening settings used by the covariance-aware display paths.
     *
     * @param settings The new whitening settings.
     */
    void setWhiteningSettings(const WhiteningSettings& settings);

    //=========================================================================================================
    /**
     * Returns the current whitening settings.
     *
     * @return Active whitening settings.
     */
    WhiteningSettings whiteningSettings() const;

    //=========================================================================================================
    /**
     * Enable or disable whitening in the butterfly plot.
     *
     * @param enabled true to whiten the butterfly plot.
     */
    void setButterflyWhiteningEnabled(bool enabled);

    //=========================================================================================================
    /**
     * Returns whether butterfly whitening is currently enabled.
     *
     * @return true when whitening is active.
     */
    bool isButterflyWhiteningEnabled() const;

    //=========================================================================================================
    /**
     * Returns the currently selected evoked-set rows.
     *
     * @return Selected row indices in the loaded-set table.
     */
    QList<int> selectedSetRows() const;

    //=========================================================================================================
    /**
     * Enable or disable the quick "recompute last evoked" action in the average manager.
     *
     * @param available true when a previously used evoked setup is available.
     */
    void setRecomputeAvailable(bool available);

private:

    //=========================================================================================================
    /**
     * called by constructor to perform common initialization step
     */
    void init();

    //=========================================================================================================
    /**
     * inits the model view controller paradigm of this window
     *
     * @param [in] file holds the file which is to be loaded on startup
     */
    void initMVC(QFile &file);

    //=========================================================================================================
    /**
     * inits the model view controller paradigm of this window
     */
    void initMVC();

    //=========================================================================================================
    /**
     * inits the table widgets of this window
     */
    void initTableViewWidgets();

    //=========================================================================================================
    /**
     * inits the average scene of this window
     */
    void initAverageSceneView();

    //=========================================================================================================
    /**
     * Inits all QPushButtons in this window
     */
    void initButtons();

    //=========================================================================================================
    /**
     * Inits all QComboBoxes in this window
     */
    void initComboBoxes();

    //=========================================================================================================
    /**
     * call this function whenever a selection was made in teh evoked data set list
     */
    void onSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);

    //=========================================================================================================
    /**
     * Rebuild plots from the current selection and display settings.
     */
    void refreshPlots();

    /**
     * Populate the 2D layout scene from evoked channel info.
     * Creates AverageSceneItems at positions derived from channel coordinates.
     *
     * @param[in] selectedRows  The currently selected evoked-set model indices.
     * @return true if items were created, false otherwise.
     */
    bool populateLayoutFromEvoked(const QModelIndexList& selectedRows);

    //=========================================================================================================
    /**
     * saves the current layout average plot as png or svg to file
     */
    void exportAverageLayoutPlot();

    //=========================================================================================================
    /**
     * saves the current butterfly average plot as png or svg to file
     */
    void exportAverageButterflyPlot();

    //=========================================================================================================
    /**
     * Selects all loaded evoked sets so newly loaded/computed data is shown immediately.
     */
    void selectLoadedSets();

    //=========================================================================================================
    /**
     * reimplemented resize event.
     */
    void resizeEvent(QResizeEvent * event);

    /**
     * Resize butterfly scene items to fill the viewport at 1:1 pixel scale.
     */
    void updateButterflySize();

    std::unique_ptr<Ui::AverageWindow> ui;                  /**< Pointer to the qt designer generated ui class.*/

    QList<QColor>           m_lButterflyColors;     /**< List which holds 500 randomly generated colors.*/

    QStringList             m_mappedChannelNames;   /**< List which holds the mapped channel names.*/

    AverageModel*           m_pAverageModel;        /**< The QAbstractTable average model being part of the model/view framework of Qt. */
    AverageDelegate*        m_pAverageDelegate;     /**< The QItemDelegateaverage delegate being part of the model/view framework of Qt. */
    AverageScene*           m_pAverageScene;        /**< The pointer to the average scene. */

    ButterflyScene*         m_pButterflyScene;      /**< The pointer to the butterfly scene. */
    FIFFLIB::FiffCov        m_noiseCovariance;      /**< Optional covariance used to whiten butterfly plots. */
    WhiteningSettings       m_whiteningSettings;    /**< Whitening controls shared with the covariance manager. */
    QVector<FIFFLIB::FiffEvoked> m_layoutDisplayEvokeds; /**< Cached display evokeds so layout plots can reference transformed data. */

signals:
    //=========================================================================================================
    /**
     * Emitted when the user requests a new evoked computation from the average manager UI.
     */
    void addAverageRequested();

    //=========================================================================================================
    /**
     * Emitted when the user requests recomputation with the last-used evoked settings.
     */
    void recomputeAverageRequested();

private:
    QAction*                m_pComputeAverageAction = nullptr; /**< Default evoked-compute action shown by the add button. */
    QAction*                m_pRecomputeAverageAction = nullptr; /**< Quick recompute action using the last saved settings. */

    QDoubleSpinBox*         m_pSpinGrad = nullptr;
    QDoubleSpinBox*         m_pSpinMag  = nullptr;
    QDoubleSpinBox*         m_pSpinEEG  = nullptr;
    bool                    m_bAutoScaled = false;
};

} // NAMESPACE MNEBROWSE

#endif // AVERAGEWINDOW_H
