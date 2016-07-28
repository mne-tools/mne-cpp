//=============================================================================================================
/**
* @file     babymeghpidlg.h
* @author   Limin Sun <liminsun@nmr.mgh.harvard.edu>
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     March, 2015
*
* @section  LICENSE
*
* Copyright (C) 2015, Limin Sun and Matti Hamalainen. All rights reserved.
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
* @brief    BabyMEGHPI class declaration.
*
*/

#ifndef BABYMEGHPIDGL_H
#define BABYMEGHPIDGL_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "babymeg_global.h"
#include "../babymeg.h"

#include <fiff/fiff_info.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDialog>
#include <QAbstractButton>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace Ui {
    class BabyMEGHPIDgl;
}


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE BABYMEGPLUGIN
//=============================================================================================================

namespace BABYMEGPLUGIN
{


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class BabyMEG;


//=============================================================================================================
/**
* DECLARE CLASS BabyMEGHPIDgl
*
* @brief The BabyMEGHPIDgl class provides a QDialog for the HPI controls.
*/
class BABYMEGSHARED_EXPORT BabyMEGHPIDgl : public QDialog
{
    Q_OBJECT

public:
    explicit BabyMEGHPIDgl(BabyMEG* p_pBabyMEG,QWidget *parent = 0);
    ~BabyMEGHPIDgl();

    //=========================================================================================================
    /**
    * Read Polhemus data from fif file
    *
    */
    void ReadPolhemusDig(QString fileName);

    //=========================================================================================================
    /**
    * Load a Polhemus file name
    *
    */
    void bnLoadPolhemusFile();
    void OKProc(QAbstractButton *b);
    void CancelProc();

    BabyMEG*                m_pBabyMEG;
    FIFFLIB::FiffInfo       info;
    QString                 FileName_HPI;

protected:
     virtual void closeEvent( QCloseEvent * event );

private:
    Ui::BabyMEGHPIDgl*      ui;

signals:
    void SendHPIFiffInfo(FIFFLIB::FiffInfo);
};
} //NAMESPACE
#endif // BABYMEGHPIDGL_H
