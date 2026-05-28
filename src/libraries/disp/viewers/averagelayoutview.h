//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2022-2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *   Gabriel Motta <gabrielbenmotta@gmail.com>
 *
 * @file averagelayoutview.h
 * @since 2022
 * @date  April 2026
 * @brief 2-D sensor-layout view placing per-channel evoked traces at their physical positions.
 *
 * AverageLayoutView embeds a @c QGraphicsView driven by an
 * @ref AverageScene that draws an @ref AverageSceneItem (one mini
 * evoked-trace plot) at each sensor coordinate read from the active
 * @c Layout. It listens to an @ref EvokedSetModel for new averaged
 * responses and to a @ref ChannelInfoModel for channel-type / bad
 * flags, and supports per-condition colour selection and selective
 * channel display via @ref SelectionItem groups.
 */

#ifndef AVERAGELAYOUTVIEW_H
#define AVERAGELAYOUTVIEW_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp_global.h"
#include "abstractview.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QPointer>
#include <QMap>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class QGraphicsView;
class QGraphicsItem;

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

class AverageScene;
class EvokedSetModel;
class ChannelInfoModel;
class SelectionItem;

//=============================================================================================================
/**
 * @brief Sensor-layout viewer drawing per-channel evoked traces at their physical 2-D positions.
 *
 * Wraps a @c QGraphicsView around an @ref AverageScene fed by an
 * @ref EvokedSetModel and a @ref ChannelInfoModel; selection groups
 * are pushed in as @ref SelectionItem to show only a subset of
 * channels.
 */
class DISPSHARED_EXPORT AverageLayoutView : public AbstractView
{
    Q_OBJECT

public:
    typedef QSharedPointer<AverageLayoutView> SPtr;              /**< Shared pointer type for AverageLayoutView. */
    typedef QSharedPointer<const AverageLayoutView> ConstSPtr;   /**< Const shared pointer type for AverageLayoutView. */

    //=========================================================================================================
    /**
     * Constructs a AverageLayoutView which is a child of parent.
     *
     * @param[in] parent    parent of widget.
     */
    AverageLayoutView(const QString& sSettingsPath = "",
                      QWidget *parent = 0,
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
    void setChannelInfoModel(QSharedPointer<ChannelInfoModel> &pChannelInfoModel);

    //=========================================================================================================
    /**
     * Sets the evoked set model.
     *
     * @param[in] pEvokedSetModel     The new evoked set model.
     */
    void setEvokedSetModel(QSharedPointer<EvokedSetModel> pEvokedSetModel);

    //=========================================================================================================
    /**
     * Returns the currently set EvokedSetModel
     *
     * @return the currently set EvokedSetModel.
     */
    QSharedPointer<EvokedSetModel> getEvokedSetModel();

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
    void setScaleMap(const QMap<qint32, float> &scaleMap);

    //=========================================================================================================
    /**
     * Get the current average colors
     *
     * @return Pointer to the current average colors.
     */
    QSharedPointer<QMap<QString, QColor> > getAverageColor() const;

    //=========================================================================================================
    /**
     * Get the current average activations
     *
     * @return Pointer to the current average activations.
     */
    QSharedPointer<QMap<QString, bool> > getAverageActivation() const;

    //=========================================================================================================
    /**
     * Set the average colors
     *
     * @param[in] qMapAverageColor      Pointer to the new average colors.
     */
    void setAverageColor(const QSharedPointer<QMap<QString, QColor> > qMapAverageColor);

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
    void setAverageActivation(const QSharedPointer<QMap<QString, bool> > qMapAverageActivation);

    //=========================================================================================================
    /**
     * Stores FiffInfo to local variable to be used for getting channel info
     *
     * @param[in] pFiffInfo    shared pointer to Fiff Info of currently loaded data.
     */
    void setFiffInfo(const QSharedPointer<FIFFLIB::FiffInfo> pFiffInfo);

    //=========================================================================================================
    /**
     * call this whenever the external channel selection manager changes
     *
     * * @param[in] selectedChannelItems list of selected graphic items
     */
    void channelSelectionManagerChanged(const QList<QGraphicsItem *> &selectedChannelItems);

    //=========================================================================================================
    /**
     * Sets the currently viewable channels using the corrent name and location parameters
     *
     * @param[in] data     QVariant containing a SelectionItem object with selected channel information.
     */
    void channelSelectionChanged(const QVariant &data);

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

    QSharedPointer<AverageScene>                                m_pAverageScene;            /**< The pointer to the average scene. */
    QSharedPointer<DISPLIB::EvokedSetModel>                     m_pEvokedSetModel;          /**< The data model. */
    QSharedPointer<DISPLIB::ChannelInfoModel>                   m_pChannelInfoModel;        /**< Channel info model. */

    QSharedPointer<FIFFLIB::FiffInfo>                           m_pFiffInfo;                /**< FiffInfo for currently loaded file. */

    QPointer<QGraphicsView>                                     m_pAverageLayoutView;       /**< View for 2D average layout scene. */

    QSharedPointer<QMap<QString, QColor> >                      m_qMapAverageColor;         /**< Average colors. */
    QSharedPointer<QMap<QString, bool> >                        m_qMapAverageActivation;    /**< Average activation status. */
    QMap<qint32,float>                                          m_scaleMap;                 /**< The current scaling map. */
};
} // NAMESPACE

#endif // AVERAGELAYOUTVIEW_H
