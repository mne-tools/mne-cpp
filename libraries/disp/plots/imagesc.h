//=============================================================================================================
/**
* @file     imagesc.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     June, 2013
*
* @section  LICENSE
*
* Copyright (C) 2013, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    ImageSc class declaration
*
*/

#ifndef IMAGESC_H
#define IMAGESC_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp_global.h"
#include "graph.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QWidget>
#include <QImage>
#include <QString>
#include <QPen>
#include <QSharedPointer>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE DISP3DLIB
//=============================================================================================================

namespace DISPLIB
{

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


//=============================================================================================================
/**
* Visualizes Eigen matrizes, similiar to MATLABs imagesc function; Available colormaps are: Jet, Hot, Bone
*
* @brief Eigen matrix visualization
*/
class DISPSHARED_EXPORT ImageSc : public Graph
{
    Q_OBJECT
public:
    typedef QSharedPointer<ImageSc> SPtr;            /**< Shared pointer type for MatrixView class. */
    typedef QSharedPointer<const ImageSc> ConstSPtr; /**< Const shared pointer type for MatrixView class. */

    //=========================================================================================================
    /**
    * Creates the scaled image view.
    *
    * @param[in] parent     Parent QObject (optional)
    */
    explicit ImageSc(QWidget *parent = 0);

    //=========================================================================================================
    /**
    * Creates the scaled image view with a given double matrix.
    *
    * @param[in] p_dMat     The double data matrix
    * @param[in] parent     Parent QObject (optional)
    */
    explicit ImageSc(MatrixXd &p_dMat, QWidget *parent = 0);

    //=========================================================================================================
    /**
    * Creates the scaled image view with a given float matrix.
    *
    * @param[in] p_fMat     The float data matrix
    * @param[in] parent     Parent QObject (optional)
    */
    explicit ImageSc(MatrixXf &p_fMat, QWidget *parent = 0);

    //=========================================================================================================
    /**
    * Creates the scaled image view with a given integer matrix.
    *
    * @param[in] p_iMat     The integer data matrix
    * @param[in] parent     Parent QObject (optional)
    */
    explicit ImageSc(MatrixXi &p_iMat, QWidget *parent = 0);

    //=========================================================================================================
    /**
    * Destructs the ImageSc object
    */
    ~ImageSc();

    //=========================================================================================================
    /**
    * Initializes the ImageSc object
    */
    void init();

    //=========================================================================================================
    /**
    * Updates the scaled image view with a given double matrix.
    *
    * @param[in] p_dMat     The double data matrix
    */
    void updateData(MatrixXd &p_dMat);
    //=========================================================================================================
    /**
    * Updates the scaled image view with a given float matrix.
    *
    * @param[in] p_fMat     The float data matrix
    */
    void updateData(MatrixXf &p_fMat);
    //=========================================================================================================
    /**
    * Updates the scaled image view with a given integer matrix.
    *
    * @param[in] p_dMat     The integer data matrix
    */
    void updateData(MatrixXi &p_iMat);

    //=========================================================================================================
    /**
    * Sets the color map to use, e.g. "Jet", "Hot", "Bone"
    *
    * @param[in] p_sColorMap    The colormap to use
    */
    void setColorMap(const QString &p_sColorMap);

protected:
    //=========================================================================================================
    /**
    * Updates data and colorbar pixmap
    */
    void updateMaps();

    void paintEvent(QPaintEvent*);

    QPixmap* m_pPixmapData;         /**< data pixmap */
    QPixmap* m_pPixmapColorbar;     /**< colorbar pixmap */

    MatrixXd m_matCentNormData;     /**< centralized and normalized data */

    double m_dMinValue;             /**< Minimal data value */
    double m_dMaxValue;             /**< Maximal data value */

    QRgb (*pColorMapper)(double);   /**< Function pointer to current colormap */

    bool m_bColorbar;                   /**< If colorbar is visible */
    QVector<double> m_qVecScaleValues;  /**< Scale values */
    qint32 m_iColorbarWidth;            /**< Colorbar width */
    qint32 m_iColorbarSteps;            /**< Number of colorbar vaues to display */
    qint32 m_iColorbarGradSteps;        /**< Gradient steps of the colorbar */
    QFont m_qFontColorbar;              /**< Colorbar font */
    QPen m_qPenColorbar;                /**< Colorbar pen */
};

//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

} // NAMESPACE

#endif // IMAGESC_H
