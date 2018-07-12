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


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QWidget>
#include <QPointer>
#include <QMap>


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
    * @param [in] parent    parent of widget
    */
    ChannelDataView(QWidget* parent = 0,
                 Qt::WindowFlags f = Qt::Widget);

    void init(QSharedPointer<FIFFLIB::FiffInfo> &info);

    void addData(const QList<Eigen::MatrixXd>& data);

    Eigen::MatrixXd getLastBlock();

    //=========================================================================================================
    /**
    * Is called when mouse wheel is used.
    * Function is selecting the tool (freezing/annotation);
    *
    * @param object
    * @param event o
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
    void setBackgroundColorChanged(const QColor& backgroundColor);

    QMap<qint32, float> getScalingMap();

    //=========================================================================================================
    /**
    * Broadcast channel scaling
    *
    * @param [in] scaleMap QMap with scaling values which is to be broadcasted to the model.
    */
    void setScalingMap(QMap<qint32, float> scaleMap);

    //=========================================================================================================
    /**
    * Set the signal color.
    *
    * @param [in] signalColor  The new signal color.
    */
    void setSignalColor(const QColor& signalColor);

    //=========================================================================================================
    /**
    * Hides/shows all bad channels in the view
    */
    void hideBadChannels();

    bool getBadChannelHideStatus();

    //=========================================================================================================
    /**
    * Only shows the channels defined in the QStringList selectedChannels
    *
    * @param [in] selectedChannels list of all channel names which are currently selected in the selection manager.
    */
    void showSelectedChannelsOnly(QStringList selectedChannels);

    //=========================================================================================================
    /**
    * Sets new zoom factor
    *
    * @param [in] zoomFac  time window size;
    */
    void setZoom(double zoomFac);

    double getZoom();

    //=========================================================================================================
    /**
    * Sets new time window size
    *
    * @param [in] T  time window size;
    */
    void setWindowSize(int T);

    int getWindowSize();

    void takeScreenshot(const QString& fileName);

    //=========================================================================================================
    /**
    * Update the SSP projection
    */
    void updateProjection();

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
    void updateSpharaOptions(const QString& sSytemType, int nBaseFctsFirst, int nBaseFctsSecond);

    //=========================================================================================================
    /**
    * Filter parameters changed
    *
    * @param[in] filterData    list of the currently active filter
    */
    void filterChanged(QList<UTILSLIB::FilterData> filterData);

    //=========================================================================================================
    /**
    * Filter avtivated
    *
    * @param[in] state    filter on/off flag
    */
    void filterActivated(bool state);

    //=========================================================================================================
    /**
    * Sets the type of channel which are to be filtered
    *
    * @param[in] channelType    the channel type which is to be filtered (EEG, MEG, All)
    */
    void setFilterChannelType(QString channelType);

    //=========================================================================================================
    /**
    * markChBad marks the selected channels as bad/good in m_chInfolist
    *
    * @param colorMap       color for each trigger channel
    * @param activ          real time trigger detection active
    * @param triggerCh      current trigger channel to scan
    * @param threshold      threshold for the detection process
    */
    void triggerInfoChanged(const QMap<double, QColor>& colorMap, bool active, QString triggerCh, double threshold);

    //=========================================================================================================
    /**
    * distanceTimeSpacerChanged changes the distance of the time spacers
    *
    * @param value the new distance for the time spacers
    */
    void distanceTimeSpacerChanged(int value);

    //=========================================================================================================
    /**
    * resetTriggerCounter resets the trigger counter
    */
    void resetTriggerCounter();

protected:
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
    *
    * @param [in] value unused int.
    */
    void visibleRowsChanged(int value);

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

signals:    
    //=========================================================================================================
    /**
    * position is emitted whenever user moves the mouse inside of the table view viewport
    *
    * @param position   the current mouse position
    * @param activeRow  the current row which the mouse is moved over
    */
    void markerMoved(QPoint position, int activeRow);

    //=========================================================================================================
    /**
    * Emmited when trigger detection was performed
    */
    void triggerDetected(int numberDetectedTriggers, const QMap<int,QList<QPair<int,double> > >& mapDetectedTriggers);

    void channelMarkingChanged();
};

} // NAMESPACE

#endif // CHANNELDATAVIEW_H
