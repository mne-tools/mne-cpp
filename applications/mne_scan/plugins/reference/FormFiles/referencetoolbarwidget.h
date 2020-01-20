//=============================================================================================================
/**
 * @file     referencetoolbarwidget.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Viktor Klueber <Viktor.Klueber@tu-ilmenau.de>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  dev
 * @date     February, 2017
 *
 * @section  LICENSE
 *
 * Copyright (C) 2017, Christoph Dinh, Viktor Klueber, Lorenz Esch. All rights reserved.
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
 * @brief    Contains the declaration of the ReferenceToolbarWidget class.
 *
 */

#ifndef REFERENCETOOLBARWIDGET_H
#define REFERENCETOOLBARWIDGET_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "reference_global.h"
#include "../reference.h"
#include "../ui_referencetoolbar.h"

#include <fiff/fiff_info.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QWidget>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE REFERENCEPLUGIN
//=============================================================================================================

namespace REFERENCEPLUGIN
{


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class Reference;


//=============================================================================================================
/**
 * DECLARE CLASS ReferenceToolbarWidget
 *
 * @brief The ReferenceToolbarWidget class provides a dummy toolbar widget structure.
 */

class REFERENCESHARED_EXPORT ReferenceToolbarWidget : public QWidget
{
    Q_OBJECT

public:
    typedef QSharedPointer<ReferenceToolbarWidget> SPtr;         /**< Shared pointer type for ReferenceToolbarWidget object. */
    typedef QSharedPointer<ReferenceToolbarWidget> ConstSPtr;    /**< Const shared pointer type for ReferenceToolbarWidget object. */

    //=========================================================================================================
    /**
    * Constructs a ReferenceToolbarWidget.
    */
    explicit ReferenceToolbarWidget(REFERENCEPLUGIN::Reference *pRef, QWidget *parent = 0);

    //=========================================================================================================
    /**
    * Destroys the ReferenceToolbarWidget.
    */
    ~ReferenceToolbarWidget();

public slots:
    //=========================================================================================================
    /**
    * updates the channels and sets them to the QListWidget
    */
    void updateChannels(FIFFLIB::FiffInfo::SPtr &pFiffInfo);

private:
    Ui::ReferenceToolbarWidget*         ui;     /**< The UI class specified in the designer. */

    Reference*                      m_pRef;     /**< pointer to the Reference object */
};

} //REFERENCEPLUGIN

#endif // REFERENCETOOLBARWIDGET_H
