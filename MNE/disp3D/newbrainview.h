//=============================================================================================================
/**
* @file     newbrainview.h
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
* @brief    NewBrainView class declaration
*
*/

#ifndef NEWBRAINVIEW_H
#define NEWBRAINVIEW_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "disp3D_global.h"

#include <fs/surfaceset.h>
#include <fs/annotationset.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QWidget>
#include <QString>
#include <QTableView>
#include <QSharedPointer>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace DISP3DLIB
{
    class ClustStcModel;
    class ClustStcTableDelegate;
}


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE DISP3DLIB
//=============================================================================================================

namespace DISP3DLIB
{

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FSLIB;
using namespace Eigen;


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


//=============================================================================================================
/**
* ToDo: derive this from geometryview!
* Visualizes FreeSurfer surfaces.
*
* @brief FreeSurfer surface visualisation
*/
class DISP3DSHARED_EXPORT NewBrainView : public QWidget
{
    Q_OBJECT
public:
    typedef QSharedPointer<NewBrainView> SPtr;             /**< Shared pointer type for NewBrainView class. */
    typedef QSharedPointer<const NewBrainView> ConstSPtr;  /**< Const shared pointer type for NewBrainView class. */

    enum ViewOption {
        ShowCurvature = 0x0
    };
    Q_DECLARE_FLAGS(ViewOptions, ViewOption)

    //=========================================================================================================
    /**
    * Default constructor
    */
    NewBrainView(QWidget * parent = 0, Qt::WindowFlags f = 0);

    //=========================================================================================================
    /**
    * Construts the NewBrainView set by reading it of the given surface.
    *
    * @param[in] subject_id         Name of subject
    * @param[in] hemi               Which hemisphere to load {0 -> lh, 1 -> rh, 2 -> both}
    * @param[in] surf               Name of the surface to load (eg. inflated, orig ...)
    * @param[in] subjects_dir       Subjects directory
    */
    explicit NewBrainView(const QString &subject_id, qint32 hemi, const QString &surf, const QString &subjects_dir, QWidget * parent = 0, Qt::WindowFlags f = 0);

    //=========================================================================================================
    /**
    * Construts the NewBrainView set by reading it of the given surface.
    *
    * @param[in] subject_id         Name of subject
    * @param[in] hemi               Which hemisphere to load {0 -> lh, 1 -> rh, 2 -> both}
    * @param[in] surf               Name of the surface to load (eg. inflated, orig ...)
    * @param[in] atlas              Name of the atlas to load (eg. aparc.a2009s, aparc, aparc.DKTatlas40, BA, BA.thresh, ...)
    * @param[in] subjects_dir       Subjects directory
    */
    explicit NewBrainView(const QString &subject_id, qint32 hemi, const QString &surf, const QString &atlas, const QString &subjects_dir, QWidget * parent = 0, Qt::WindowFlags f = 0);

    //=========================================================================================================
    /**
    * Construts the brain view by reading a given surface.
    *
    * @param[in] p_sFile    Surface file name with path
    */
    explicit NewBrainView(const QString& p_sFile, QWidget * parent = 0, Qt::WindowFlags f = 0);

    //=========================================================================================================
    /**
    * Default destructor
    */
    ~NewBrainView();

    void init(const AnnotationSet &annotationSet, const SurfaceSet &surfSet);

    void showDebugTable();

private:
    ViewOptions m_viewOptionFlags;

    SurfaceSet m_SurfaceSet;            /**< Surface set */
    AnnotationSet m_AnnotationSet;      /**< Annotation set */

    bool m_bShowClustModel;
    QSharedPointer<ClustStcModel> m_pClustStcModel;

    QSharedPointer<QWidget> m_pWidgetTable;
    QSharedPointer<ClustStcTableDelegate> m_pClustStcTableDelegate;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(NewBrainView::ViewOptions)

} // NAMESPACE

#endif // NEWBRAINVIEW_H
