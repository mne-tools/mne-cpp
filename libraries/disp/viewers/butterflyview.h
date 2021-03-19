//=============================================================================================================
/**
 * @file     butterflyview.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2018
 *
 * @section  LICENSE
 *
 * Copyright (C) 2018, Lorenz Esch, Christoph Dinh. All rights reserved.
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
 * @brief    Declaration of the ButterflyView class.
 *
 */

#ifndef BUTTERFLYVIEW_H
#define BUTTERFLYVIEW_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp_global.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QMap>
#include <QWidget>
#if !defined(NO_QOPENGLWIDGET)
#include <QOpenGLWidget>
#endif

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
// DEFINE NAMESPACE DISPLIB
//=============================================================================================================

namespace DISPLIB
{

//=============================================================================================================
// DISPLIB FORWARD DECLARATIONS
//=============================================================================================================

class EvokedSetModel;
class ChannelInfoModel;

/**
 * DECLARE CLASS ButterflyView
 *
 * @brief The ButterflyView class provides a butterfly view.
 */
#if !defined(NO_QOPENGLWIDGET)
class DISPSHARED_EXPORT ButterflyView : public QOpenGLWidget
#else
class DISPSHARED_EXPORT ButterflyView : public QWidget
#endif
{
    Q_OBJECT

public:
    typedef QSharedPointer<ButterflyView> SPtr;              /**< Shared pointer type for ButterflyView. */
    typedef QSharedPointer<const ButterflyView> ConstSPtr;   /**< Const shared pointer type for ButterflyView. */

    //=========================================================================================================
    /**
     * The constructor.
     */
    explicit ButterflyView(const QString& sSettingsPath = "",
                           QWidget *parent = 0,
                           Qt::WindowFlags f = Qt::Widget);

    //=========================================================================================================
    /**
     * Destroys the ButterflyView.
     */
    ~ButterflyView();

    //=========================================================================================================
    /**
     * Update the OpenGL viewport. This, e.g., necessary if this widget was set to a QDockWidget which changes
     * its floating state.
     */
    void updateOpenGLViewport();

    //=========================================================================================================
    /**
     * Set the evoked set model.
     *
     * @param[in] model     The new evoked set model.
     */
    void setEvokedSetModel(QSharedPointer<EvokedSetModel> model);

    //=========================================================================================================
    /**
     * Returns the currently set EvokedSetModel
     *
     * @return the currently set EvokedSetModel.
     */
    QSharedPointer<EvokedSetModel> getEvokedSetModel();

    //=========================================================================================================
    /**
     * Perform a data update.
     */
    void dataUpdate();

    //=========================================================================================================
    /**
     * Get the activation of the already created modality check boxes.
     *
     * @return The current modality map.
     */
    QMap<QString, bool> getModalityMap();

    //=========================================================================================================
    /**
     * Set the modality checkboxes.
     *
     * @param[in] modalityMap    The modality map.
     */
    void setModalityMap(const QMap<QString, bool>& modalityMap);

    //=========================================================================================================
    /**
     * Sets the scale map to scaleMap.
     *
     * @param[in] scaleMap map with all channel types and their current scaling value.
     */
    void setScaleMap(const QMap<qint32, float> &scaleMap);

    //=========================================================================================================
    /**
     * Set the selected channels.
     *
     * @param[in] selectedChannels     The new selected channels.
     */
    void setSelectedChannels(const QList<int> &selectedChannels);

    //=========================================================================================================
    /**
     * Perform a view update from outside of this class.
     */
    void updateView();

    //=========================================================================================================
    /**
     * Set the background color.
     *
     * @param[in] backgroundColor     The new background color.
     */
    void setBackgroundColor(const QColor& backgroundColor);

    //=========================================================================================================
    /**
     * Returns the background color.
     *
     * @return The current background color.
     */
    const QColor& getBackgroundColor();

    //=========================================================================================================
    /**
     * Renders a screenshot of the view and saves it to the passed path. SVG and PNG supported.
     *
     * @param[in] fileName     The file name and path where to store the screenshot.
     */
    void takeScreenshot(const QString& fileName);

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
     * Set the channel info model.
     *
     * @param[in] pChannelInfoModel     The new channel info model.
     */
    void setChannelInfoModel(QSharedPointer<ChannelInfoModel> &pChannelInfoModel);

    //=========================================================================================================
    /**
     * Only shows the channels defined in the QStringList selectedChannels
     *
     * @param[in] selectedChannels list of all channel names which are currently selected in the selection manager.
     */
    void showSelectedChannelsOnly(const QStringList& selectedChannels);

    //=========================================================================================================
    /**
     * Shows selected channels based on a list of selected channel indices
     *
     * @param[in] selectedChannelsIndexes  list of indices of channels channels to be displayed.
     */
    void showSelectedChannels(const QList<int> selectedChannelsIndexes);

    //=========================================================================================================
    /**
     * Shows all channels in view
     */
    void showAllChannels();

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
     * Is called to paint the incoming real-time data block.
     * Function is painting the real-time butterfly plot
     *
     * @param[in] event pointer to PaintEvent -> not used.
     */
    #if !defined(NO_QOPENGLWIDGET)
    virtual void paintGL();
    #else
    virtual void paintEvent(QPaintEvent *event);
    #endif

    //=========================================================================================================
    /**
     * createPlotPath creates the QPointer path for the data plot.
     *
     * @param[in] index QModelIndex for accessing associated data and model object.
     */
    void createPlotPath(qint32 row, QPainter& painter) const;

    bool        m_bShowMAG;                     /**< Show Magnetometers channels. */
    bool        m_bShowGRAD;                    /**< Show Gradiometers channels. */
    bool        m_bShowEEG;                     /**< Show EEG channels. */
    bool        m_bShowEOG;                     /**< Show EEG channels. */
    bool        m_bShowMISC;                    /**< Show Miscellaneous channels. */
    bool        m_bIsInit;                      /**< Whether this class has been initialized. */

    QString     m_sSettingsPath;                /**< The settings path to store the GUI settings to. */

    QColor      m_colCurrentBackgroundColor;    /**< The current background color. */

    QList<int>  m_lSelectedChannels;            /**< The currently selected channels. */

    QMap<QString, bool>                     m_modalityMap;                  /**< Map of different modalities. */
    QMap<qint32,float>                      m_scaleMap;                     /**< Map with all channel types and their current scaling value.*/

    QSharedPointer<EvokedSetModel>          m_pEvokedSetModel;              /**< The evoked model. */
    QSharedPointer<ChannelInfoModel>        m_pChannelInfoModel;            /**< The channel info model. */

    QSharedPointer<QMap<QString, bool> >    m_qMapAverageActivation;        /**< Average activation status. */
    QSharedPointer<QMap<QString, QColor> >  m_qMapAverageColor;             /**< Average colors. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE

#endif // BUTTERFLYVIEW_H
