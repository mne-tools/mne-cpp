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
* @brief    ImageSc class declaration
*
*/

#ifndef IMAGESC_H
#define IMAGESC_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "disp_global.h"


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
* Visualizes Eigen matrizes labels using a HSV colormap
*
* @brief Eigen matrix visualization
*/
class DISPSHARED_EXPORT ImageSc : public QWidget
{
    Q_OBJECT
public:
    typedef QSharedPointer<ImageSc> SPtr;            /**< Shared pointer type for MatrixView class. */
    typedef QSharedPointer<const ImageSc> ConstSPtr; /**< Const shared pointer type for MatrixView class. */

    explicit ImageSc(QWidget *parent = 0);
    explicit ImageSc(MatrixXd &p_dMat, QWidget *parent = 0);
    explicit ImageSc(MatrixXf &p_fMat, QWidget *parent = 0);
    explicit ImageSc(MatrixXi &p_iMat, QWidget *parent = 0);

    ~ImageSc();

    void init();

    void updateMatrix(MatrixXd &p_dMat);
    void updateMatrix(MatrixXf &p_fMat);
    void updateMatrix(MatrixXi &p_iMat);

    void setTitle(const QString &p_sTitle);
    void setXLabel(const QString &p_sXLabel);
    void setYLabel(const QString &p_sYLabel);


protected:
    void paintEvent(QPaintEvent*);
    void resizeEvent(QResizeEvent*);
    
private:

    QPixmap* m_pPixmapData;
    QPixmap* m_pPixmapColorbar;

    QSize widgetSize;

    qint32 m_iBorderTopBottom;
    qint32 m_iBorderLeftRight;

    double m_dMinValue;
    double m_dMaxValue;

    QString m_sTitle;
    QFont m_qFontTitle;
    QPen m_qPenTitle;

    QString m_sXLabel;
    QString m_sYLabel;
    QFont m_qFontAxes;
    QPen m_qPenAxes;

    bool m_bColorbar;
    qint32 m_iColorbarWidth;
    qint32 m_iColorbarSteps;
    QFont m_qFontColorbar;
    QPen m_qPenColorbar;

};

//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

} // NAMESPACE

#endif // IMAGESC_H
