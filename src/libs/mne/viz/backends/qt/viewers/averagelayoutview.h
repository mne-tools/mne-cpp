//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2014-2026 MNE-CPP Authors
 *
 * @file     averagelayoutview.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>
 * @since    0.1.0
 * @date     May 2014
 * @brief    2-D sensor-layout view placing per-channel evoked traces at their physical positions.
 *
 * AverageLayoutView embeds an @ref mne::viz::qt::AverageLayoutWidget that draws one
 * mini evoked-trace plot (an mne::viz::AverageTile) at each sensor coordinate
 * read from the active @c Layout. It listens to an @ref mne::viz::qt::EvokedSetModel for new averaged
 * responses and to a @ref mne::viz::qt::ChannelInfoModel for channel-type / bad
 * flags, and supports per-condition colour selection and selective
 * channel display via @ref mne::viz::qt::SelectionItem groups.
 */

#ifndef AVERAGELAYOUTVIEW_H
#define AVERAGELAYOUTVIEW_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../viz_qt_global.h"
#include "abstractview.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QPointer>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <map>
#include <memory>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace mne
{
class FiffInfo;
}

//=============================================================================================================
// DEFINE NAMESPACE mne::viz::qt
//=============================================================================================================

namespace mne::viz::qt
{

//=============================================================================================================
// mne::viz::qt FORWARD DECLARATIONS
//=============================================================================================================

class AverageLayoutWidget;
class EvokedSetModel;
class ChannelInfoModel;
class SelectionItem;

//=============================================================================================================
/**
 * @brief Sensor-layout viewer drawing per-channel evoked traces at their physical 2-D positions.
 *
 * Wraps an @ref AverageLayoutWidget fed by an
 * @ref EvokedSetModel and a @ref ChannelInfoModel; selection groups
 * are pushed in as @ref SelectionItem to show only a subset of
 * channels.
 */
class VIZQTSHARED_EXPORT AverageLayoutView : public AbstractView
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     * Constructs a AverageLayoutView which is a child of parent.
     *
     * @param[in] sSettingsPath Settings path under which the widget persists its state (empty = no persistence).
     * @param[in] parent    parent of widget.
     * @param[in] f Qt window flags passed to the QWidget constructor.
     */
    AverageLayoutView(const QString& sSettingsPath = "",
                      QWidget* parent = 0,
                      Qt::WindowFlags f = Qt::Widget);

    //=========================================================================================================
    /**
     * Destroys the AverageLayoutView.
     */
    ~AverageLayoutView();

    //=========================================================================================================
    /**
     * Update the viewport. This, e.g., necessary if this widget was set to a QDockWidget which changes
     * its floating state.
     */
    void updateViewport();

    //=========================================================================================================
    /**
     * Sets the channel info model.
     *
     * @param[in] pChannelInfoModel     The new channel info model.
     */
    void setChannelInfoModel(std::shared_ptr<ChannelInfoModel>& pChannelInfoModel);

    //=========================================================================================================
    /**
     * Sets the evoked set model.
     *
     * @param[in] pEvokedSetModel     The new evoked set model.
     */
    void setEvokedSetModel(const std::shared_ptr<EvokedSetModel>& pEvokedSetModel);

    //=========================================================================================================
    /**
     * Returns the currently set EvokedSetModel
     *
     * @return the currently set EvokedSetModel.
     */
    std::shared_ptr<EvokedSetModel> getEvokedSetModel();

    //=========================================================================================================
    /**
     * Sets the background color of the scene.
     *
     * @param[in] backgroundColor     The new background color.
     */
    void setBackgroundColor(const QColor& backgroundColor);

    //=========================================================================================================
    /**
     * Returns the background color of the scene.
     *
     * @return     The current background color.
     */
    QColor getBackgroundColor();

    //=========================================================================================================
    /**
     * Renders a screenshot of the scene and saves it to the passed path. SVG and PNG supported.
     *
     * @param[in] fileName     The file name and path where to store the screenshot.
     */
    void takeScreenshot(const QString& fileName);

    //=========================================================================================================
    /**
     * Sets the scale map to scaleMap.
     *
     * @param[in] scaleMap map with all channel types and their current scaling value.
     */
    void setScaleMap(const std::map<int, float>& scaleMap);

    //=========================================================================================================
    /**
     * Get the current average colors
     *
     * @return Pointer to the current average colors.
     */
    std::shared_ptr<std::map<std::string, QColor>> getAverageColor() const;

    //=========================================================================================================
    /**
     * Get the current average activations
     *
     * @return Pointer to the current average activations.
     */
    std::shared_ptr<std::map<std::string, bool>> getAverageActivation() const;

    //=========================================================================================================
    /**
     * Set the average colors
     *
     * @param[in] qMapAverageColor      Pointer to the new average colors.
     */
    void setAverageColor(const std::shared_ptr<std::map<std::string, QColor>>& qMapAverageColor);

    //=========================================================================================================
    /**
     * Sets the color used to draw the average to avgColor
     *
     * @param[in] avgColor     Color of averaged signal.
     */
    void setSingleAverageColor(const QColor& avgColor);

    //=========================================================================================================
    /**
     * Set the average activations
     *
     * @param[in] qMapAverageActivation      Pointer to the new average activations.
     */
    void setAverageActivation(const std::shared_ptr<std::map<std::string, bool>>& qMapAverageActivation);

    //=========================================================================================================
    /**
     * Stores FiffInfo to local variable to be used for getting channel info
     *
     * @param[in] pFiffInfo    shared pointer to Fiff Info of currently loaded data.
     */
    void setFiffInfo(const std::shared_ptr<mne::FiffInfo>& pFiffInfo);

    //=========================================================================================================
    /**
     * Shows the channels picked in the channel selection view.
     *
     * @param[in] selection   The picked channels with their layout positions.
     */
    void channelSelectionManagerChanged(const mne::viz::qt::SelectionItem& selection);

    //=========================================================================================================
    /**
     * Sets the currently viewable channels using the corrent name and location parameters
     *
     * @param[in] selectionData     QVariant containing a SelectionItem object with selected channel information.
     */
    void channelSelectionChanged(const QVariant& selectionData);

    //=========================================================================================================
    /**
     * call this function whenever the items' data needs to be updated
     */
    void updateData();

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
     * @param mode     The new mode (Clinical=0, Research=1).
     */
    void updateGuiMode(GuiMode mode);

    //=========================================================================================================
    /**
     * Update the views GUI based on the set ProcessingMode (RealTime=0, Offline=1).
     *
     * @param mode     The new mode (RealTime=0, Offline=1).
     */
    void updateProcessingMode(ProcessingMode mode);

    std::shared_ptr<mne::viz::qt::EvokedSetModel> m_pEvokedSetModel;     /**< The data model. */
    std::shared_ptr<mne::viz::qt::ChannelInfoModel> m_pChannelInfoModel; /**< Channel info model. */

    std::shared_ptr<mne::FiffInfo> m_pFiffInfo; /**< FiffInfo for currently loaded file. */

    QPointer<AverageLayoutWidget> m_pAverageLayoutWidget; /**< The tiles at their layout positions. */

    std::shared_ptr<std::map<std::string, QColor>> m_qMapAverageColor;    /**< Average colors. */
    std::shared_ptr<std::map<std::string, bool>> m_qMapAverageActivation; /**< Average activation status. */
    std::map<int, float> m_scaleMap;                                      /**< The current scaling map. */
};
} // NAMESPACE

#endif // AVERAGELAYOUTVIEW_H
