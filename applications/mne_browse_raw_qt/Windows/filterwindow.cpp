//=============================================================================================================
/**
* @file     filterwindow.cpp
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     August, 2014
*
* @section  LICENSE
*
* Copyright (C) 2014, Lorenz Esch, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Contains the implementation of the FilterWindow class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "filterwindow.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEBrowseRawQt;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FilterWindow::FilterWindow(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::FilterWindowDockWidget),
    m_pFilterPlotScene(new FilterPlotScene),
    m_pMainWindow(static_cast<MainWindow*>(parent))
{
    ui->setupUi(this);

    initSpinBoxes();
    initButtons();
    initComboBoxes();
    initFilterPlot();

    m_iWindowSize = MODEL_WINDOW_SIZE;
    m_iFilterTaps = MODEL_NUM_FILTER_TAPS;
}


//*************************************************************************************************************

FilterWindow::~FilterWindow()
{
    delete ui;
}


//*************************************************************************************************************

void FilterWindow::init()
{
    initSpinBoxes();
    initButtons();
    initComboBoxes();
    initFilterPlot();
    initTableViews();
}


//*************************************************************************************************************

void FilterWindow::initSpinBoxes()
{
    connect(ui->m_doubleSpinBox_lowpass,static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
                this,&FilterWindow::filterParametersChanged);

    connect(ui->m_doubleSpinBox_highpass,static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
                this,&FilterWindow::filterParametersChanged);

    connect(ui->m_doubleSpinBox_transitionband,static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
                this,&FilterWindow::filterParametersChanged);

    connect(ui->m_spinBox_filterTaps,static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
                this,&FilterWindow::filterParametersChanged);

    //Intercept events from the spin boxes to get control over key events
    ui->m_doubleSpinBox_lowpass->installEventFilter(this);
    ui->m_doubleSpinBox_highpass->installEventFilter(this);
    ui->m_doubleSpinBox_transitionband->installEventFilter(this);
}


//*************************************************************************************************************

void FilterWindow::initButtons()
{
    connect(ui->m_pushButton_applyFilter,&QPushButton::released,
                this,&FilterWindow::applyFilter);

    connect(ui->m_pushButton_undoFiltering,&QPushButton::released,
                this,&FilterWindow::undoFilter);

    connect(ui->m_pushButton_exportPlot,&QPushButton::released,
                this,&FilterWindow::exportFilterPlot);

    connect(ui->m_pushButton_exportFilter,&QPushButton::released,
                this,&FilterWindow::exportFilterCoefficients);
}


//*************************************************************************************************************

void FilterWindow::initComboBoxes()
{
    connect(ui->m_comboBox_filterType,static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
                this,&FilterWindow::changeStateSpinBoxes);

    //Initial selection is a lowpass
    ui->m_doubleSpinBox_lowpass->setVisible(true);
    ui->m_label_lowpass->setVisible(true);
    ui->m_label_lowpass->setText("Cut-Off (Hz):");

    ui->m_doubleSpinBox_highpass->setVisible(false);
    ui->m_label_highpass->setVisible(false);
    ui->m_doubleSpinBox_highpass->setEnabled(false);

    //If add filter to channel type combo box changes -> also change combo box for undo filtering
    connect(ui->m_comboBox_filterApplyTo, &QComboBox::currentTextChanged,
            ui->m_comboBox_filterUndoTo, &QComboBox::setCurrentText);
}


//*************************************************************************************************************

void FilterWindow::initFilterPlot()
{
    ui->m_graphicsView_filterPlot->setScene(m_pFilterPlotScene);
}


//*************************************************************************************************************

void FilterWindow::initTableViews()
{
    ui->m_tableView_activeFilters->setModel(m_pMainWindow->m_pChInfoWindow->getDataModel());

    //Hide columns
    ui->m_tableView_activeFilters->hideColumn(0);
    ui->m_tableView_activeFilters->hideColumn(2);
    ui->m_tableView_activeFilters->hideColumn(3);
    ui->m_tableView_activeFilters->hideColumn(4);
    ui->m_tableView_activeFilters->hideColumn(5);
    ui->m_tableView_activeFilters->hideColumn(6);
    ui->m_tableView_activeFilters->hideColumn(7);
    ui->m_tableView_activeFilters->hideColumn(8);

    ui->m_tableView_activeFilters->verticalHeader()->hide();

    ui->m_tableView_activeFilters->resizeColumnsToContents();
    ui->m_groupBox_activeFilters->adjustSize();
    ui->m_groupBox_activeFilters->adjustSize();
}


//*************************************************************************************************************

void FilterWindow::updateFilterPlot()
{
    //Update the filter of the scene
    QMutableMapIterator<QString,QSharedPointer<MNEOperator> > it(m_pMainWindow->m_pDataWindow->getDataModel()->m_Operators);
    while(it.hasNext()) {
        it.next();
        if(it.key() == "User defined (See 'Adjust/Filter')") {
            m_pFilterPlotScene->updateFilter(it.value(),
                                             m_pMainWindow->m_pDataWindow->getDataModel()->m_fiffInfo.sfreq,
                                             ui->m_doubleSpinBox_lowpass->value(),
                                             ui->m_doubleSpinBox_highpass->value());
        }
    }

    ui->m_graphicsView_filterPlot->fitInView(m_pFilterPlotScene->itemsBoundingRect(), Qt::KeepAspectRatio);
}


//*************************************************************************************************************

void FilterWindow::resizeEvent(QResizeEvent* event)
{
    Q_UNUSED(event);
    ui->m_graphicsView_filterPlot->fitInView(m_pFilterPlotScene->itemsBoundingRect(), Qt::KeepAspectRatio);
}


//*************************************************************************************************************

void FilterWindow::keyPressEvent(QKeyEvent * event)
{
    qDebug()<<"Key pressed"<<event->key();
    if(event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return)
        applyFilter();

    if((event->modifiers() == Qt::ControlModifier && event->key() == Qt::Key_Z) || event->key() == Qt::Key_Delete)
        undoFilter();
}


//*************************************************************************************************************

bool FilterWindow::eventFilter(QObject *obj, QEvent *event)
{
    if(obj == ui->m_doubleSpinBox_highpass || obj == ui->m_doubleSpinBox_lowpass || obj == ui->m_doubleSpinBox_transitionband) {
        if (event->type() == QEvent::KeyPress) {
            QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
            qDebug("Ate key press %d", keyEvent->key());

            if((keyEvent->modifiers() == Qt::ControlModifier && keyEvent->key() == Qt::Key_Z) || keyEvent->key() == Qt::Key_Delete)
                undoFilter();
            else // standard event processing
                return QObject::eventFilter(obj, event);

            return true;
        } else {
            // standard event processing
            return QObject::eventFilter(obj, event);
        }
    }

    return true;
}


//*************************************************************************************************************

void FilterWindow::changeStateSpinBoxes(int currentIndex)
{
    //Change visibility of sin boxes depending on filter type
    switch(currentIndex) {
        case 0: //Lowpass
            ui->m_doubleSpinBox_lowpass->setVisible(true);
            ui->m_label_lowpass->setVisible(true);
            ui->m_label_lowpass->setText("Cut-Off (Hz):");

            ui->m_doubleSpinBox_highpass->setVisible(false);
            ui->m_label_highpass->setVisible(false);
            ui->m_doubleSpinBox_highpass->setEnabled(false);
            break;

        case 1: //Highpass
            ui->m_doubleSpinBox_highpass->setVisible(true);
            ui->m_label_highpass->setVisible(true);
            ui->m_label_highpass->setText("Cut-Off (Hz):");

            ui->m_doubleSpinBox_lowpass->setVisible(false);
            ui->m_label_lowpass->setVisible(false);
            ui->m_doubleSpinBox_highpass->setEnabled(true);
            break;

        case 2: //Bandpass
            ui->m_doubleSpinBox_highpass->setVisible(true);
            ui->m_label_highpass->setVisible(true);
            ui->m_doubleSpinBox_lowpass->setVisible(true);
            ui->m_label_lowpass->setText("Cut-Off Low (Hz):");

            ui->m_label_lowpass->setVisible(true);
            ui->m_doubleSpinBox_lowpass->setEnabled(true);
            ui->m_doubleSpinBox_highpass->setEnabled(true);
            ui->m_label_highpass->setText("Cut-Off High (Hz):");
            break;
    }

    filterParametersChanged();
}


//*************************************************************************************************************

void FilterWindow::filterParametersChanged()
{
    //User defined filter parameters
    double lowpassHz = ui->m_doubleSpinBox_lowpass->value();
    double highpassHz = ui->m_doubleSpinBox_highpass->value();
    double center = (lowpassHz+highpassHz)/2;
    double trans_width = ui->m_doubleSpinBox_transitionband->value();
    double bw = highpassHz-lowpassHz;
    double samplingFrequency = m_pMainWindow->m_pDataWindow->getDataModel()->m_fiffInfo.sfreq;

    int filterTaps = ui->m_spinBox_filterTaps->value();

    //Update min max of spin boxes to nyquist
    ui->m_doubleSpinBox_highpass->setMaximum(samplingFrequency/2);
    ui->m_doubleSpinBox_lowpass->setMaximum(samplingFrequency/2);

    QSharedPointer<MNEOperator> userDefinedFilterOperator;

    if(ui->m_comboBox_filterType->currentText() == "Lowpass") {
        userDefinedFilterOperator = QSharedPointer<MNEOperator>(
                   new FilterOperator("User defined (See 'Adjust/Filter')",FilterOperator::LPF,filterTaps,lowpassHz/samplingFrequency,0.2,(double)trans_width/samplingFrequency,(m_iWindowSize+m_iFilterTaps)));
    }

    if(ui->m_comboBox_filterType->currentText() == "Highpass") {
        userDefinedFilterOperator = QSharedPointer<MNEOperator>(
                   new FilterOperator("User defined (See 'Adjust/Filter')",FilterOperator::HPF,filterTaps,highpassHz/samplingFrequency,0.2,(double)trans_width/samplingFrequency,(m_iWindowSize+m_iFilterTaps)));
    }

    if(ui->m_comboBox_filterType->currentText() == "Bandpass") {
        userDefinedFilterOperator = QSharedPointer<MNEOperator>(
                   new FilterOperator("User defined (See 'Adjust/Filter')",FilterOperator::BPF,filterTaps,(double)center/samplingFrequency,(double)bw/samplingFrequency,(double)trans_width/samplingFrequency,(m_iWindowSize+m_iFilterTaps)));
    }

    //Replace old with new filter operator
    QMutableMapIterator<QString,QSharedPointer<MNEOperator> > it(m_pMainWindow->m_pDataWindow->getDataModel()->m_Operators);
    while(it.hasNext()) {
        it.next();
        if(it.key() == "User defined (See 'Adjust/Filter')") {
            it.setValue(userDefinedFilterOperator);
        }
    }

    //update filter plot
    updateFilterPlot();
}


//*************************************************************************************************************

void FilterWindow::applyFilter()
{
    //Undo all previous filters first
    m_pMainWindow->m_pDataWindow->getDataModel()->undoFilter(ui->m_comboBox_filterApplyTo->currentText());

    QMutableMapIterator<QString,QSharedPointer<MNEOperator> > it(m_pMainWindow->m_pDataWindow->getDataModel()->m_Operators);

    while(it.hasNext()) {
        it.next();
        if(it.key() == "User defined (See 'Adjust/Filter')") {
            m_pMainWindow->m_pDataWindow->getDataModel()->applyOperator(QModelIndexList(), it.value(), ui->m_comboBox_filterApplyTo->currentText());
        }
    }

    m_pMainWindow->m_pDataWindow->updateDataTableViews();
}


//*************************************************************************************************************

void FilterWindow::undoFilter()
{
    m_pMainWindow->m_pDataWindow->getDataModel()->undoFilter(ui->m_comboBox_filterUndoTo->currentText());

    m_pMainWindow->m_pDataWindow->updateDataTableViews();
}


//*************************************************************************************************************

void FilterWindow::exportFilterPlot()
{
    // Open file dialog
    QDate date;
    QString fileName = QFileDialog::getSaveFileName(this,
                                                    "Save filter plot",
                                                    QString("%1/%2_%3_%4_FilterPlot").arg(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation)).arg(date.currentDate().year()).arg(date.currentDate().month()).arg(date.currentDate().day()),
                                                    tr("Vector graphic(*.svg);;Images (*.png)"));

    if(!fileName.isEmpty())
    {
        // Generate screenshot
        if(fileName.contains(".svg"))
        {
            QSvgGenerator svgGen;

            svgGen.setFileName(fileName);
            QRectF rect = m_pFilterPlotScene->itemsBoundingRect();
            svgGen.setSize(QSize(rect.width(), rect.height()));
            //svgGen.setViewBox(QRect(0, 0, rect.width(), rect.height()));

            QPainter painter(&svgGen);
            m_pFilterPlotScene->render(&painter);
        }

        if(fileName.contains(".png"))
        {
            m_pFilterPlotScene->setSceneRect(m_pFilterPlotScene->itemsBoundingRect());                  // Re-shrink the scene to it's bounding contents
            QImage image(m_pFilterPlotScene->sceneRect().size().toSize(), QImage::Format_ARGB32);       // Create the image with the exact size of the shrunk scene
            image.fill(Qt::transparent);                                                                // Start all pixels transparent

            QPainter painter(&image);
            m_pFilterPlotScene->render(&painter);
            image.save(fileName);
        }
    }
}


//*************************************************************************************************************

void FilterWindow::exportFilterCoefficients()
{
    // Open file dialog
    QDate date;
    QString fileName = QFileDialog::getSaveFileName(this,
                                                    "Save filter coefficients",
                                                    QString("%1/%2_%3_%4_FilterCoeffs").arg(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation)).arg(date.currentDate().year()).arg(date.currentDate().month()).arg(date.currentDate().day()),
                                                    tr("Text file(*.txt)"));

    if(!fileName.isEmpty())
    {
        QFile file(fileName);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
            return;

        //get user defined filter oeprator
        QSharedPointer<FilterOperator> currentFilter;

        QMutableMapIterator<QString,QSharedPointer<MNEOperator> > it(m_pMainWindow->m_pDataWindow->getDataModel()->m_Operators);
        while(it.hasNext()) {
            it.next();
            if(it.key() == "User defined (See 'Adjust/Filter')") {
                if(it.value()->m_OperatorType == MNEOperator::FILTER) {
                    currentFilter = it.value().staticCast<FilterOperator>();

                    //Write coefficients to file
                    QTextStream out(&file);
                    for(int i = 0 ; i<currentFilter->m_dCoeffA.cols() ;i++)
                        out << currentFilter->m_dCoeffA(i) << "\n";
                }
            }
        }

        file.close();
    }
}



