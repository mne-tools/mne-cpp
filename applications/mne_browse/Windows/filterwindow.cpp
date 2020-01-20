//=============================================================================================================
/**
 * @file     filterwindow.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  dev
 * @date     August, 2014
 *
 * @section  LICENSE
 *
 * Copyright (C) 2014, Christoph Dinh, Lorenz Esch. All rights reserved.
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
 * @brief    Definition of the FilterWindow class.
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

using namespace MNEBROWSE;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FilterWindow::FilterWindow(MainWindow *mainWindow, QWidget *parent)
: QDockWidget(parent)
, ui(new Ui::FilterWindowDockWidget)
, m_pFilterPlotScene(new FilterPlotScene)
, m_pMainWindow(mainWindow)
{
    ui->setupUi(this);

    initSpinBoxes();
    initButtons();
    initComboBoxes();
    initFilterPlot();
    initTableViews();

    m_iWindowSize = MODEL_WINDOW_SIZE;
    m_iFilterTaps = MODEL_NUM_FILTER_TAPS;
}


//*************************************************************************************************************

FilterWindow::~FilterWindow()
{
    delete ui;
}


//*************************************************************************************************************

void FilterWindow::newFileLoaded(FiffInfo::SPtr& pFiffInfo)
{
    Q_UNUSED(pFiffInfo);
    filterParametersChanged();

    //Update min max of spin boxes to nyquist
    double samplingFrequency = m_pMainWindow->m_pDataWindow->getDataModel()->m_pFiffInfo->sfreq;
    double nyquistFrequency = samplingFrequency/2;

    ui->m_doubleSpinBox_highpass->setMaximum(nyquistFrequency);
    ui->m_doubleSpinBox_lowpass->setMaximum(nyquistFrequency);
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
    connect(ui->m_comboBox_designMethod,static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
                this,&FilterWindow::changeStateSpinBoxes);

    connect(ui->m_comboBox_filterType,static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
                this,&FilterWindow::changeStateSpinBoxes);

    //Initial selection is a lowpass and Cosine design method
    ui->m_doubleSpinBox_lowpass->setVisible(true);
    ui->m_label_lowpass->setVisible(true);
    ui->m_label_lowpass->setText("Cut-Off (Hz):");

    ui->m_doubleSpinBox_highpass->setVisible(false);
    ui->m_label_highpass->setVisible(false);
    ui->m_doubleSpinBox_highpass->setEnabled(false);

    ui->m_spinBox_filterTaps->setVisible(false);
    ui->m_label_filterTaps->setVisible(false);

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
    ui->m_tableView_activeFilters->setModel(m_pMainWindow->m_pChInfoWindow->getDataModel().data());

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
                                             m_pMainWindow->m_pDataWindow->getDataModel()->m_pFiffInfo->sfreq,
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

            if((keyEvent->modifiers() == Qt::ControlModifier && keyEvent->key() == Qt::Key_Z)/* || keyEvent->key() == Qt::Key_Delete*/)
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
    Q_UNUSED(currentIndex);

    //Change visibility offilter tap spin boxes depending on filter design method
    switch(ui->m_comboBox_designMethod->currentIndex()) {
        case 0: //Cosine
            ui->m_spinBox_filterTaps->setVisible(false);
            ui->m_label_filterTaps->setVisible(false);
            break;

        case 1: //Tschebyscheff
            ui->m_spinBox_filterTaps->setVisible(true);
            ui->m_label_filterTaps->setVisible(true);
            break;
    }

    //Change visibility of spin boxes depending on filter type
    switch(ui->m_comboBox_filterType->currentIndex()) {
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

    double trans_width = ui->m_doubleSpinBox_transitionband->value();

    double bw = highpassHz-lowpassHz;
    double center = lowpassHz+bw/2;

    double samplingFrequency = m_pMainWindow->m_pDataWindow->getDataModel()->m_pFiffInfo->sfreq;
    double nyquistFrequency = samplingFrequency/2;

    //Calculate the needed fft length
    int filterTaps = ui->m_spinBox_filterTaps->value();
    int fftLength = m_iWindowSize;
    int exp = ceil(MNEMath::log2(fftLength));
    fftLength = pow(2, exp+1);

    //set maximum and minimum for cut off frequency spin boxes
    if(ui->m_comboBox_filterType->currentText() == "Bandpass") {
        ui->m_doubleSpinBox_highpass->setMinimum(ui->m_doubleSpinBox_lowpass->value());
        ui->m_doubleSpinBox_lowpass->setMaximum(ui->m_doubleSpinBox_highpass->value());
    }
    else {
        ui->m_doubleSpinBox_highpass->setMaximum(nyquistFrequency);
        ui->m_doubleSpinBox_lowpass->setMaximum(nyquistFrequency);
    }

    //set current fft length info label
    ui->m_label_fftLength->setText(QString().number(fftLength));

    //set filter design method
    FilterOperator::DesignMethod dMethod = FilterOperator::Tschebyscheff;
    if(ui->m_comboBox_designMethod->currentText() == "Tschebyscheff")
        dMethod = FilterOperator::Tschebyscheff;

    if(ui->m_comboBox_designMethod->currentText() == "Cosine")
        dMethod = FilterOperator::Cosine;

    //Generate filters
    QSharedPointer<MNEOperator> userDefinedFilterOperator;

    if(ui->m_comboBox_filterType->currentText() == "Lowpass") {
        userDefinedFilterOperator = QSharedPointer<MNEOperator>(
                   new FilterOperator("User defined (See 'Adjust/Filter')",FilterOperator::LPF,filterTaps,lowpassHz/nyquistFrequency,0.2,(double)trans_width/nyquistFrequency,samplingFrequency,fftLength,dMethod));
    }

    if(ui->m_comboBox_filterType->currentText() == "Highpass") {
        userDefinedFilterOperator = QSharedPointer<MNEOperator>(
                   new FilterOperator("User defined (See 'Adjust/Filter')",FilterOperator::HPF,filterTaps,highpassHz/nyquistFrequency,0.2,(double)trans_width/nyquistFrequency,samplingFrequency,fftLength,dMethod));
    }

    if(ui->m_comboBox_filterType->currentText() == "Bandpass") {
        userDefinedFilterOperator = QSharedPointer<MNEOperator>(
                   new FilterOperator("User defined (See 'Adjust/Filter')",FilterOperator::BPF,filterTaps,(double)center/nyquistFrequency,(double)bw/nyquistFrequency,(double)trans_width/nyquistFrequency,samplingFrequency,fftLength,dMethod));
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



