//=============================================================================================================
/**
* @file     hpiwidget.h
* @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     March, 2017
*
* @section  LICENSE
*
* Copyright (C) 2017, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
* @brief    HPIWidget class declaration.
*
*/

#ifndef HPIWIDGET_H
#define HPIWIDGET_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "scdisp_global.h"

#include <fiff/fiff_coord_trans.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QWidget>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/SparseCore>


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace Ui {
    class HPIWidget;
}

namespace DISP3DLIB {
    class View3D;
    class Data3DTreeModel;
}

namespace FIFFLIB {
    class FiffDigPointSet;
    class FiffInfo;
    class FiffDigPoint;
}


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE BABYMEGPLUGIN
//=============================================================================================================

namespace SCDISPLIB
{


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


//=============================================================================================================
/**
* The HPIWidget class provides a QDialog for the HPI controls.
*
* @brief The HPIWidget class provides a QDialog for the HPI controls.
*/
class SCDISPSHARED_EXPORT HPIWidget : public QWidget
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
    * Constructs a HPIWidget object.
    *
    * @param[in] pFiffInfo      The FiffInfo.
    * @param[in] parent         The parent widget.
    */
    HPIWidget(QSharedPointer<FIFFLIB::FiffInfo> pFiffInfo, QWidget *parent = 0);
    ~HPIWidget();

    //=========================================================================================================
    /**
    * Set the data needed for fitting.
    *
    * @param[in] data  the data matrix
    */
    void setData(const Eigen::MatrixXd& data);

protected:
     virtual void closeEvent( QCloseEvent * event );

    //=========================================================================================================
    /**
    * Set new digitizer data to View3D.
    *
    * @param[in] digPointSet                    The new digitizer set.
    * @param[in] fittedPointSet                 The new fitted dipoles set.
    * @param[in] vGof                           The goodness of fit in mm for each fitted HPI coil.
    * @param[in] bSortOutAdditionalDigitizer    Whether additional or extra digitized points dhould be sorted out. Too many points could lead to 3D performance issues.
    */
    void setDigitizerDataToView3D(const FIFFLIB::FiffDigPointSet& digPointSet,
                                  const FIFFLIB::FiffDigPointSet& fittedPointSet,
                                  const QVector<double>& vGof,
                                  bool bSortOutAdditionalDigitizer = true);

    //=========================================================================================================
    /**
    * Returns true if any digitizers were loaded that correspond to HPI coils.
    *
    * @return true  If any digitizers were loaded that correspond to HPI coils, false otherwise.
    */
    bool hpiLoaded();

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

    //=========================================================================================================
    /**
    * Perform a HPI fitting procedure.
    *
    * @param[in] vFreqs    the frequencies for each coil.
    */
    void performHPIFitting(const QVector<int>& vFreqs);

    Ui::HPIWidget*                              ui;                 /**< The HPI dialog. */

    QVector<int>                                m_vCoilFreqs;       /**< Vector contains the HPI coil frequencies. */

    Eigen::SparseMatrix<double>                 m_sparseMatCals;    /**< Sparse calibration matrix.*/
    Eigen::MatrixXd                             m_matValue;         /**< The current data block.*/

    QSharedPointer<DISP3DLIB::View3D>           m_pView3D;          /**< The 3D view. */
    QSharedPointer<DISP3DLIB::Data3DTreeModel>  m_pData3DModel;     /**< The Disp3D model. */
    QSharedPointer<FIFFLIB::FiffInfo>           m_pFiffInfo;        /**< The FiffInfo. */

};
} //NAMESPACE
#endif // HPIWIDGET_H
