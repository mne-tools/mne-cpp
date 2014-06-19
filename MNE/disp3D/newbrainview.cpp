//=============================================================================================================
/**
* @file     newbrainview.cpp
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
* @brief    Implementation of the NewBrainView class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "newbrainview.h"
#include "helpers/cluststcmodel.h"
#include "helpers/cluststctabledelegate.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QMouseEvent>
#include <QTableView>
#include <QGridLayout>
#include <QLabel>
#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISP3DLIB;
//using namespace DISPLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

NewBrainView::NewBrainView(QWidget * parent, Qt::WindowFlags f)
: QWidget(parent, f)
, m_bShowClustModel(false)
{
//    init();
}


//*************************************************************************************************************

NewBrainView::NewBrainView(const QString &subject_id, qint32 hemi, const QString &surf, const QString &subjects_dir, QWidget * parent, Qt::WindowFlags f)
: QWidget(parent, f)
, m_SurfaceSet(subject_id, hemi, surf, subjects_dir)
, m_bShowClustModel(false)
{
//    init();
}


//*************************************************************************************************************

NewBrainView::NewBrainView(const QString &subject_id, qint32 hemi, const QString &surf, const QString &atlas, const QString &subjects_dir, QWidget * parent, Qt::WindowFlags f)
: QWidget(parent, f)
, m_SurfaceSet(subject_id, hemi, surf, subjects_dir)
, m_AnnotationSet(subject_id, hemi, atlas, subjects_dir)
, m_bShowClustModel(false)
{
    init(m_AnnotationSet, m_SurfaceSet);
}


//*************************************************************************************************************

NewBrainView::NewBrainView(const QString& p_sFile, QWidget * parent, Qt::WindowFlags f)
: QWidget(parent, f)
, m_bShowClustModel(false)
{
    Surface t_Surf(p_sFile);
    m_SurfaceSet.insert(t_Surf);

//    init();
}


//*************************************************************************************************************

NewBrainView::~NewBrainView()
{
}


//*************************************************************************************************************

void NewBrainView::init(const AnnotationSet &annotationSet, const SurfaceSet &surfSet)
{
    m_pClustStcModel = ClustStcModel::SPtr(new ClustStcModel);
    m_pClustStcModel->init(annotationSet, surfSet);
    m_pClustStcModel->setLoop(true);

    m_bShowClustModel = true;
}


//*************************************************************************************************************

void NewBrainView::showDebugTable()
{
    if(m_bShowClustModel)
    {
        //
        // QDebugTable
        //
        if(!m_pWidgetTable)
        {
            m_pWidgetTable = QSharedPointer<QWidget>(new QWidget);
            QGridLayout *mainLayoutTable = new QGridLayout;

            QTableView* pTableView = new QTableView;
            if(m_pClustStcTableDelegate.isNull())
                m_pClustStcTableDelegate = QSharedPointer<ClustStcTableDelegate>(new ClustStcTableDelegate);
            pTableView->setModel(m_pClustStcModel.data());
            pTableView->setItemDelegate(m_pClustStcTableDelegate.data());
            pTableView->setColumnHidden(0,true); //because content is plotted jointly with column=1

            QLabel * pLabelNorm = new QLabel("Norm");
            QSlider* pSliderNorm = new QSlider(Qt::Vertical);
            QObject::connect(pSliderNorm, &QSlider::valueChanged, m_pClustStcModel.data(), &ClustStcModel::setNormalization);
            pSliderNorm->setMinimum(1);
            pSliderNorm->setMaximum(100);
            pSliderNorm->setValue(60);

            QLabel * pLabelAverage = new QLabel("Average");
            QSlider* pSliderAverage = new QSlider(Qt::Horizontal);
            QObject::connect(pSliderAverage, &QSlider::valueChanged, m_pClustStcModel.data(), &ClustStcModel::setAverage);
            pSliderAverage->setMinimum(1);
            pSliderAverage->setMaximum(500);
            pSliderAverage->setValue(100);

            mainLayoutTable->addWidget(pTableView,0,0,2,2);
            mainLayoutTable->addWidget(pLabelNorm,0,3);
            mainLayoutTable->addWidget(pSliderNorm,1,3);
            mainLayoutTable->addWidget(pLabelAverage,3,0);
            mainLayoutTable->addWidget(pSliderAverage,3,1);

            m_pWidgetTable->setLayout(mainLayoutTable);
            m_pWidgetTable->setWindowTitle("Stc Table View");
        }
        m_pWidgetTable->show();
        m_pWidgetTable->resize(800,600);
    }
}
