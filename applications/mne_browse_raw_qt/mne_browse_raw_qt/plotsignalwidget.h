//=============================================================================================================
/**
* @file     plotsignal.h
* @author   Florian Schlembach <florian.schlembach@tu-ilmenau.de>
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     January, 2014
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
* @brief    Implements the PlotSignalWidget of mne_browse_raw_qt
*
*/

#ifndef PLOTSIGNAL_H
#define PLOTSIGNAL_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

//Qt
#include <QWidget>

//Eigen
#include <Eigen/Core>
#include <Eigen/SparseCore>

//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE
//=============================================================================================================

using namespace Eigen;

//=============================================================================================================

#include <QWidget>

class PlotSignalWidget : public QWidget
{
    Q_OBJECT
public:
    PlotSignalWidget(QWidget *parent = 0);
    PlotSignalWidget(MatrixXd data, MatrixXd times, QWidget *parent = 0);

protected:
    void paintEvent(QPaintEvent *event);

signals:

public slots:

private:
    void createPath();

    QVector<double> m_data,m_times;
    QPainterPath m_qPainterPath; /**< The current painter path which is the real-time curve. */


    double m_dPosition; /**< The start position which is the x position of the frame. */
    double m_dPosX; /**< The x position of the frame. */
    double m_dPosY; /**< The middle y position of the frame. */
    float m_fScaleFactor; /**< Current scaling factor -> renewed over actualize. */

};

#endif // PLOTSIGNAL_H
