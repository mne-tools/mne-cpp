//=============================================================================================================
/**
 * @file     filterdesignview.cpp
 * @author   Gabriel B Motta <gabrielbenmotta@gmail.com>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     August, 2014
 *
 * @section  LICENSE
 *
 * Copyright (C) 2014, Gabriel B Motta, Lorenz Esch, Christoph Dinh. All rights reserved.
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

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "filterdesignview.h"
#include "ui_filterdesignview.h"

#include "helpers/filterplotscene.h"

#include "utils/mnemath.h"

#include <rtprocessing/helpers/filterio.h>

#include <fiff/fiff_info.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDate>
#include <QFileDialog>
#include <QStandardPaths>
#include <QSvgGenerator>
#include <QCheckBox>
#include <QSettings>
#include <QApplication>
#include <QDesktopWidget>
#include <QKeyEvent>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;
using namespace FIFFLIB;
using namespace UTILSLIB;
using namespace RTPROCESSINGLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FilterDesignView::FilterDesignView(const QString& sSettingsPath,
                                   QWidget *parent,
                                   Qt::WindowFlags f)
: AbstractView(parent, f)
, m_pUi(new Ui::FilterDesignViewWidget)
, m_iWindowSize(4016)
, m_iFilterTaps(512)
, m_dSFreq(600)
{
    m_sSettingsPath = sSettingsPath;
    m_pUi->setupUi(this);

    initSpinBoxes();
    initButtons();
    initComboBoxes();
    initFilterPlot();

    loadSettings();
}

//=============================================================================================================

FilterDesignView::~FilterDesignView()
{
    saveSettings();

    delete m_pUi;
}

//=============================================================================================================

void FilterDesignView::setWindowSize(int iWindowSize)
{
    m_iWindowSize = iWindowSize;

    //Only set even numbers -> otherwise cosine design method gives wrong results
    if(m_iWindowSize%2 != 0) {
        m_iWindowSize--;
    }

    //m_pUi->m_spinBox_filterTaps->setValue(m_iWindowSize);
}

//=============================================================================================================

void FilterDesignView::setMaxFilterTaps(int iMaxNumberFilterTaps)
{
    if(iMaxNumberFilterTaps%2 != 0) {
        iMaxNumberFilterTaps--;
    }

    if(iMaxNumberFilterTaps > 4096) {
        iMaxNumberFilterTaps = 4096;
    }

    m_pUi->m_spinBox_filterTaps->setMaximum(iMaxNumberFilterTaps);
    m_pUi->m_spinBox_filterTaps->setMinimum(16);

    //Update filter depending on new window size
    filterParametersChanged();
}

//=============================================================================================================

int FilterDesignView::getMaxFilterTaps()
{
    return m_pUi->m_spinBox_filterTaps->maximum();
}

//=============================================================================================================

void FilterDesignView::setSamplingRate(double dSamplingRate)
{
    if(dSamplingRate <= 0) {
        qWarning() << "[FilterDesignView::setSamplingRate] Sampling frequency is <= 0. Returning.";
    }

    m_dSFreq = dSamplingRate;

    //Update min max of spin boxes to nyquist
    double nyquistFrequency = m_dSFreq/2;

    m_pUi->m_doubleSpinBox_to->setMaximum(nyquistFrequency);
    m_pUi->m_doubleSpinBox_from->setMaximum(nyquistFrequency);

    if(m_pUi->m_doubleSpinBox_to->value()>m_dSFreq/2) {
        m_pUi->m_doubleSpinBox_to->setValue(m_dSFreq/2);
    }

    if(m_pUi->m_doubleSpinBox_from->value()>m_dSFreq/2) {
        m_pUi->m_doubleSpinBox_from->setValue(m_dSFreq/2);
    }

    filterParametersChanged();

    updateFilterPlot();
}

//=============================================================================================================

void FilterDesignView::setFilterParameters(double from,
                                           double to,
                                           int order,
                                           int designMethod,
                                           double transition,
                                           const QString &sChannelType)
{
    m_pUi->m_doubleSpinBox_to->setValue(to);
    m_pUi->m_doubleSpinBox_from->setValue(from);
    m_pUi->m_spinBox_filterTaps->setValue(order);

    if(designMethod == 0) {
        m_pUi->m_comboBox_designMethod->setCurrentText("Tschebyscheff");
    }
    if(designMethod == 1) {
        m_pUi->m_comboBox_designMethod->setCurrentText("Cosine");
    }

    m_pUi->m_doubleSpinBox_transitionband->setValue(transition);

    m_pUi->m_comboBox_filterApplyTo->setCurrentText(sChannelType);

    filterParametersChanged();
}

//=============================================================================================================

FilterKernel FilterDesignView::getCurrentFilter()
{
    return m_filterKernel;
}

//=============================================================================================================

QString FilterDesignView::getChannelType()
{
    return m_pUi->m_comboBox_filterApplyTo->currentText();
}

//=============================================================================================================

void FilterDesignView::setChannelType(const QString& sType)
{
    m_pUi->m_comboBox_filterApplyTo->setCurrentText(sType);
}

//=============================================================================================================

void FilterDesignView::saveSettings()
{
    if(m_sSettingsPath.isEmpty()) {
        return;
    }

    QSettings settings("MNECPP");

    settings.setValue(m_sSettingsPath + QString("/FilterDesignView/filterFrom"), m_filterKernel.getHighpassFreq());
    settings.setValue(m_sSettingsPath + QString("/FilterDesignView/filterTo"), m_filterKernel.getLowpassFreq());
    settings.setValue(m_sSettingsPath + QString("/FilterDesignView/filterOrder"), m_filterKernel.getFilterOrder());
    settings.setValue(m_sSettingsPath + QString("/FilterDesignView/filterDesignMethod"), m_filterKernel.m_designMethod);
    settings.setValue(m_sSettingsPath + QString("/FilterDesignView/filterTransition"), m_filterKernel.getParksWidth()*(m_filterKernel.getSamplingFrequency()/2));
    settings.setValue(m_sSettingsPath + QString("/FilterDesignView/filterChannelType"), getChannelType());
    settings.setValue(m_sSettingsPath + QString("/FilterDesignView/Position"), this->pos());
}

//=============================================================================================================

void FilterDesignView::loadSettings()
{
    if(m_sSettingsPath.isEmpty()) {
        return;
    }

    QSettings settings("MNECPP");

    //Set stored filter settings from last session
    setFilterParameters(settings.value(m_sSettingsPath + QString("/FilterDesignView/filterFrom"), 5.0).toDouble(),
                        settings.value(m_sSettingsPath + QString("/FilterDesignView/filterTo"), 40.0).toDouble(),
                        settings.value(m_sSettingsPath + QString("/FilterDesignView/filterOrder"), 128).toInt(),
                        settings.value(m_sSettingsPath + QString("/FilterDesignView/filterDesignMethod"), 0).toInt(),
                        settings.value(m_sSettingsPath + QString("/FilterDesignView/filterTransition"), 1.0).toDouble(),
                        settings.value(m_sSettingsPath + QString("/FilterDesignView/filterChannelType"), "MEG").toString());

    QPoint pos = settings.value(m_sSettingsPath + QString("/FilterDesignView/Position"), QPoint(100,100)).toPoint();

    QRect screenRect = QApplication::desktop()->screenGeometry();
    if(!screenRect.contains(pos) && QGuiApplication::screens().size() == 1) {
        move(QPoint(100,100));
    } else {
        move(pos);
    }
}

//=============================================================================================================

void FilterDesignView::updateGuiMode(GuiMode mode)
{
    switch(mode) {
        case GuiMode::Clinical:
            break;
        default: // default is scientific mode
            break;
    }
}

//=============================================================================================================

void FilterDesignView::initSpinBoxes()
{
    m_pUi->m_doubleSpinBox_from->setValue(5.0);
    m_pUi->m_doubleSpinBox_to->setValue(50.0);
    m_pUi->m_doubleSpinBox_transitionband->setValue(1.0);

    connect(m_pUi->m_doubleSpinBox_from,static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
                this,&FilterDesignView::filterParametersChanged);

    connect(m_pUi->m_doubleSpinBox_to,static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
                this,&FilterDesignView::filterParametersChanged);

    connect(m_pUi->m_doubleSpinBox_transitionband,static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
                this,&FilterDesignView::filterParametersChanged);

    connect(m_pUi->m_spinBox_filterTaps,static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
                this,&FilterDesignView::filterParametersChanged);

    //Intercept events from the spin boxes to get control over key events
    m_pUi->m_doubleSpinBox_from->installEventFilter(this);
    m_pUi->m_doubleSpinBox_to->installEventFilter(this);
    m_pUi->m_doubleSpinBox_transitionband->installEventFilter(this);
}

//=============================================================================================================

void FilterDesignView::initButtons()
{
    connect(m_pUi->m_pushButton_exportPlot,&QPushButton::released,
                this,&FilterDesignView::onBtnExportFilterPlot);

    connect(m_pUi->m_pushButton_exportFilter,&QPushButton::released,
                this,&FilterDesignView::onBtnExportFilterCoefficients);

    connect(m_pUi->m_pushButton_loadFilter,&QPushButton::released,
                this,&FilterDesignView::onBtnLoadFilter);
}

//=============================================================================================================

void FilterDesignView::initComboBoxes()
{
    connect(m_pUi->m_comboBox_designMethod,static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
                this,&FilterDesignView::changeStateSpinBoxes);

    //Initial selection is a bandpass and Cosine design method
    m_pUi->m_doubleSpinBox_from->setVisible(true);
    m_pUi->m_label_lowpass->setVisible(true);

    m_pUi->m_doubleSpinBox_to->setVisible(true);
    m_pUi->m_label_highpass->setVisible(true);

    m_pUi->m_spinBox_filterTaps->setVisible(true);
    m_pUi->m_label_filterTaps->setVisible(true);

    connect(m_pUi->m_comboBox_filterApplyTo, &QComboBox::currentTextChanged,
            this, &FilterDesignView::onSpinBoxFilterChannelType);

    m_pUi->m_comboBox_filterApplyTo->setCurrentIndex(1);
}

//=============================================================================================================

void FilterDesignView::initFilterPlot()
{
    m_pFilterPlotScene = new FilterPlotScene(m_pUi->m_graphicsView_filterPlot, this);

    m_pUi->m_graphicsView_filterPlot->setScene(m_pFilterPlotScene);

    updateFilterPlot();
}

//=============================================================================================================

void FilterDesignView::resizeEvent(QResizeEvent* event)
{
    Q_UNUSED(event);
    m_pUi->m_graphicsView_filterPlot->fitInView(m_pFilterPlotScene->itemsBoundingRect(), Qt::KeepAspectRatio);
}

//=============================================================================================================

void FilterDesignView::keyPressEvent(QKeyEvent * event)
{
    if(event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return) {
        emit filterChannelTypeChanged(m_pUi->m_comboBox_filterApplyTo->currentText());
    }

    if((event->modifiers() == Qt::ControlModifier && event->key() == Qt::Key_Z) || event->key() == Qt::Key_Delete) {
        emit filterChannelTypeChanged(m_pUi->m_comboBox_filterApplyTo->currentText());
    }
}

//=============================================================================================================

void FilterDesignView::updateFilterPlot()
{
    //Update the filter of the scene
    m_pFilterPlotScene->updateFilter(m_filterKernel,
                                     m_filterKernel.getSamplingFrequency(), //Pass the filters sampling frequency, not the one from the fiff info. Reason: sFreq from a loaded filter could be different
                                     m_pUi->m_doubleSpinBox_from->value(),
                                     m_pUi->m_doubleSpinBox_to->value());

    m_pUi->m_graphicsView_filterPlot->fitInView(m_pFilterPlotScene->itemsBoundingRect(), Qt::KeepAspectRatio);
}

//=============================================================================================================

void FilterDesignView::changeStateSpinBoxes(int currentIndex)
{
    Q_UNUSED(currentIndex);

    //Change visibility of filter tap spin boxes depending on filter design method
    switch(m_pUi->m_comboBox_designMethod->currentIndex()) {
        case 0: //Cosine
//            m_pUi->m_spinBox_filterTaps->setVisible(false);
//            m_pUi->m_label_filterTaps->setVisible(false);
            m_pUi->m_spinBox_filterTaps->setVisible(true);
            m_pUi->m_label_filterTaps->setVisible(true);
            break;

        case 1: //Tschebyscheff
            m_pUi->m_spinBox_filterTaps->setVisible(true);
            m_pUi->m_label_filterTaps->setVisible(true);
            break;
    }

    filterParametersChanged();
}

//=============================================================================================================

void FilterDesignView::filterParametersChanged()
{
    //User defined filter parameters
    double from = m_pUi->m_doubleSpinBox_from->value();
    double to = m_pUi->m_doubleSpinBox_to->value();

    double trans_width = m_pUi->m_doubleSpinBox_transitionband->value();

    double bw = to-from;
    double center = from+bw/2;

    double nyquistFrequency = m_dSFreq/2;

    //Calculate the needed fft length
    m_iFilterTaps = m_pUi->m_spinBox_filterTaps->value();
    if(m_pUi->m_spinBox_filterTaps->value()%2 != 0) {
        m_iFilterTaps--;
    }

    int fftLength = m_iWindowSize + m_pUi->m_spinBox_filterTaps->value() * 4; // *2 to take into account the overlap in front and back after the convolution. Another *2 to take into account the appended and prepended data.
    int exp = ceil(MNEMath::log2(fftLength));
    fftLength = pow(2, exp) > 4096 ? 4096 : pow(2, exp);

    //set maximum and minimum for cut off frequency spin boxes
    m_pUi->m_doubleSpinBox_to->setMaximum(nyquistFrequency);
    m_pUi->m_doubleSpinBox_from->setMaximum(nyquistFrequency);
    m_pUi->m_doubleSpinBox_to->setMinimum(0);
    m_pUi->m_doubleSpinBox_from->setMinimum(0);

    if((m_pUi->m_doubleSpinBox_to->value() < m_pUi->m_doubleSpinBox_from->value())) {
        m_pUi->m_doubleSpinBox_to->setValue(m_pUi->m_doubleSpinBox_from->value());
    }

    m_pUi->m_doubleSpinBox_to->setMinimum(m_pUi->m_doubleSpinBox_from->value());
    m_pUi->m_doubleSpinBox_from->setMaximum(m_pUi->m_doubleSpinBox_to->value());

    //set filter design method
    FilterKernel::DesignMethod dMethod = FilterKernel::Tschebyscheff;
    if(m_pUi->m_comboBox_designMethod->currentText() == "Tschebyscheff") {
        dMethod = FilterKernel::Tschebyscheff;
    }

    if(m_pUi->m_comboBox_designMethod->currentText() == "Cosine") {
        dMethod = FilterKernel::Cosine;
    }

    //Generate filters
    m_filterKernel = FilterKernel("Designed Filter",
                              FilterKernel::BPF,
                              m_iFilterTaps,
                              (double)center/nyquistFrequency,
                              (double)bw/nyquistFrequency,
                              (double)trans_width/nyquistFrequency,
                              m_dSFreq,
                              fftLength,
                              dMethod);

    emit filterChanged(m_filterKernel);

    //update filter plot
    updateFilterPlot();

    saveSettings();
}

//=============================================================================================================

void FilterDesignView::onSpinBoxFilterChannelType(const QString& channelType)
{
    emit filterChannelTypeChanged(channelType);
}

//=============================================================================================================

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

//=============================================================================================================

void FilterDesignView::onBtnExportFilterCoefficients()
{
    //Generate appropriate name for the filter to be saved
    QString filtername;

    filtername = QString("%1_%2_%3_Fs%4").arg(FilterKernel::getStringForFilterType(m_filterKernel.m_Type)).arg((int)m_filterKernel.getHighpassFreq()).arg((int)m_filterKernel.getLowpassFreq()).arg((int)m_filterKernel.getSamplingFrequency());

    //Do not pass m_filterKernel because this is most likely the User Defined filter which name should not change due to the filter model implementation. Hence use temporal copy of m_filterKernel.
    FilterKernel filterWriteTemp = m_filterKernel;
    filterWriteTemp.getName() = filtername;

    QString fileName = QFileDialog::getSaveFileName(this,
                                                    "Save filter coefficients",
                                                    QString("%1/%2").arg(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation)).arg(filtername),
                                                    tr("Text file(*.txt)"));

    FilterIO::writeFilter(fileName, filterWriteTemp);
}

//=============================================================================================================

void FilterDesignView::onBtnLoadFilter()
{
    QString path = QFileDialog::getOpenFileName(this,
                                                QString("Load filter"),
                                                QString("./"),
                                                tr("txt files (*.txt)"));

    if(!path.isEmpty()) {
        //Replace old with new filter operator
        FilterKernel filterLoadTemp;

        if(!FilterIO::readFilter(path, filterLoadTemp)) {
            return;
        }

        m_filterKernel = filterLoadTemp;

        emit filterChanged(m_filterKernel);

        updateFilterPlot();
    } else {
        qDebug()<<"Could not load filter.";
    }
}
