//=============================================================================================================
/**
* @file     projectorwidget.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     May, 2014
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
* @brief    Declaration of the ProjectorWidget Class.
*
*/

#ifndef PROJECTORWIDGET_H
#define PROJECTORWIDGET_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fiff/fiff_info.h>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCheckBox>
#include <QWidget>
#include <QGraphicsView>
#include <QGraphicsScene>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE XDISPLIB
//=============================================================================================================

namespace XDISPLIB
{

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace FIFFLIB;


//=============================================================================================================
/**
* DECLARE CLASS ProjectorWidget
*
* @brief The ProjectorWidget class provides the sensor selection widget
*/
class ProjectorWidget : public QWidget
{
    Q_OBJECT
public:

    //=========================================================================================================
    /**
    * Constructs a ProjectorWidget which is a child of parent.
    *
    * @param [in] parent    parent of widget
    * @param [in] f         widget flags
    */
    ProjectorWidget(QWidget *parent = 0, Qt::WindowFlags f = 0);

    //=========================================================================================================
    /**
    * Create the user interface
    */
    void createUI();

    //=========================================================================================================
    /**
    * Set fiff info
    */
    void setFiffInfo(FiffInfo::SPtr& p_pFiffInfo);

signals:
    void projSelectionChanged();

private:
    void checkStatusChanged(int state);

    void enableDisableAll(bool status);

    QList<QCheckBox*>   m_qListCheckBox;            /**< List of CheckBox. */
    FiffInfo::SPtr      m_pFiffInfo;                /**< Connected fiff info. */

    QCheckBox *         m_enableDisableProjectors;  /**< Holds the enable disable all button. */
};

} // NAMESPACE

#ifndef metatype_matrixxd
#define metatype_matrixxd
Q_DECLARE_METATYPE(Eigen::MatrixXd);    /**< Provides QT META type declaration of the Eigen::MatrixXd type. For signal/slot usage.*/
#endif

#endif // PROJECTORWIDGET_H
