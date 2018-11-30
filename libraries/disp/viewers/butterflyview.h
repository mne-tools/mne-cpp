//=============================================================================================================
/**
* @file     butterflyview.h
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     July, 2018
*
* @section  LICENSE
*
* Copyright (C) 2018, Lorenz Esch, Christoph Dinh and Matti Hamalainen. All rights reserved.
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

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp_global.h"


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QOpenGLWidget>
#include <QMap>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


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

class EvokedSetModel;
class ChannelInfoModel;


/**
* DECLARE CLASS ButterflyView
*
* @brief The ButterflyView class provides a butterfly view.
*/
class DISPSHARED_EXPORT ButterflyView : public QOpenGLWidget
{
    Q_OBJECT

public:
    typedef QSharedPointer<ButterflyView> SPtr;              /**< Shared pointer type for ButterflyView. */
    typedef QSharedPointer<const ButterflyView> ConstSPtr;   /**< Const shared pointer type for ButterflyView. */

    //=========================================================================================================
    /**
    * The constructor.
    */
    explicit ButterflyView(QWidget *parent = 0,
                           Qt::WindowFlags f = Qt::Widget);

    //=========================================================================================================
    /**
    * Set the evoked set model.
    *
    * @param [in] model     The new evoked set model.
    */
    void setModel(QSharedPointer<EvokedSetModel> model);

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
    * @param [in] modalityMap    The modality map.
    */
    void setModalityMap(const QMap<QString, bool>& modalityMap);

    //=========================================================================================================
    /**
    * Sets the scale map to scaleMap.
    *
    * @param [in] scaleMap map with all channel types and their current scaling value.
    */
    void setScaleMap(const QMap<qint32, float> &scaleMap);

    //=========================================================================================================
    /**
    * Set the selected channels.
    *
    * @param [in] selectedChannels     The new selected channels.
    */
    void setSelectedChannels(const QList<int> &selectedChannels);

    //=========================================================================================================
    /**
    * Perform a view update.
    */
    void updateView();

    //=========================================================================================================
    /**
    * Set the background color.
    *
    * @param [in] backgroundColor     The new background color.
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
    * @param [in] fileName     The file name and path where to store the screenshot.
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
    * @param [in] qMapAverageColor      Pointer to the new average colors
    */
    void setAverageColor(const QSharedPointer<QMap<QString, QColor> > qMapAverageColor);

    //=========================================================================================================
    /**
    * Set the average activations
    *
    * @param [in] qMapAverageActivation      Pointer to the new average activations
    */
    void setAverageActivation(const QSharedPointer<QMap<QString, bool> > qMapAverageActivation);

    //=========================================================================================================
    /**
    * Set the channel info model.
    *
    * @param [in] pChannelInfoModel     The new channel info model.
    */
    void setChannelInfoModel(QSharedPointer<ChannelInfoModel> &pChannelInfoModel);

    //=========================================================================================================
    /**
    * Only shows the channels defined in the QStringList selectedChannels
    *
    * @param [in] selectedChannels list of all channel names which are currently selected in the selection manager.
    */
    void showSelectedChannelsOnly(const QStringList& selectedChannels);

protected:
    //=========================================================================================================
    /**
    * Is called to paint the incoming real-time data block.
    * Function is painting the real-time butterfly plot
    *
    * @param [in] event pointer to PaintEvent -> not used.
    */
    virtual void paintGL();

    //=========================================================================================================
    /**
    * createPlotPath creates the QPointer path for the data plot.
    *
    * @param[in] index QModelIndex for accessing associated data and model object.
    */
    void createPlotPath(qint32 row, QPainter& painter) const;

    bool        m_bShowMAG;                     /**< Show Magnetometers channels */
    bool        m_bShowGRAD;                    /**< Show Gradiometers channels */
    bool        m_bShowEEG;                     /**< Show EEG channels */
    bool        m_bShowEOG;                     /**< Show EEG channels */
    bool        m_bShowMISC;                    /**< Show Miscellaneous channels */
    bool        m_bIsInit;                      /**< Whether this class has been initialized */

    QColor      m_colCurrentBackgroundColor;    /**< The current background color */

    QList<int>  m_lSelectedChannels;            /**< The currently selected channels */

    QMap<QString, bool>                     m_modalityMap;                  /**< Map of different modalities. */
    QMap<qint32,float>                      m_scaleMap;                     /**< Map with all channel types and their current scaling value.*/

    QSharedPointer<EvokedSetModel>          m_pEvokedSetModel;              /**< The evoked model */
    QSharedPointer<ChannelInfoModel>        m_pChannelInfoModel;            /**< The channel info model */

    QSharedPointer<QMap<QString, bool> >    m_qMapAverageActivation;        /**< Average activation status. */
    QSharedPointer<QMap<QString, QColor> >  m_qMapAverageColor;             /**< Average colors. */
};


//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

} // NAMESPACE

#endif // BUTTERFLYVIEW_H
