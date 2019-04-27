//=============================================================================================================
/**
* @file     filterdesignview.cpp
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
* @brief    Definition of the FilterDesignView class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "filterdesignview.h"
#include "ui_filterdesignview.h"

#include "helpers/filterplotscene.h"

#include "utils/mnemath.h"
#include "utils/filterTools/filterio.h"

#include <fiff/fiff_info.h>


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QDate>
#include <QFileDialog>
#include <QStandardPaths>
#include <QSvgGenerator>
#include <QCheckBox>
#include <QSettings>
#include <QApplication>
#include <QDesktopWidget>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;
using namespace FIFFLIB;
using namespace UTILSLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FilterDesignView::FilterDesignView(const QString& sSettingsPath,
                       QWidget *parent,
                       Qt::WindowFlags f)
: QWidget(parent, f)
, ui(new Ui::FilterDesignViewWidget)
, m_iWindowSize(4016)
, m_iFilterTaps(512)
, m_dSFreq(600)
, m_sSettingsPath(sSettingsPath)
{
    ui->setupUi(this);

    initSpinBoxes();
    initButtons();
    initComboBoxes();
    initFilterPlot();

    loadSettings(m_sSettingsPath);
}


//*************************************************************************************************************

FilterDesignView::~FilterDesignView()
{
    saveSettings(m_sSettingsPath);

    delete ui;
}


//*************************************************************************************************************

void FilterDesignView::init(double dSFreq)
{
    setSamplingRate(dSFreq);

    //Update min max of spin boxes to nyquist
    double samplingFrequency = m_dSFreq;
    double nyquistFrequency = samplingFrequency/2;

    ui->m_doubleSpinBox_highpass->setMaximum(nyquistFrequency);
    ui->m_doubleSpinBox_lowpass->setMaximum(nyquistFrequency);

    if(ui->m_doubleSpinBox_highpass->value()>m_dSFreq/2) {
        ui->m_doubleSpinBox_highpass->setValue(m_dSFreq/2);
    }

    if(ui->m_doubleSpinBox_lowpass->value()>m_dSFreq/2) {
        ui->m_doubleSpinBox_lowpass->setValue(m_dSFreq/2);
    }

    filterParametersChanged();

    updateFilterPlot();
}


//*************************************************************************************************************

void FilterDesignView::setWindowSize(int iWindowSize)
{
    m_iWindowSize = iWindowSize;

    //Only set even numbers -> otherwise cosine design method gives wrong results
    if(m_iWindowSize%2 != 0) {
        m_iWindowSize--;
    }

    //ui->m_spinBox_filterTaps->setValue(m_iWindowSize);
}


//*************************************************************************************************************

void FilterDesignView::setMaxFilterTaps(int iMaxNumberFilterTaps)
{
    if(iMaxNumberFilterTaps%2 != 0) {
        iMaxNumberFilterTaps--;
    }

    if(iMaxNumberFilterTaps > 512) {
        iMaxNumberFilterTaps = 512;
    }

    ui->m_spinBox_filterTaps->setMaximum(iMaxNumberFilterTaps);
    ui->m_spinBox_filterTaps->setMinimum(16);

    //Update filter depending on new window size
    filterParametersChanged();
}


//*************************************************************************************************************

void FilterDesignView::setSamplingRate(double dSamplingRate)
{
    m_dSFreq = dSamplingRate;

    if(ui->m_doubleSpinBox_highpass->value()>m_dSFreq/2) {
        ui->m_doubleSpinBox_highpass->setValue(m_dSFreq/2);
    }

    if(ui->m_doubleSpinBox_lowpass->value()>m_dSFreq/2) {
        ui->m_doubleSpinBox_lowpass->setValue(m_dSFreq/2);
    }
}


//*************************************************************************************************************

void FilterDesignView::setFilterParameters(double hp,
                                           double lp,
                                           int order,
                                           int type,
                                           int designMethod,
                                           double transition,
                                           const QString &sChannelType)
{
    ui->m_doubleSpinBox_highpass->setValue(lp);
    ui->m_doubleSpinBox_lowpass->setValue(hp);
    ui->m_spinBox_filterTaps->setValue(order);

    if(type == 0) {
        ui->m_comboBox_filterType->setCurrentText("Lowpass");
    }
    if(type == 1) {
        ui->m_comboBox_filterType->setCurrentText("Highpass");
    }
    if(type == 2) {
        ui->m_comboBox_filterType->setCurrentText("Bandpass");
    }
    if(type == 3) {
        ui->m_comboBox_filterType->setCurrentText("Notch");
    }

    if(designMethod == 0) {
        ui->m_comboBox_designMethod->setCurrentText("Tschebyscheff");
    }
    if(designMethod == 1) {
        ui->m_comboBox_designMethod->setCurrentText("Cosine");
    }

    ui->m_doubleSpinBox_transitionband->setValue(transition);

    ui->m_comboBox_filterApplyTo->setCurrentText(sChannelType);

    filterParametersChanged();
}


//*************************************************************************************************************

FilterData FilterDesignView::getCurrentFilter()
{
    return m_filterData;
}


//*************************************************************************************************************

QString FilterDesignView::getChannelType()
{
    return ui->m_comboBox_filterApplyTo->currentText();
}


//*************************************************************************************************************

void FilterDesignView::saveSettings(const QString& settingsPath)
{
    if(settingsPath.isEmpty()) {
        return;
    }

    QSettings settings;

    settings.setValue(settingsPath + QString("/filterHP"), m_filterData.m_dHighpassFreq);
    settings.setValue(settingsPath + QString("/filterLP"), m_filterData.m_dLowpassFreq);
    settings.setValue(settingsPath + QString("/filterOrder"), m_filterData.m_iFilterOrder);
    settings.setValue(settingsPath + QString("/filterType"), m_filterData.m_Type);
    settings.setValue(settingsPath + QString("/filterDesignMethod"), m_filterData.m_designMethod);
    settings.setValue(settingsPath + QString("/filterTransition"), m_filterData.m_dParksWidth*(m_filterData.m_sFreq/2));
    settings.setValue(settingsPath + QString("/filterChannelType"), getChannelType());
    settings.setValue(settingsPath + QString("/FilterDesignViewPos"), this->pos());
}


//*************************************************************************************************************

void FilterDesignView::loadSettings(const QString& settingsPath)
{
    if(settingsPath.isEmpty()) {
        return;
    }

    QSettings settings;

    //Set stored filter settings from last session
    setFilterParameters(settings.value(settingsPath + QString("/filterHP"), 5.0).toDouble(),
                        settings.value(settingsPath + QString("/filterLP"), 40.0).toDouble(),
                        settings.value(settingsPath + QString("/filterOrder"), 128).toInt(),
                        settings.value(settingsPath + QString("/filterType"), 2).toInt(),
                        settings.value(settingsPath + QString("/filterDesignMethod"), 0).toInt(),
                        settings.value(settingsPath + QString("/filterTransition"), 5.0).toDouble(),
                        settings.value(settingsPath + QString("/filterChannelType"), "MEG").toString());

    QPoint pos = settings.value(settingsPath + QString("/FilterDesignViewPos"), QPoint(100,100)).toPoint();

    QRect screenRect = QApplication::desktop()->screenGeometry();
    if(!screenRect.contains(pos) && QGuiApplication::screens().size() == 1) {
        move(QPoint(100,100));
    } else {
        move(pos);
    }
}


//*************************************************************************************************************

void FilterDesignView::initSpinBoxes()
{
    ui->m_doubleSpinBox_lowpass->setValue(5.0);
    ui->m_doubleSpinBox_highpass->setValue(50.0);
    ui->m_doubleSpinBox_transitionband->setValue(4.0);

    connect(ui->m_doubleSpinBox_lowpass,static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
                this,&FilterDesignView::filterParametersChanged);

    connect(ui->m_doubleSpinBox_highpass,static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
                this,&FilterDesignView::filterParametersChanged);

    connect(ui->m_doubleSpinBox_transitionband,static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
                this,&FilterDesignView::filterParametersChanged);

    connect(ui->m_spinBox_filterTaps,static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
                this,&FilterDesignView::filterParametersChanged);

    //Intercept events from the spin boxes to get control over key events
    ui->m_doubleSpinBox_lowpass->installEventFilter(this);
    ui->m_doubleSpinBox_highpass->installEventFilter(this);
    ui->m_doubleSpinBox_transitionband->installEventFilter(this);
}


//*************************************************************************************************************

void FilterDesignView::initButtons()
{
    connect(ui->m_pushButton_exportPlot,&QPushButton::released,
                this,&FilterDesignView::onBtnExportFilterPlot);

    connect(ui->m_pushButton_exportFilter,&QPushButton::released,
                this,&FilterDesignView::onBtnExportFilterCoefficients);

    connect(ui->m_pushButton_loadFilter,&QPushButton::released,
                this,&FilterDesignView::onBtnLoadFilter);
}


//*************************************************************************************************************

void FilterDesignView::initComboBoxes()
{
    connect(ui->m_comboBox_designMethod,static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
                this,&FilterDesignView::changeStateSpinBoxes);

    connect(ui->m_comboBox_filterType,static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
                this,&FilterDesignView::changeStateSpinBoxes);

    //Initial selection is a bandpass and Cosine design method
    ui->m_doubleSpinBox_lowpass->setVisible(true);
    ui->m_label_lowpass->setVisible(true);

    ui->m_doubleSpinBox_highpass->setVisible(true);
    ui->m_label_highpass->setVisible(true);

    ui->m_spinBox_filterTaps->setVisible(true);
    ui->m_label_filterTaps->setVisible(true);

    connect(ui->m_comboBox_filterApplyTo, &QComboBox::currentTextChanged,
            this, &FilterDesignView::onSpinBoxFilterChannelType);

    ui->m_comboBox_filterApplyTo->setCurrentIndex(1);
}


//*************************************************************************************************************

void FilterDesignView::initFilterPlot()
{
    m_pFilterPlotScene = FilterPlotScene::SPtr::create(ui->m_graphicsView_filterPlot, this);

    ui->m_graphicsView_filterPlot->setScene(m_pFilterPlotScene.data());

    updateFilterPlot();
}


//*************************************************************************************************************

void FilterDesignView::resizeEvent(QResizeEvent* event)
{
    Q_UNUSED(event);
    ui->m_graphicsView_filterPlot->fitInView(m_pFilterPlotScene->itemsBoundingRect(), Qt::KeepAspectRatio);
}


//*************************************************************************************************************

void FilterDesignView::keyPressEvent(QKeyEvent * event)
{
    if(event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return) {
        emit filterChannelTypeChanged(ui->m_comboBox_filterApplyTo->currentText());
    }

    if((event->modifiers() == Qt::ControlModifier && event->key() == Qt::Key_Z) || event->key() == Qt::Key_Delete) {
        emit filterChannelTypeChanged(ui->m_comboBox_filterApplyTo->currentText());
    }
}


//*************************************************************************************************************

void FilterDesignView::updateFilterPlot()
{
    //Update the filter of the scene
    m_pFilterPlotScene->updateFilter(m_filterData,
                                     m_filterData.m_sFreq, //Pass the filters sampling frequency, not the one from the fiff info. Reason: sFreq from a loaded filter could be different
                                     ui->m_doubleSpinBox_lowpass->value(),
                                     ui->m_doubleSpinBox_highpass->value());

    ui->m_graphicsView_filterPlot->fitInView(m_pFilterPlotScene->itemsBoundingRect(), Qt::KeepAspectRatio);
}


//*************************************************************************************************************

void FilterDesignView::changeStateSpinBoxes(int currentIndex)
{
    Q_UNUSED(currentIndex);

    //Change visibility of filter tap spin boxes depending on filter design method
    switch(ui->m_comboBox_designMethod->currentIndex()) {
        case 0: //Cosine
//            ui->m_spinBox_filterTaps->setVisible(false);
//            ui->m_label_filterTaps->setVisible(false);
            ui->m_spinBox_filterTaps->setVisible(true);
            ui->m_label_filterTaps->setVisible(true);
            break;

        case 1: //Tschebyscheff
            ui->m_spinBox_filterTaps->setVisible(true);
            ui->m_label_filterTaps->setVisible(true);
            break;
    }

    //Change visibility of spin boxes depending on filter type
    switch(ui->m_comboBox_filterType->currentIndex()) {
        case 0: //Bandpass
            ui->m_doubleSpinBox_highpass->setVisible(true);
            ui->m_label_highpass->setVisible(true);
            ui->m_doubleSpinBox_lowpass->setVisible(true);
            ui->m_label_lowpass->setText("Highpass (Hz):");

            ui->m_label_lowpass->setVisible(true);
            ui->m_doubleSpinBox_lowpass->setEnabled(true);
            ui->m_doubleSpinBox_highpass->setEnabled(true);
            ui->m_label_highpass->setText("Lowpass (Hz):");
            break;

        case 1: //Lowpass
            ui->m_doubleSpinBox_lowpass->setVisible(true);
            ui->m_label_lowpass->setVisible(true);
            ui->m_label_lowpass->setText("Highpass (Hz):");

            ui->m_doubleSpinBox_highpass->setVisible(false);
            ui->m_label_highpass->setVisible(false);
            ui->m_doubleSpinBox_highpass->setEnabled(false);
            break;

        case 2: //Highpass
            ui->m_doubleSpinBox_highpass->setVisible(true);
            ui->m_label_highpass->setVisible(true);
            ui->m_label_highpass->setText("Lowpass (Hz):");

            ui->m_doubleSpinBox_lowpass->setVisible(false);
            ui->m_label_lowpass->setVisible(false);
            ui->m_doubleSpinBox_highpass->setEnabled(true);
            break;
    }

    filterParametersChanged();
}


//*************************************************************************************************************

void FilterDesignView::filterParametersChanged()
{
    //User defined filter parameters
    double lowpassHz = ui->m_doubleSpinBox_lowpass->value();
    double highpassHz = ui->m_doubleSpinBox_highpass->value();

    double trans_width = ui->m_doubleSpinBox_transitionband->value();

    double bw = highpassHz-lowpassHz;
    double center = lowpassHz+bw/2;

    double samplingFrequency = m_dSFreq <= 0 ? 600 : m_dSFreq;
    double nyquistFrequency = samplingFrequency/2;

    //Calculate the needed fft length
    m_iFilterTaps = ui->m_spinBox_filterTaps->value();
    if(ui->m_spinBox_filterTaps->value()%2 != 0)
        m_iFilterTaps--;

    int fftLength = m_iWindowSize + ui->m_spinBox_filterTaps->value() * 4; // *2 to take into account the overlap in front and back after the convolution. Another *2 to take into account the appended and prepended data.
    int exp = ceil(MNEMath::log2(fftLength));
    fftLength = pow(2, exp) <512 ? 512 : pow(2, exp);

//    qDebug() <<"fftLength: "<<fftLength;
//    qDebug()<<"m_iWindowSize: "<<m_iWindowSize;
//    qDebug()<<"m_iWindowSize + ui->m_spinBox_filterTaps->value() * 4: "<<m_iWindowSize + ui->m_spinBox_filterTaps->value() * 4;

    //set maximum and minimum for cut off frequency spin boxes
    ui->m_doubleSpinBox_highpass->setMaximum(nyquistFrequency);
    ui->m_doubleSpinBox_lowpass->setMaximum(nyquistFrequency);
    ui->m_doubleSpinBox_highpass->setMinimum(0);
    ui->m_doubleSpinBox_lowpass->setMinimum(0);

    if(ui->m_comboBox_filterType->currentText() == "Bandpass") {
        if((ui->m_doubleSpinBox_highpass->value() < ui->m_doubleSpinBox_lowpass->value())) {
            ui->m_doubleSpinBox_highpass->setValue(ui->m_doubleSpinBox_lowpass->value());
        }
        ui->m_doubleSpinBox_highpass->setMinimum(ui->m_doubleSpinBox_lowpass->value());
        ui->m_doubleSpinBox_lowpass->setMaximum(ui->m_doubleSpinBox_highpass->value());
    }

    //set filter design method
    FilterData::DesignMethod dMethod = FilterData::Tschebyscheff;
    if(ui->m_comboBox_designMethod->currentText() == "Tschebyscheff") {
        dMethod = FilterData::Tschebyscheff;
    }

    if(ui->m_comboBox_designMethod->currentText() == "Cosine") {
        dMethod = FilterData::Cosine;
    }

    //Generate filters
    //Note: Always use "User Design" as filter name for user designed filters, which are stored in the model. This needs to be done because there only should be one filter in this model which holds the user designed filter.
    //Otherwise everytime a filter is designed a new filter would be added to this model -> too much storage consumption.
    if(ui->m_comboBox_filterType->currentText() == "Lowpass") {
        m_filterData = FilterData("User Design",
                                  FilterData::LPF,
                                  m_iFilterTaps,
                                  lowpassHz/nyquistFrequency,
                                  0.2,
                                  (double)trans_width/nyquistFrequency,
                                  samplingFrequency,
                                  fftLength,
                                  dMethod);
    }

    if(ui->m_comboBox_filterType->currentText() == "Highpass") {
        m_filterData = FilterData("User Design",
                                  FilterData::HPF,
                                  m_iFilterTaps,
                                  highpassHz/nyquistFrequency,
                                  0.2,
                                  (double)trans_width/nyquistFrequency,
                                  samplingFrequency,
                                  fftLength,
                                  dMethod);
    }

    if(ui->m_comboBox_filterType->currentText() == "Bandpass") {
        m_filterData = FilterData("User Design",
                                  FilterData::BPF,
                                  m_iFilterTaps,
                                  (double)center/nyquistFrequency,
                                  (double)bw/nyquistFrequency,
                                  (double)trans_width/nyquistFrequency,
                                  samplingFrequency,
                                  fftLength,
                                  dMethod);
    }

    emit filterChanged(m_filterData);

    //update filter plot
    updateFilterPlot();
}


//*************************************************************************************************************

void FilterDesignView::onSpinBoxFilterChannelType(const QString& channelType)
{
    emit filterChannelTypeChanged(channelType);
}


//*************************************************************************************************************

void FilterDesignView::onBtnExportFilterPlot()
{
    // Open file dialog
    QDate date;
    QString fileName = QFileDialog::getSaveFileName(this,
                                                    "Save filter plot",
                                                    QString("%1/%2_%3_%4_FilterPlot").arg(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation)).arg(date.currentDate().year()).arg(date.currentDate().month()).arg(date.currentDate().day()),
                                                    tr("Vector graphic(*.svg);;Images (*.png)"));

    if(!fileName.isEmpty()) {
        // Generate screenshot
        if(fileName.contains(".svg")) {
            QSvgGenerator svgGen;

            svgGen.setFileName(fileName);
            QRectF rect = m_pFilterPlotScene->itemsBoundingRect();
            svgGen.setSize(QSize(rect.width(), rect.height()));
            //svgGen.setViewBox(QRect(0, 0, rect.width(), rect.height()));

            QPainter painter(&svgGen);
            m_pFilterPlotScene->render(&painter);
        }

        if(fileName.contains(".png")) {
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

void FilterDesignView::onBtnExportFilterCoefficients()
{
    //Generate appropriate name for the filter to be saved
    QString filtername;
    if(m_filterData.m_Type == FilterData::LPF)
        filtername = QString("%1_%2_Fs%3").arg(FilterData::getStringForFilterType(m_filterData.m_Type)).arg((int)m_filterData.m_dHighpassFreq).arg((int)m_filterData.m_sFreq);

    if(m_filterData.m_Type == FilterData::HPF)
        filtername = QString("%1_%2_Fs%3").arg(FilterData::getStringForFilterType(m_filterData.m_Type)).arg((int)m_filterData.m_dLowpassFreq).arg((int)m_filterData.m_sFreq);

    if(m_filterData.m_Type == FilterData::BPF)
         filtername = QString("%1_%2_%3_Fs%4").arg(FilterData::getStringForFilterType(m_filterData.m_Type)).arg((int)m_filterData.m_dHighpassFreq).arg((int)m_filterData.m_dLowpassFreq).arg((int)m_filterData.m_sFreq);

    //Do not pass m_filterData because this is most likely the User Defined filter which name should not change due to the filter model implementation. Hence use temporal copy of m_filterData.
    FilterData filterWriteTemp = m_filterData;
    filterWriteTemp.m_sName = filtername;

    QString fileName = QFileDialog::getSaveFileName(this,
                                                    "Save filter coefficients",
                                                    QString("%1/%2").arg(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation)).arg(filtername),
                                                    tr("Text file(*.txt)"));

    FilterIO::writeFilter(fileName, filterWriteTemp);
}


//*************************************************************************************************************

void FilterDesignView::onBtnLoadFilter()
{
    QString path = QFileDialog::getOpenFileName(this,
                                                QString("Load filter"),
                                                QString("./"),
                                                tr("txt files (*.txt)"));

    if(!path.isEmpty()) {
        //Replace old with new filter operator
        FilterData filterLoadTemp;

        if(!FilterIO::readFilter(path, filterLoadTemp)) {
            return;
        }

        m_filterData = filterLoadTemp;

        emit filterChanged(m_filterData);

        updateFilterPlot();
    } else {
        qDebug()<<"Could not load filter.";
    }
}
