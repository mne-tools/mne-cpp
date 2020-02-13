//=============================================================================================================
/**
 * @file     brainflowsetupwidget.h
 * @author   Andrey Parfenov <a1994ndrey@gmail.com>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  dev
 * @date     February, 2019
 *
 * @section  LICENSE
 *
 * Copyright (C) 2019, Simon Heinke, Lorenz Esch. All rights reserved.
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
 * @brief Contains the declaration of the BrainFlowSetupWidget class.
 *
 */

#ifndef BRAINFLOWSETUPWIDGET_H
#define BRAINFLOWSETUPWIDGET_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "brainflowboard.h"

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QWidget>

namespace Ui {
class BrainFlowSetupWidget;
}

//=============================================================================================================
/**
 * DECLARE CLASS BrainFlowSetupWidget
 *
 * @brief The BrainFlowSetupWidget class provides the BrainFlowSetupWidget configuration window.
 */
class BrainFlowSetupWidget : public QWidget
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     * Construct BrainFlowSetupWidget object.
     *
     * @param [in] p_pBoard a pointer to BrainFlowBoard plugin
     * @param [in] parent a pointer to parent widget.
     *
     */
    explicit BrainFlowSetupWidget(BrainFlowBoard *p_pBoard, QWidget *parent = nullptr);
    ~BrainFlowSetupWidget();

    //=========================================================================================================
    /**
     * Prepares streaming session.
     *
     */
    void prepareSession();

    //=========================================================================================================
    /**
     * Releases streaming session.
     *
     */
    void releaseSession();

private:
    BrainFlowBoard *m_pBrainFlowBoard;
    Ui::BrainFlowSetupWidget *ui;
};

#endif // BRAINFLOWSETUPWIDGET_H
