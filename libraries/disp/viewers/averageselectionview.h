//=============================================================================================================
/**
* @file     averageselectionview.h
* @author   Lorenz Esch <lesch@mgh.harvard.edu>;
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
* @brief    Declaration of the AverageSelectionView Class.
*
*/

#ifndef AVERAGESELECTIONVIEW_H
#define AVERAGESELECTIONVIEW_H


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
#include <QMap>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class QPushButton;
class QCheckBox;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE DISPLIB
//=============================================================================================================

namespace DISPLIB
{

struct AverageSelectionInfo {
    QColor color;
    QString name;
    bool active;
};


//*************************************************************************************************************
//=============================================================================================================
// DISPLIB FORWARD DECLARATIONS
//=============================================================================================================


//=============================================================================================================
/**
* DECLARE CLASS AverageSelectionView
*
* @brief The AverageSelectionView class provides a view to select different averages
*/
class DISPSHARED_EXPORT AverageSelectionView : public QWidget
{
    Q_OBJECT

public:
    typedef QSharedPointer<AverageSelectionView> SPtr;              /**< Shared pointer type for AverageSelectionView. */
    typedef QSharedPointer<const AverageSelectionView> ConstSPtr;   /**< Const shared pointer type for AverageSelectionView. */

    //=========================================================================================================
    /**
    * Constructs a AverageSelectionView which is a child of parent.
    *
    * @param [in] parent        parent of widget
    */
    AverageSelectionView(QWidget *parent = 0,
                         Qt::WindowFlags f = Qt::Widget);

    //=========================================================================================================
    /**
    * Init the view
    */
    void init();

    //=========================================================================================================
    /**
    * Set the old average map which holds the inforamtion about the calcuated averages.
    *
    * @param [in] qMapAverageInfoOld     the old average info map.
    */
    void setAverageInformationMapOld(const QMap<double, AverageSelectionInfo>& qMapAverageInfoOld);

    //=========================================================================================================
    /**
    * Set the average map which holds the inforamtion about the currently calcuated averages.
    *
    * @param [in] qMapAverageSelectionInfo     the average map.
    */
    void setAverageInformationMap(const QMap<double, AverageSelectionInfo>& qMapAverageSelectionInfo);

    //=========================================================================================================
    /**
    * Create list of channels which are to be filtered based on channel names
    *
    * @return the average information map
    */
    QMap<double, AverageSelectionInfo> getAverageInformationMap();

protected:
    //=========================================================================================================
    /**
    * Call this slot whenever the averages changed.
    */
    void onAveragesChanged();

    QMap<double, AverageSelectionInfo>      m_qMapAverageInfo;              /**< Average colors and names. */
    QMap<double, AverageSelectionInfo>      m_qMapAverageInfoOld;           /**< Old average colors and names. */
    QMap<QCheckBox*, double>                m_qMapChkBoxAverageType;        /**< Check box to average type map. */
    QMap<QPushButton*, double>              m_qMapButtonAverageType;        /**< Push button to average type map. */

signals:
    //=========================================================================================================
    /**
    * Emit this signal whenever the user wants to make a screenshot.
    *
    * @param[out] map     The current average map.
    */
    void averageInformationChanged(const QMap<double, AverageSelectionInfo>& map);

};

} // NAMESPACE

#ifndef metatype_averageselectioninfo
#define metatype_averageselectioninfo
Q_DECLARE_METATYPE(DISPLIB::AverageSelectionInfo); /**< Provides QT META type declaration of the DISPLIBB::AverageSelectionInfo type. For signal/slot usage.*/
#endif

#endif // AVERAGESELECTIONVIEW_H
