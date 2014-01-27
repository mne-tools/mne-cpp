//=============================================================================================================
/**
* @file     rawdelegate.h
* @author   Florian Schlembach <florian.schlembach@tu-ilmenau.de>;
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
*           Jens Haueisen <jens.haueisen@tu-ilmenau.de>
* @version  1.0
* @date     January, 2014
*
* @section  LICENSE
*
* Copyright (C) 2012, Christoph Dinh and Matti Hamalainen. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that
* the following conditions are met:
*     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
*       following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
*       the following disclaimer in the documentation and/or other materials provided with the distribution.
*     * Neither the name of the Massachusetts General Hospital nor the names of its contributors may be used
*       to endorse or promote products derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MASSACHUSETTS GENERAL HOSPITAL BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*
* @brief    Implementation of delegate of mne_browse_raw_qt
*
*/

#ifndef RAWDELEGATE_H
#define RAWDELEGATE_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

//Qt
#include <QDebug>
#include <QAbstractItemDelegate>
#include <QPainter>
#include <QPainterPath>

//MNE
#include <fiff/fiff.h>
#include <mne/mne.h>
#include <fiff/fiff_io.h>

//MNE_BROWSE_RAW_QT
#include "types.h"

//Eigen
#include <Eigen/Core>
#include <Eigen/SparseCore>

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNE_BROWSE_RAW_QT;
using namespace Eigen;
using namespace MNELIB;

//=============================================================================================================

class RawDelegate : public QAbstractItemDelegate
{
    Q_OBJECT
public:
    typedef Matrix<double,Dynamic,Dynamic,RowMajor> MatrixXdR;

    RawDelegate(QObject *parent = 0);
    virtual void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    virtual QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;

    // Plots settings
    double m_dPlotHeight; /**< The height of the plot */

    // Scaling
    double m_dMaxValue; /**< Maximum value of the data to plot  */
    double m_dScaleY; /**< Maximum amplitude of plot (max is m_dPlotHeight/2) */
    double m_dDx; /**< pixel difference to the next sample*/

signals:

public slots:

private:
    /**
     * @brief createPlotPath Creates the QPointer path for the data plot.
     *
     * @param[in] index QModelIndex for accessing associated data and model object.
     * @param[in,out] path The QPointerPath to create for the data plot.
     */
    void createPlotPath(const QModelIndex &index, QPainterPath& path, QList<RowVectorPair>& listPairs) const;
    /**
     * @brief createGridPath Creates the QPointer path for the grid plot.
     *
     * @param[in,out] path The row vector of the data matrix <1 x nsamples>.
     * @param[in] data The row vector of the data matrix <1 x nsamples>.
     */
    void createGridPath(QPainterPath& path, QList<RowVectorPair>& listPairs) const;

    //Look
    qint8 m_nhlines; /**< Number of horizontal lines for the grid plot */

};

#endif // RAWDELEGATE_H
