//=============================================================================================================
/**
* @file     channeldataview.h
* @author   Lorenz Esch <lesc@mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     July, 2018
*
* @section  LICENSE
*
* Copyright (C) 2018, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
* @brief    Declaration of the ChannelDataView Class.
*
*/

#ifndef CHANNELDATAVIEW_H
#define CHANNELDATAVIEW_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp_global.h"

#include <fiff/fiff_proj.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QPointer>
#include <QMap>
#include <QWidget>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class QTableView;

namespace FIFFLIB {
    class FiffInfo;
}

namespace UTILSLIB {
    class FilterData;
}


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE DISPLIB
//=============================================================================================================

namespace DISPLIB
{


//*************************************************************************************************************
//=============================================================================================================
// DISPLIB FORWARD DECLARATIONS
//=============================================================================================================

class ChannelDataModel;
class ChannelDataDelegate;


//=============================================================================================================
/**
* DECLARE CLASS ChannelDataView
*
* @brief The ChannelDataView class provides a channel view display
*/
class DISPSHARED_EXPORT ChannelDataView : public QWidget
{    
    Q_OBJECT

public:
    typedef QSharedPointer<ChannelDataView> SPtr;              /**< Shared pointer type for ChannelDataView. */
    typedef QSharedPointer<const ChannelDataView> ConstSPtr;   /**< Const shared pointer type for ChannelDataView. */

    //=========================================================================================================
    /**
    * Constructs a ChannelDataView which is a child of parent.
    *
    * @param [in] parent    The parent of widget.
    */
    ChannelDataView(const QString& sSettingsPath = "",
                    QWidget* parent = 0,
                    Qt::WindowFlags f = Qt::Widget);

    //=========================================================================================================
    /**
    * Destroys the ChannelDataView.
    */
    ~ChannelDataView();

    //=========================================================================================================
    /**
    * Initilaizes the ChannelDataView based on a FiffInfo.
    *
    * @param [in] info    The FiffInfo.
    */
    void init(QSharedPointer<FIFFLIB::FiffInfo> &info);

    //=========================================================================================================
    /**
    * Add data to the view.
    *
    * @param [in] data    The new data.
    */
    void addData(const QList<Eigen::MatrixXd>& data);

    //=========================================================================================================
    /**
    * Get the latest data block from the underlying model.
    *
    * @return    The last data block.
    */
    Eigen::MatrixXd getLastBlock();

    //=========================================================================================================
    /**
    * Is called when mouse wheel is used.
    * Function is selecting the tool (freezing/annotation);
    *
    * @param object
    * @param event
    *
    * @return
    */
    bool eventFilter(QObject *object, QEvent *event);

    //=========================================================================================================
    /**
    * Broadcast the background color changes made in the QuickControl widget
    *
    * @param [in] backgroundColor  The new background color.
    */
    void setBackgroundColor(const QColor& backgroundColor);

    //=========================================================================================================
    /**
    * Returns the current background color.
    *
    * @return  The current background color.
    */
    QColor getBackgroundColor();

    //=========================================================================================================
    /**
    * Get the current scaling map.
    *
    * @return    The current scaling map.
    */
    QMap<qint32, float> getScalingMap();

    //=========================================================================================================
    /**
    * Broadcast channel scaling
    *
    * @param [in] scaleMap QMap with scaling values which is to be broadcasted to the model.
    */
    void setScalingMap(const QMap<qint32, float>& scaleMap);

    //=========================================================================================================
    /**
    * Set the signal color.
    *
    * @param [in] signalColor  The new signal color.
    */
    void setSignalColor(const QColor& signalColor);

    //=========================================================================================================
    /**
    * Returns the signal color.
    *
    * @return  The current signal color.
    */
    QColor getSignalColor();

    //=========================================================================================================
    /**
    * Hides/shows all bad channels in the view
    */
    void hideBadChannels();

    //=========================================================================================================
    /**
    * Get whether the bad channels are currently hidden or not.
    *
    * @return    The current bad channel hidden status.
    */
    bool getBadChannelHideStatus();

    //=========================================================================================================
    /**
    * Only shows the channels defined in the QStringList selectedChannels
    *
    * @param [in] selectedChannels list of all channel names which are currently selected in the selection manager.
    */
    void showSelectedChannelsOnly(const QStringList& selectedChannels);

    //=========================================================================================================
    /**
    * Sets new zoom factor
    *
    * @param [in] zoomFac  time window size;
    */
    void setZoom(double zoomFac);

    //=========================================================================================================
    /**
    * Get the current zoom.
    *
    * @return    The current zoom.
    */
    double getZoom();

    //=========================================================================================================
    /**
    * Sets new time window size
    *
    * @param [in] T  time window size;
    */
    void setWindowSize(int T);

    //=========================================================================================================
    /**
    * Get the current window size.
    *
    * @return    The current window size.
    */
    int getWindowSize();

    //=========================================================================================================
    /**
    * Renders a screenshot of the view and saves it to the passed path. SVG and PNG supported.
    *
    * @param [in] fileName     The file name and path where to store the screenshot.
    */
    void takeScreenshot(const QString& fileName);

    //=========================================================================================================
    /**
    * Update the SSP projection
    *
    * @param [in] projs    The new projectors.
    */
    void updateProjection(const QList<FIFFLIB::FiffProj>& projs);

    //=========================================================================================================
    /**
    * Update the compensator
    *
    * @param[in] to    Compensator to use in fiff constant format FiffCtfComp.kind (NOT FiffCtfComp.ctfkind)
    */
    void updateCompensator(int to);

    //=========================================================================================================
    /**
    * Update the SPHARA operator
    *
    * @param[in] state            The current state of teh SPHARA tool
    */
    void updateSpharaActivation(bool state);

    //=========================================================================================================
    /**
    * Update the SPHARA operator
    *
    * @param[in] sSystemType            The current acquisition system type (VectorView, BabyMEG, EEG)
    * @param[in] nBaseFctsFirst         The new number of basis function to use for the first SPHARA operator
    * @param[in] nBaseFctsSecond        The new number of basis function to use for the second SPHARA operator
    */
    void updateSpharaOptions(const QString& sSytemType,
                             int nBaseFctsFirst,
                             int nBaseFctsSecond);

    //=========================================================================================================
    /**
    * Filter parameters changed
    *
    * @param[in] filterData   the currently active filter
    */
    void setFilter(const UTILSLIB::FilterData &filterData);

    //=========================================================================================================
    /**
    * Filter avtivated
    *
    * @param[in] state    filter on/off flag
    */
    void setFilterActive(bool state);

    //=========================================================================================================
    /**
    * Sets the type of channel which are to be filtered
    *
    * @param[in] channelType    the channel type which is to be filtered (EEG, MEG, All)
    */
    void setFilterChannelType(const QString& channelType);

    //=========================================================================================================
    /**
    * markChBad marks the selected channels as bad/good in m_chInfolist
    *
    * @param colorMap       color for each trigger channel
    * @param activ          real time trigger detection active
    * @param triggerCh      current trigger channel to scan
    * @param threshold      threshold for the detection process
    */
    void triggerInfoChanged(const QMap<double, QColor>& colorMap,
                            bool active,
                            const QString& triggerCh,
                            double threshold);

    //=========================================================================================================
    /**
    * distanceTimeSpacerChanged changes the distance of the time spacers
    *
    * @param value the new distance for the time spacers
    */
    void setDistanceTimeSpacer(int value);

    //=========================================================================================================
    /**
    * Returns teh current distance between time spacers.
    *
    * @return The current distance between the time spacers
    */
    int getDistanceTimeSpacer();

    //=========================================================================================================
    /**
    * resetTriggerCounter resets the trigger counter
    */
    void resetTriggerCounter();

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
    * Show channel context menu
    *
    * @param [in] pos   Position to popup the conext menu.
    */
    void channelContextMenu(QPoint pos);

    //=========================================================================================================
    /**
    * apply the in m_qListCurrentSelection stored selection -> hack around C++11 lambda
    */
    void applySelection();

    //=========================================================================================================
    /**
    * hides the in m_qListCurrentSelection stored selection -> hack around C++11 lambda
    */
    void hideSelection();

    //=========================================================================================================
    /**
    * reset the in m_qListCurrentSelection stored selection -> hack around C++11 lambda
    */
    void resetSelection();

    //=========================================================================================================
    /**
    * Gets called when the views in the viewport of the table view change
    */
    void visibleRowsChanged();

    //=========================================================================================================
    /**
    * Gets called when the bad channels are about to be marked as bad or good
    */
    void markChBad();

    QPointer<QTableView>                        m_pTableView;                   /**< The QTableView being part of the model/view framework of Qt */
    QPointer<DISPLIB::ChannelDataDelegate>      m_pDelegate;                    /**< The channel data delegate */
    QPointer<DISPLIB::ChannelDataModel>         m_pModel;                       /**< The channel data model */

    QMap<qint32,float>                          m_qMapChScaling;                /**< Channel scaling values. */

    qint32                                      m_iT;                           /**< Display window size in seconds */
    QSharedPointer<FIFFLIB::FiffInfo>           m_pFiffInfo;                    /**< FiffInfo, which is used insteadd of ListChInfo*/
    float                                       m_fSamplingRate;                /**< Sampling rate */
    float                                       m_fDefaultSectionSize;          /**< Default row height */
    float                                       m_fZoomFactor;                  /**< Zoom factor */
    QList<qint32>                               m_qListBadChannels;             /**< Current list of bad channels  */
    QList<qint32>                               m_qListCurrentSelection;        /**< Current selection list -> hack around C++11 lambda  */
    bool                                        m_bHideBadChannels;             /**< hide bad channels flag. */
    QStringList                                 m_slSelectedChannels;           /**< the currently selected channels from the selection manager window. */
    QColor                                      m_backgroundColor;              /**< Current background color. */
    int                                         m_iDistanceTimeSpacer;          /**< Current distance between time spacer. */

    QString                                     m_sSettingsPath;                /**< The settings path to store the GUI settings to. */

signals:    
    //=========================================================================================================
    /**
    * position is emitted whenever user moves the mouse inside of the table view viewport
    *
    * @param position   the current mouse position
    * @param activeRow  the current row which the mouse is moved over
    */
    void markerMoved(QPoint position,
                     int activeRow);

    //=========================================================================================================
    /**
    * Emmited when trigger detection was performed
    */
    void triggerDetected(int numberDetectedTriggers,
                         const QMap<int,QList<QPair<int,double> > >& mapDetectedTriggers);

    void channelMarkingChanged();
};

} // NAMESPACE

#endif // CHANNELDATAVIEW_H
