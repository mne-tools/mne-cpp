//=============================================================================================================
/**
* @file     main.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     July, 2013
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
* @brief    Example of the computation of a rawClusteredInverse
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fs/label.h>
#include <fs/surface.h>
#include <fs/annotationset.h>

#include <fiff/fiff_evoked.h>
#include <fiff/fiff.h>
#include <mne/mne.h>

#include <mne/mne_epoch_data_list.h>

#include <mne/mne_sourceestimate.h>
#include <inverse/minimumNorm/minimumnorm.h>


#include <disp3D/helpers/stcview.h>
#include <disp3D/helpers/stcmodel.h>
#include <disp3D/helpers/stctabledelegate.h>

#include <utils/mnemath.h>

#include <iostream>

#include <fstream>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QApplication>
#include <QSet>
#include <QElapsedTimer>
#include <QTableView>
#include <QGridLayout>
#include <QLabel>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNELIB;
using namespace FSLIB;
using namespace FIFFLIB;
using namespace INVERSELIB;
//using namespace DISP3DLIB;
using namespace UTILSLIB;


//*************************************************************************************************************
//=============================================================================================================
// MAIN
//=============================================================================================================

//=============================================================================================================
/**
* The function main marks the entry point of the program.
* By default, main has the storage class extern.
*
* @param [in] argc (argument count) is an integer that indicates how many arguments were entered on the command line when the program was started.
* @param [in] argv (argument vector) is an array of pointers to arrays of character objects. The array objects are null-terminated strings, representing the arguments that were entered on the command line when the program was started.
* @return the value that was set to exit() (which is 0 if exit() is called via quit()).
*/
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    AnnotationSet t_annotationSet("mind006", 2, "aparc.a2009s", "D:/Data/subjects");
    SurfaceSet t_surfSet("mind006", 2, "inflated", "D:/Data/subjects");

    QString t_sFileNameStc("mind006_051209_auditory01_test.stc");
    MNESourceEstimate sourceEstimateClustered;

    if(!t_sFileNameStc.isEmpty())
    {
        QFile t_fileClusteredStc(t_sFileNameStc);
        MNESourceEstimate::read(t_fileClusteredStc, sourceEstimateClustered);
    }

    qDebug() << "sourceEstimateClustered" << sourceEstimateClustered.data.rows() << "x" << sourceEstimateClustered.data.cols();

    //
    // QDebugTable
    //
    QWidget* pWidgetTable = new QWidget;
    QGridLayout *mainLayoutTable = new QGridLayout;

    QTableView* pTableView = new QTableView;
    StcModel* pStcModel = new StcModel;
    pStcModel->init(t_annotationSet, t_surfSet);
    pStcModel->setLoop(true);
    StcTableDelegate* pStcTableDelegate = new StcTableDelegate;
    pTableView->setModel(pStcModel);
    pTableView->setItemDelegate(pStcTableDelegate);
    pTableView->setColumnHidden(0,true); //because content is plotted jointly with column=1

    QLabel * pLabelNorm = new QLabel("Norm");
    QSlider* pSliderNorm = new QSlider(Qt::Vertical);
    QObject::connect(pSliderNorm, &QSlider::valueChanged, pStcModel, &StcModel::setNormalization);
    pSliderNorm->setMinimum(1);
    pSliderNorm->setMaximum(100);
    pSliderNorm->setValue(50);

    QLabel * pLabelAverage = new QLabel("Average");
    QSlider* pSliderAverage = new QSlider(Qt::Horizontal);
    QObject::connect(pSliderAverage, &QSlider::valueChanged, pStcModel, &StcModel::setAverage);
    pSliderAverage->setMinimum(1);
    pSliderAverage->setMaximum(500);
    pSliderAverage->setValue(500);

    mainLayoutTable->addWidget(pTableView,0,0,2,2);
    mainLayoutTable->addWidget(pLabelNorm,0,3);
    mainLayoutTable->addWidget(pSliderNorm,1,3);
    mainLayoutTable->addWidget(pLabelAverage,3,0);
    mainLayoutTable->addWidget(pSliderAverage,3,1);

    pWidgetTable->setLayout(mainLayoutTable);
    pWidgetTable->show();
    pWidgetTable->resize(800,600);

    //
    // STC view
    //
    QWidget* pWidgetView = new QWidget;
    QGridLayout *mainLayoutView = new QGridLayout;

    QLabel * pLabelNormView = new QLabel("Norm");
    QSlider* pSliderNormView = new QSlider(Qt::Vertical);
    QObject::connect(pSliderNormView, &QSlider::valueChanged, pStcModel, &StcModel::setNormalization);
    pSliderNormView->setMinimum(1);
    pSliderNormView->setMaximum(100);
    pSliderNormView->setValue(50);

    QLabel * pLabelAverageView = new QLabel("Average");
    QSlider* pSliderAverageView = new QSlider(Qt::Horizontal);
    QObject::connect(pSliderAverageView, &QSlider::valueChanged, pStcModel, &StcModel::setAverage);
    pSliderAverageView->setMinimum(1);
    pSliderAverageView->setMaximum(500);
    pSliderAverageView->setValue(100);

    StcView* view = new StcView;
    view->setModel(pStcModel);

    if (view->stereoType() != QGLView::RedCyanAnaglyph)
        view->camera()->setEyeSeparation(0.3f);

    QWidget *pWidgetContainer = QWidget::createWindowContainer(view);

    mainLayoutView->addWidget(pWidgetContainer,0,0,2,2);
    mainLayoutView->addWidget(pLabelNormView,0,3);
    mainLayoutView->addWidget(pSliderNormView,1,3);
    mainLayoutView->addWidget(pLabelAverageView,3,0);
    mainLayoutView->addWidget(pSliderAverageView,3,1);

    pWidgetView->setLayout(mainLayoutView);

    //connect the sliders

    QObject::connect(pSliderNorm, &QSlider::valueChanged, pSliderNormView, &QSlider::setValue);
    QObject::connect(pSliderAverage, &QSlider::valueChanged, pSliderAverageView, &QSlider::setValue);

    QObject::connect(pSliderNormView, &QSlider::valueChanged, pSliderNorm, &QSlider::setValue);
    QObject::connect(pSliderAverageView, &QSlider::valueChanged, pSliderAverage, &QSlider::setValue);

    pWidgetView->show();
    pWidgetView->resize(800,600);

    //
    // Add Data
    //
    qDebug() << "sourceEstimateClustered" << sourceEstimateClustered.data.cols();
    pStcModel->addData(sourceEstimateClustered);

    return a.exec();//1;//a.exec();
}
