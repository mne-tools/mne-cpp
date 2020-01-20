//=============================================================================================================
/**
 * @file     rthpisetupwidget.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  dev
 * @date     March, 2015
 *
 * @section  LICENSE
 *
 * Copyright (C) 2015, Christoph Dinh, Lorenz Esch. All rights reserved.
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
 * @brief    Contains the declaration of the RtHpiSetupWidget class.
 *
 */

#ifndef RTHPISETUPWIDGET_H
#define RTHPISETUPWIDGET_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../ui_rthpisetup.h"

//*************************************************************************************************************
//=============================================================================================================
// FIFF INCLUDES
//=============================================================================================================

#include <fiff/fiff_info.h>
#include <fiff/fiff.h>

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE RtHpiPlugin
//=============================================================================================================

namespace RTHPIPLUGIN
{


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class RtHpi;

using namespace FIFFLIB;


//=============================================================================================================
/**
 * DECLARE CLASS RtHpiSetupWidget
 *
 * @brief The RtHpiSetupWidget class provides the RtHpi configuration window.
 */
class RtHpiSetupWidget : public QWidget
{
    Q_OBJECT

public:

    //=========================================================================================================
    /**
     * Constructs a RtHpiSetupWidget which is a child of parent.
     *
     * @param [in] toolbox a pointer to the corresponding RtHpi.
     * @param [in] parent pointer to parent widget; If parent is 0, the new RtHpiSetupWidget becomes a window. If parent is another widget, RtHpiSetupWidget becomes a child window inside parent. RtHpiSetupWidget is deleted when its parent is deleted.
     */
    RtHpiSetupWidget(RtHpi* toolbox, QWidget *parent = 0);

    //=========================================================================================================
    /**
     * Destroys the RtHpiSetupWidget.
     * All RtHpiSetupWidget's children are deleted first. The application exits if RtHpiSetupWidget is the main widget.
     */
    ~RtHpiSetupWidget();

//    void ReadPolhemusDig(QString fileName);

//    bool read_hpi_info(FiffStream *t_pStream, const FiffDirNode& p_Node, FiffInfo& info);
signals:


private slots:
    //=========================================================================================================
    /**
     * Shows the About Dialog
     *
     */
    void showAboutDialog();
//    //=========================================================================================================
//    /**
//    * Load a Polhemus file
//    *
//    */
//    void bnLoadPolhemusFile();

private:

    RtHpi* m_pRtHpi;	/**< Holds a pointer to corresponding RtHpi.*/

    Ui::RtHpiSetupWidgetClass ui;	/**< Holds the user interface for the RtHpiSetupWidget.*/
};

} // NAMESPACE

#endif // RTHPISETUPWIDGET_H
