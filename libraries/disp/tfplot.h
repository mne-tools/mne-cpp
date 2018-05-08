//=============================================================================================================
/**
* @file     tfplot.cpp
* @author   Martin Henfling <martin.henfling@tu-ilmenau.de>;
*           Daniel Knobl <daniel.knobl@tu-ilmenau.de>;
* @version  1.0
* @date     September, 2015
*
* @section  LICENSE
*
* Copyright (C) 2015, Martin Henfling and Daniel Knobl. All rights reserved.
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
* @brief    Declaration of time-frequency plot class.
*/

#ifndef TFPLOT_H
#define TFPLOT_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "disp_global.h"


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QImage>
#include <QGridLayout>
#include <QGraphicsView>
#include <QGraphicsPixmapItem>
#include <QGraphicsSceneResizeEvent>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/SparseCore>
#include <unsupported/Eigen/FFT>


namespace DISPLIB
{


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;

enum ColorMaps
{
    Hot,
    HotNeg1,
    HotNeg2,
    Jet,
    Bone,
    RedBlue
};

class DISPSHARED_EXPORT TFplot : public QWidget
{

public:

    //=========================================================================================================
    /**
    * TFplot_TFplot
    *
    * ### display tf-plot function ###
    *
    * Constructor
    *
    * constructs TFplot class
    *
    *  @param[in] tf_matrix         given spectrogram
    *  @param[in] sample_rate       given sample rate of signal related to th spectrogram
    *  @param[in] lower_frq         lower bound frequency, that should be plotted
    *  @param[in] upper_frq         upper bound frequency, that should be plotted
    *  @param[in] cmap              colormap used to plot the spectrogram
    *
    */
    TFplot(MatrixXd tf_matrix, qreal sample_rate, qreal lower_frq, qreal upper_frq, ColorMaps cmap);

    //=========================================================================================================
    /**
    * TFplot_TFplot
    *
    * ### display tf-plot function ###
    *
    * Constructor
    *
    * constructs TFplot class
    *
    *  @param[in] tf_matrix         given spectrogram
    *  @param[in] sample_rate       given sample rate of signal related to th spectrogram
    *  @param[in] cmap              colormap used to plot the spectrogram
    *
    */
    TFplot(MatrixXd tf_matrix, qreal sample_rate, ColorMaps cmap);

private:
    //=========================================================================================================
    /**
    * TFplot_calc_plot
    *
    * ### display tf-plot function ###
    *
    * calculates a image to plot the tf_matrix
    *
    *  @param[in] tf_matrix         given spectrogram
    *  @param[in] sample_rate       given sample rate of signal related to th spectrogram
    *  @param[in] cmap              colormap used to plot the spectrogram
    *  @param[in] lower_frq         lower bound frequency, that should be plotted
    *  @param[in] upper_frq         upper bound frequency, that should be plotted
    *
    */
    void calc_plot(MatrixXd tf_matrix, qreal sample_rate, ColorMaps cmap, qreal lower_frq, qreal upper_frq);

protected:
     virtual void resizeEvent(QResizeEvent *event);
};

}

#endif // TFPLOT_H
