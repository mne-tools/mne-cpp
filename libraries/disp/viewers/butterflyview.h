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

#include "modalityselectionview.h"
#include "averageselectionview.h"


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QWidget>
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
class DISPSHARED_EXPORT ButterflyView : public QWidget
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
    *
    * @param [in] topLeft       The new top left index.
    * @param [in] bottomRight   The new bottom right index.
    * @param [in] roles         The new roles.
    */
    void dataUpdate(const QModelIndex& topLeft,
                    const QModelIndex& bottomRight,
                    const QVector<int>& roles = QVector<int>());

    //=========================================================================================================
    /**
    * Returns the modality list.
    *
    * @return A list with all currently selected modalities.
    */
    QList<Modality> getModalities();

    //=========================================================================================================
    /**
    * Set the modalities.
    *
    * @param [in] p_qListModalities     The new modalities.
    */
    void setModalities(const QList<Modality>& p_qListModalities);

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
    * Set the average map information
    *
    * @param [in] mapAvr     The average data information including the color per average type.
    */
    void setAverageInformationMap(const QMap<double, AverageSelectionInfo>& mapAvr);

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
    virtual void paintEvent(QPaintEvent* paintEvent );

private:
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

    float       m_fMaxMAG;                      /**< Scale for Magnetometers channels */
    float       m_fMaxGRAD;                     /**< Scale for Gradiometers channels */
    float       m_fMaxEEG;                      /**< Scale for EEG channels */
    float       m_fMaxEOG;                      /**< Scale for EEG channels */
    float       m_fMaxMISC;                     /**< Scale for Miscellaneous channels */

    qint32      m_iNumChannels;                 /**< Number of channels */

    QColor      m_colCurrentBackgroundColor;    /**< The current background color */

    QList<int>  m_lSelectedChannels;            /**< The currently selected channels */

    QList<DISPLIB::Modality>                m_qListModalities;                      /**< The list of currently selected modalities */

    QSharedPointer<EvokedSetModel>          m_pEvokedModel;                         /**< The evoked model */
    QSharedPointer<ChannelInfoModel>        m_pChannelInfoModel;                    /**< The channel info model */

    QMap<double, AverageSelectionInfo>      m_qMapAverageColor;     /**< The current average color information. */
};


//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

} // NAMESPACE

Q_DECLARE_METATYPE(QList<DISPLIB::Modality>);

#endif // BUTTERFLYVIEW_H
