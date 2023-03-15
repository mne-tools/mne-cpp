//=============================================================================================================
/**
 * @file     modalityselectionview.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     May, 2014
 *
 * @section  LICENSE
 *
 * Copyright (C) 2014, Lorenz Esch, Christoph Dinh. All rights reserved.
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
 * @brief    Declaration of the ModalitySelectionView Class.
 *
 */

#ifndef MODALITYSELECTIONVIEW_H
#define MODALITYSELECTIONVIEW_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp_global.h"
#include "abstractview.h"

#include <fiff/fiff_ch_info.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QMap>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class QCheckBox;

//=============================================================================================================
// DEFINE NAMESPACE DISPLIB
//=============================================================================================================

namespace DISPLIB
{

//=============================================================================================================
// DISPLIB FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * DECLARE CLASS ModalitySelectionView
 *
 * @brief The ModalitySelectionView class provides a view to select between different modalities
 */
class DISPSHARED_EXPORT ModalitySelectionView : public AbstractView
{
    Q_OBJECT

public:
    typedef QSharedPointer<ModalitySelectionView> SPtr;              /**< Shared pointer type for ModalitySelectionView. */
    typedef QSharedPointer<const ModalitySelectionView> ConstSPtr;   /**< Const shared pointer type for ModalitySelectionView. */

    //=========================================================================================================
    /**
     * Constructs a ModalitySelectionView which is a child of parent.
     *
     * @param[in] parent        parent of widget.
     */
    ModalitySelectionView(const QList<FIFFLIB::FiffChInfo> &lChannelList,
                          const QString& sSettingsPath = "",
                          QWidget *parent = 0,
                          Qt::WindowFlags f = Qt::Widget);

    //=========================================================================================================
    /**
     * Destroys the ModalitySelectionView.
     */
    ~ModalitySelectionView();

    //=========================================================================================================
    /**
     * Set the modality checkboxes.
     *
     * @param[in] modalityMap    The modality map.
     */
    void setModalityMap(const QMap<QString, bool>& modalityMap);

    //=========================================================================================================
    /**
     * Get the activation of the already created modality check boxes.
     *
     * @return The current modality map.
     */
    QMap<QString, bool> getModalityMap();

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
     * @param[in] mode     The new mode (Clinical=0, Research=1).
     */
    void updateGuiMode(GuiMode mode);

    //=========================================================================================================
    /**
     * Update the views GUI based on the set ProcessingMode (RealTime=0, Offline=1).
     *
     * @param[in] mode     The new mode (RealTime=0, Offline=1).
     */
    void updateProcessingMode(ProcessingMode mode);

    //=========================================================================================================
    /**
     * Redraw the GUI.
     */
    void redrawGUI();

    //=========================================================================================================
    /**
     * Slot called when modality check boxes were changed
     */
    void onUpdateModalityCheckbox(qint32 state);

    QMap<QString, bool>                 m_modalityMap;                  /**< Map of different modalities. */
    QList<QCheckBox*>                   m_qListModalityCheckBox;        /**< List of modality checkboxes. */

    QStringList                         m_lChannelTypeList;             /**< Channel type list. */

    QString                             m_sSettingsPath;                /**< The settings path to store the GUI settings to. */

signals:
    //=========================================================================================================
    /**
     * Emit this signal whenever the user changed the modality.
     */
    void modalitiesChanged(const QMap<QString, bool>& modalityMap);
};
} // NAMESPACE

#endif // MODALITYSELECTIONVIEW_H
