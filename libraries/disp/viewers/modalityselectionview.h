//=============================================================================================================
/**
* @file     modalityselectionview.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     May, 2014
*
* @section  LICENSE
*
* Copyright (C) 2014, Christoph Dinh and Matti Hamalainen. All rights reserved.
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


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class QCheckBox;

namespace FIFFLIB {
    class FiffInfo;
}


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


//*************************************************************************************************************
//=============================================================================================================
// STRUCTS
//=============================================================================================================

struct Modality {
    QString m_sName;
    bool m_bActive;
    float m_fNorm;

    Modality(QString name, bool active, double norm)
    : m_sName(name), m_bActive(active), m_fNorm(norm)
    {}
};


//=============================================================================================================
/**
* DECLARE CLASS ModalitySelectionView
*
* @brief The ModalitySelectionView class provides a view to select between different modalities
*/
class DISPSHARED_EXPORT ModalitySelectionView : public QWidget
{
    Q_OBJECT

public:    
    typedef QSharedPointer<ModalitySelectionView> SPtr;              /**< Shared pointer type for ModalitySelectionView. */
    typedef QSharedPointer<const ModalitySelectionView> ConstSPtr;   /**< Const shared pointer type for ModalitySelectionView. */

    //=========================================================================================================
    /**
    * Constructs a ModalitySelectionView which is a child of parent.
    *
    * @param [in] parent        parent of widget
    */
    ModalitySelectionView(QWidget *parent = 0,
                          Qt::WindowFlags f = Qt::Widget);

    //=========================================================================================================
    /**
    * Init the view.
    *
    * @param [in] pFiffInfo    The fiff info.
    */
    void init(const QSharedPointer<FIFFLIB::FiffInfo> pFiffInfo);

    //=========================================================================================================
    /**
    * Init the view.
    *
    * @param [in] modalityList    The modality info.
    */
    void init(const QList<DISPLIB::Modality>& modalityList);

    //=========================================================================================================
    /**
    * Set the activation of the already created modality check boxes.
    *
    * @param [in] modalityList    The modality info.
    */
    void setModalities(const QList<Modality> &lModalities);

protected:
    //=========================================================================================================
    /**
    * Slot called when modality check boxes were changed
    */
    void onUpdateModalityCheckbox(qint32 state);

    QList<Modality>                     m_qListModalities;              /**< List of different modalities. */
    QList<QCheckBox*>                   m_qListModalityCheckBox;        /**< List of modality checkboxes. */

    QSharedPointer<FIFFLIB::FiffInfo>   m_pFiffInfo;                    /**< Connected fiff info. */

signals:
    //=========================================================================================================
    /**
    * Emit this signal whenever the user changed the modality.
    */
    void modalitiesChanged(const QList<Modality>& modalityList);

};

} // NAMESPACE

#endif // MODALITYSELECTIONVIEW_H
