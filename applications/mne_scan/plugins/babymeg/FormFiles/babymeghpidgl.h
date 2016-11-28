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

namespace DISP3DLIB {
    class View3D;
}

namespace FIFFLIB {
    class FiffDigPointSet;
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
* The BabyMEGHPIDgl class provides a QDialog for the HPI controls.
*
* @brief The BabyMEGHPIDgl class provides a QDialog for the HPI controls.
*/
class BABYMEGSHARED_EXPORT BabyMEGHPIDgl : public QWidget
{
    Q_OBJECT

public:
    explicit BabyMEGHPIDgl(BabyMEG* p_pBabyMEG, QWidget *parent = 0);
    ~BabyMEGHPIDgl();

    //=========================================================================================================
    /**
    * Set new digitizer data to View3D.
    *
    * @param[in] digPointSet                    The new digitizer set.
    * @param[in] vGof                           The goodness of fit in mm for each fitted HPI coil.
    * @param[in] bSortOutAdditionalDigitizer    Whether additional or extra digitized points dhould be sorted out. Too many points could lead to 3D performance issues.
    */
    void setDigitizerDataToView3D(const FIFFLIB::FiffDigPointSet& digPointSet, const QVector<double>& vGof, bool bSortOutAdditionalDigitizer = true);

    //=========================================================================================================
    /**
    * Returns true if any digitizers were loaded that correspond to HPI coils.
    *
    * @return true  If any digitizers were loaded that correspond to HPI coils, false otherwise.
    */
    bool hpiLoaded();

    BabyMEG*                m_pBabyMEG;

protected:
     virtual void closeEvent( QCloseEvent * event );

private:

    //=========================================================================================================
    /**
    * Read Polhemus data from fif file.
    *
    */
    QList<FIFFLIB::FiffDigPoint> readPolhemusDig(QString fileName);

    //=========================================================================================================
    /**
    * Perform a single HPI fit.
    *
    */
    void onBtnDoSingleFit();

    //=========================================================================================================
    /**
    * Load a Polhemus file name.
    *
    */
    void onBtnLoadPolhemusFile();

    //=========================================================================================================
    /**
    * Load a Polhemus file name.
    *
    */
    void onFreqsChanged();

    Ui::BabyMEGHPIDgl*                  ui;

    QVector<int>                        m_vCoilFreqs;

    QSharedPointer<DISP3DLIB::View3D>   m_pView3D;

signals:
    void SendHPIFiffInfo(FIFFLIB::FiffInfo);
};
} //NAMESPACE
#endif // BABYMEGHPIDGL_H
