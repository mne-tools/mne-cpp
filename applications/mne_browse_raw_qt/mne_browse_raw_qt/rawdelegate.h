//=============================================================================================================
/**
* @file     rawdelegate.h
* @author   Florian Schlembach <florian.schlembach@tu-ilmenau.de>;
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
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
#include <QAbstractItemDelegate>
#include <QPainter>
#include <QPainterPath>

//MNE
#include <fiff/fiff.h>
#include <mne/mne.h>
#include <fiff/fiff_io.h>

//Eigen
#include <Eigen/Core>
#include <Eigen/SparseCore>

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace MNELIB;

//=============================================================================================================

class RawDelegate : public QAbstractItemDelegate
{
    Q_OBJECT
public:
    RawDelegate(QObject *parent = 0);
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;

    // Plots settings
    double m_dPlotHeight; /**< The height of the plot */

signals:

public slots:

private:
    void createPlotPath(QPainterPath& path, MatrixXd data) const;
    void createGridPath(QPainterPath& path);


    // Scaling
    double m_dMaxValue; /**< Maximum value of the data to plot  */
    double m_dScaleY; /**< Maximum amplitude of plot (max is m_dPlotHeight/2) */
    double m_dDx; /**< pixel difference to the next sample*/

    //Look
    qint8 m_nhlines;

};

#endif // RAWDELEGATE_H
