//=============================================================================================================
/**
* @file     filtersettingsview.h
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
* @brief    Declaration of the FilterSettingsView Class.
*
*/

#ifndef FILTERSETTINGSVIEW_H
#define FILTERSETTINGSVIEW_H


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


//=============================================================================================================
/**
* DECLARE CLASS FilterSettingsView
*
* @brief The FilterSettingsView class provides a view to select between different modalities
*/
class DISPSHARED_EXPORT FilterSettingsView : public QWidget
{
    Q_OBJECT

public:    
    typedef QSharedPointer<FilterSettingsView> SPtr;              /**< Shared pointer type for FilterSettingsView. */
    typedef QSharedPointer<const FilterSettingsView> ConstSPtr;   /**< Const shared pointer type for FilterSettingsView. */

    //=========================================================================================================
    /**
    * Constructs a FilterSettingsView which is a child of parent.
    *
    * @param [in] parent        parent of widget
    */
    FilterSettingsView(QWidget *parent = 0,
                Qt::WindowFlags f = Qt::Widget);

    //=========================================================================================================
    /**
    * Call this whenever the current filters have changed.
    *
    * @param [in] list    list of QCheckBoxes which are to be added to the filter group
    */
    void filterGroupChanged(QList<QCheckBox*> list);

protected:
    //=========================================================================================================
    /**
    * Slot called when the user designed filter was toggled
    */
    void onUserFilterToggled(bool state);

    //=========================================================================================================
    /**
    * Show the filter option screen to the user.
    *
    * @param [in] state toggle state.
    */
    void onShowFilterOptions(bool state);

    QList<QCheckBox*>                                   m_qFilterListCheckBox;          /**< List of filter CheckBox. */
    QPushButton*                                        m_pShowFilterOptions;           /**< Holds the show filter options button. */

signals:
    //=========================================================================================================
    /**
    * Emit this signal whenever the user is supposed to see the filter option window.
    */
    void showFilterOptions(bool state);

};

} // NAMESPACE

#endif // FILTERSETTINGSVIEW_H
