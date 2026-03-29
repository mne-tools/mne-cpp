//=============================================================================================================
/**
 * @file     filterwindow.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  2.1.0
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

#include <QSignalBlocker>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEBROWSE;
using namespace UTILSLIB;
using namespace FIFFLIB;
using namespace MNELIB;
using namespace DISPLIB;


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
}


//*************************************************************************************************************

FilterWindow::~FilterWindow()
{
}


//*************************************************************************************************************

void FilterWindow::newFileLoaded(FiffInfo::SPtr& pFiffInfo)
{
    //Update min max of spin boxes to nyquist
    if(!pFiffInfo) {
        return;
    }

    double samplingFrequency = pFiffInfo->sfreq;
    double nyquistFrequency = samplingFrequency/2;

    QSignalBlocker lowpassBlocker(ui->m_doubleSpinBox_lowpass);
    QSignalBlocker highpassBlocker(ui->m_doubleSpinBox_highpass);
    ui->m_doubleSpinBox_highpass->setMaximum(nyquistFrequency);
    ui->m_doubleSpinBox_lowpass->setMaximum(nyquistFrequency);

    filterParametersChanged();
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

    //Initial selection is a lowpass FIR design method.
    ui->m_doubleSpinBox_lowpass->setVisible(true);
    ui->m_label_lowpass->setVisible(true);
    ui->m_label_lowpass->setText("Cut-Off (Hz):");

    ui->m_doubleSpinBox_highpass->setVisible(false);
    ui->m_label_highpass->setVisible(false);
    ui->m_doubleSpinBox_highpass->setEnabled(false);

    ui->m_spinBox_filterTaps->setVisible(true);
    ui->m_label_filterTaps->setVisible(true);
    ui->m_label_filterTaps->setText("Filter order:");

    //If add filter to channel type combo box changes -> also change combo box for undo filtering
    connect(ui->m_comboBox_filterApplyTo, &QComboBox::currentTextChanged,
            ui->m_comboBox_filterUndoTo, &QComboBox::setCurrentText);

    // Normalize the initial widget state right away so the dock opens with the
    // correct labels, ranges, and FIR/IIR controls before the user changes anything.
    changeStateSpinBoxes(ui->m_comboBox_designMethod->currentIndex());
}


//*************************************************************************************************************

void FilterWindow::initFilterPlot()
{
    ui->m_graphicsView_filterPlot->setScene(m_pFilterPlotScene.get());
}


//*************************************************************************************************************

void FilterWindow::initTableViews()
{
    ui->m_tableView_activeFilters->setModel(m_pMainWindow->chInfoWindow()->getDataModel().data());

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

void FilterWindow::setFrequencies(double highpass, double lowpass)
{
    const bool hasHp = highpass >= 0.0;
    const bool hasLp = lowpass  >= 0.0;

    if(hasHp && hasLp) {
        ui->m_comboBox_filterType->setCurrentIndex(2); // Bandpass
    } else if(hasHp) {
        ui->m_comboBox_filterType->setCurrentIndex(1); // Highpass
    } else {
        ui->m_comboBox_filterType->setCurrentIndex(0); // Lowpass
    }

    if(hasHp) {
        ui->m_doubleSpinBox_highpass->setValue(highpass);
    }
    if(hasLp) {
        ui->m_doubleSpinBox_lowpass->setValue(lowpass);
    }
}


//*************************************************************************************************************

void FilterWindow::updateFilterPlot()
{
    const auto fiffInfo = m_pMainWindow->dataWindow()->fiffInfo();
    if(!fiffInfo) {
        return;
    }

    m_pFilterPlotScene->updateFilter(m_pUserDefinedFilter,
                                     static_cast<int>(fiffInfo->sfreq));

    const QRectF plotRect = m_pFilterPlotScene->itemsBoundingRect();
    if(!plotRect.isEmpty()) {
        ui->m_graphicsView_filterPlot->fitInView(plotRect, Qt::KeepAspectRatio);
    }
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
    const bool useIir = ui->m_comboBox_designMethod->currentText() == QLatin1String("Butterworth");
    ui->m_spinBox_filterTaps->setVisible(true);
    ui->m_label_filterTaps->setVisible(true);
    ui->m_label_filterTaps->setText(useIir ? QStringLiteral("Filter order:")
                                           : QStringLiteral("Filter order:"));
    ui->m_label_transitionBand->setVisible(!useIir);
    ui->m_doubleSpinBox_transitionband->setVisible(!useIir);

    if(useIir) {
        ui->m_spinBox_filterTaps->setMinimum(1);
        ui->m_spinBox_filterTaps->setMaximum(16);
        if(ui->m_spinBox_filterTaps->value() > 16) {
            ui->m_spinBox_filterTaps->setValue(4);
        }
        ui->label_2->setText(QStringLiteral("SOS sections:"));
    } else {
        ui->m_spinBox_filterTaps->setMinimum(9);
        ui->m_spinBox_filterTaps->setMaximum(2048);
        if(ui->m_spinBox_filterTaps->value() < 9) {
            ui->m_spinBox_filterTaps->setValue(256);
        }
        ui->label_2->setText(QStringLiteral("Response size:"));
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
        case 3: //Bandstop
            ui->m_doubleSpinBox_highpass->setVisible(true);
            ui->m_label_highpass->setVisible(true);
            ui->m_doubleSpinBox_lowpass->setVisible(true);
            ui->m_label_lowpass->setText("Stop Low (Hz):");

            ui->m_label_lowpass->setVisible(true);
            ui->m_doubleSpinBox_lowpass->setEnabled(true);
            ui->m_doubleSpinBox_highpass->setEnabled(true);
            ui->m_label_highpass->setText("Stop High (Hz):");
            break;
    }

    filterParametersChanged();
}


//*************************************************************************************************************

void FilterWindow::filterParametersChanged()
{
    const auto fiffInfo = m_pMainWindow->dataWindow()->fiffInfo();
    if(!fiffInfo) {
        return;
    }

    const double nyquistFrequency = fiffInfo->sfreq / 2.0;

    if(ui->m_comboBox_filterType->currentText() == "Bandpass"
       || ui->m_comboBox_filterType->currentText() == "Bandstop") {
        ui->m_doubleSpinBox_highpass->setMinimum(ui->m_doubleSpinBox_lowpass->value());
        ui->m_doubleSpinBox_lowpass->setMaximum(ui->m_doubleSpinBox_highpass->value());
    } else {
        ui->m_doubleSpinBox_highpass->setMaximum(nyquistFrequency);
        ui->m_doubleSpinBox_lowpass->setMaximum(nyquistFrequency);
    }

    m_pUserDefinedFilter = buildUserDefinedFilter();
    if(!m_pUserDefinedFilter.isNull()) {
        ui->m_label_fftLength->setText(QString::number(m_pUserDefinedFilter->responseSizeHint()));
        if(!m_pMainWindow->dataWindow()->activeSessionFilter().isNull()) {
            m_pMainWindow->dataWindow()->setUserDefinedFilter(m_pUserDefinedFilter);
        }
    } else {
        ui->m_label_fftLength->setText(QStringLiteral("-"));
    }
    updateFilterPlot();
}


//*************************************************************************************************************

void FilterWindow::applyFilter()
{
    if(m_pMainWindow->dataWindow()->fiffInfo().isNull()) {
        return;
    }

    if(m_pUserDefinedFilter.isNull()) {
        m_pUserDefinedFilter = buildUserDefinedFilter();
    }

    if(m_pUserDefinedFilter.isNull()) {
        return;
    }

    m_pMainWindow->dataWindow()->setUserDefinedFilter(m_pUserDefinedFilter);
    m_pMainWindow->dataWindow()->updateDataTableViews();
}


//*************************************************************************************************************

void FilterWindow::undoFilter()
{
    m_pMainWindow->dataWindow()->clearUserDefinedFilter();
    m_pMainWindow->dataWindow()->updateDataTableViews();
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
    if(m_pUserDefinedFilter.isNull()) {
        m_pUserDefinedFilter = buildUserDefinedFilter();
    }

    if(m_pUserDefinedFilter.isNull()) {
        return;
    }

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

        QTextStream out(&file);
        out << m_pUserDefinedFilter->coefficientExportText();

        file.close();
    }
}

//*************************************************************************************************************

QSharedPointer<SessionFilter> FilterWindow::buildUserDefinedFilter() const
{
    const auto fiffInfo = m_pMainWindow->dataWindow()->fiffInfo();
    if(!fiffInfo) {
        return {};
    }

    const double lowpassHz = ui->m_doubleSpinBox_lowpass->value();
    const double highpassHz = ui->m_doubleSpinBox_highpass->value();
    const double transWidth = ui->m_doubleSpinBox_transitionband->value();
    const double samplingFrequency = fiffInfo->sfreq;
    const int filterTaps = ui->m_spinBox_filterTaps->value();

    SessionFilter::DesignMethod designMethod = SessionFilter::DesignMethod::Cosine;
    if(ui->m_comboBox_designMethod->currentText() == QLatin1String("Tschebyscheff")) {
        designMethod = SessionFilter::DesignMethod::Tschebyscheff;
    } else if(ui->m_comboBox_designMethod->currentText() == QLatin1String("Butterworth")) {
        designMethod = SessionFilter::DesignMethod::Butterworth;
    }

    SessionFilter::FilterType filterType = SessionFilter::FilterType::LowPass;
    if(ui->m_comboBox_filterType->currentText() == QLatin1String("Highpass")) {
        filterType = SessionFilter::FilterType::HighPass;
    } else if(ui->m_comboBox_filterType->currentText() == QLatin1String("Bandpass")) {
        filterType = SessionFilter::FilterType::BandPass;
    } else if(ui->m_comboBox_filterType->currentText() == QLatin1String("Bandstop")) {
        filterType = SessionFilter::FilterType::BandStop;
    }

    double cutoffLowHz = lowpassHz;
    double cutoffHighHz = highpassHz;
    switch(filterType) {
        case SessionFilter::FilterType::LowPass:
            cutoffLowHz = lowpassHz;
            cutoffHighHz = lowpassHz;
            break;
        case SessionFilter::FilterType::HighPass:
            cutoffLowHz = highpassHz;
            cutoffHighHz = highpassHz;
            break;
        case SessionFilter::FilterType::BandPass:
        case SessionFilter::FilterType::BandStop:
            cutoffLowHz = lowpassHz;
            cutoffHighHz = highpassHz;
            break;
    }

    QSharedPointer<SessionFilter> filter = QSharedPointer<SessionFilter>::create(QStringLiteral("User defined (Filter Window)"),
                                                                                 designMethod,
                                                                                 filterType,
                                                                                 filterTaps,
                                                                                 cutoffLowHz,
                                                                                 cutoffHighHz,
                                                                                 std::max(0.1, transWidth),
                                                                                 samplingFrequency,
                                                                                 ui->m_comboBox_filterApplyTo->currentText());
    if(!filter->isValid()) {
        return {};
    }

    return filter;
}
