//=============================================================================================================
/**
 * @file     fiffrawview.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Simon Heinke <Simon.Heinke@tu-ilmenau.de>
 * @version  dev
 * @date     July, 2018
 *
 * @section  LICENSE
 *
 * Copyright (C) 2018, Lorenz Esch, Lars Debor, Simon Heinke. All rights reserved.
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
 * @brief    Declaration of the FiffRawView Class.
 *
 */

#ifndef FIFFRAWVIEW_H
#define FIFFRAWVIEW_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rawdataviewer_global.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QWidget>
#include <QPointer>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace ANSHAREDLIB {
    class FiffRawViewModel;
}

class QTableView;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE RAWDATAVIEWEREXTENSION
//=============================================================================================================

namespace RAWDATAVIEWEREXTENSION {


//*************************************************************************************************************
//=============================================================================================================
// RAWDATAVIEWEREXTENSION FORWARD DECLARATIONS
//=============================================================================================================

class FiffRawViewDelegate;


//=============================================================================================================
/**
 * TableView for Fiff data.
 */
class RAWDATAVIEWERSHARED_EXPORT FiffRawView : public QWidget
{
    Q_OBJECT

public:
    typedef QSharedPointer<FiffRawView> SPtr;            /**< Shared pointer type for FiffRawView. */
    typedef QSharedPointer<const FiffRawView> ConstSPtr; /**< Const shared pointer type for FiffRawView. */

    //=========================================================================================================
    /**
     * Constructs a FiffRawView which is a child of parent.
     *
     * @param [in] parent    The parent of widget.
     */
    FiffRawView(QWidget *parent = nullptr);

    //=========================================================================================================
    /**
     * Destructor.
     */
    virtual ~FiffRawView();

    //=========================================================================================================
    /**
     * Setup the model view controller.
     */
    void initMVCSettings(const QSharedPointer<ANSHAREDLIB::FiffRawViewModel> &pModel,
                         const QSharedPointer<FiffRawViewDelegate>& pDelegate);

private:
    //=========================================================================================================
    /**
     * resizeEvent reimplemented virtual function to handle resize events of the data dock window
     */
    void resizeEvent(QResizeEvent* event);

    QPointer<QTableView> m_pTableView;

signals:
    void tableViewDataWidthChanged(int iWidth);
};


//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================


} // NAMESPACE RAWDATAVIEWEREXTENSION

#endif // FIFFRAWVIEW_H
