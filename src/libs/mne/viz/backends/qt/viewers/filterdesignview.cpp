//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2014-2026 MNE-CPP Authors
 *
 * @file     filterdesignview.cpp
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Andreas Griesshammer <ag@fieldlineinc.com>;
 *           Limin Sun <limin.sun@childrens.harvard.edu>
 * @since    0.1.0
 * @date     August 2014
 * @brief    Implementation of the FilterDesignView interactive filter-design GUI (parameter→kernel pipeline, live plot).
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "filterdesignview.h"
#include <mne/base/log.h>
#include "ui_filterdesignview.h"

#include "../plots/filterplot.h"
#include "../plots/helpers/scenepainter.h"

#include <mne/base/string_utils.h>
#include <mne/dsp/filterio.h>
#include <mne/viz/figures/svg_renderer.h>

#include <mne/fiff/fiff_info.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDate>
#include <QFileDialog>
#include <QStandardPaths>
#include <QPainter>
#include <QCheckBox>
#include <QSettings>
#include <QApplication>
#include <QKeyEvent>
#include <QScreen>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::viz::qt;
using namespace mne;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FilterDesignView::FilterDesignView(const QString& sSettingsPath,
                                   QWidget* parent,
                                   Qt::WindowFlags f)
: AbstractView(parent, f)
, m_pUi(std::make_unique<Ui::FilterDesignViewWidget>())
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
    filterParametersChanged();
}

//=============================================================================================================

FilterDesignView::~FilterDesignView()
{
    saveSettings();
}

//=============================================================================================================

void FilterDesignView::setMaxAllowedFilterTaps(int iMaxNumberFilterTaps)
{
    if (iMaxNumberFilterTaps % 2 != 0) {
        iMaxNumberFilterTaps--;
    }

    m_pUi->m_spinBox_filterTaps->setMaximum(iMaxNumberFilterTaps);
    m_pUi->m_spinBox_filterTaps->setMinimum(16);

    //Update filter depending on new window size
    filterParametersChanged();
}

//=============================================================================================================

int FilterDesignView::getFilterTaps()
{
    return m_pUi->m_spinBox_filterTaps->value();
}

//=============================================================================================================

void FilterDesignView::setSamplingRate(double dSamplingRate)
{
    if (dSamplingRate <= 0) {
        mne::Log::warning("[FilterDesignView::setSamplingRate] Sampling frequency is <= 0. Returning.");
    }

    m_dSFreq = dSamplingRate;

    //Update min max of spin boxes to nyquist
    double nyquistFrequency = m_dSFreq / 2;

    m_pUi->m_doubleSpinBox_to->setMaximum(nyquistFrequency);
    m_pUi->m_doubleSpinBox_from->setMaximum(nyquistFrequency);

    if (m_pUi->m_doubleSpinBox_to->value() > m_dSFreq / 2) {
        m_pUi->m_doubleSpinBox_to->setValue(m_dSFreq / 2);
    }

    if (m_pUi->m_doubleSpinBox_from->value() > m_dSFreq / 2) {
        m_pUi->m_doubleSpinBox_from->setValue(m_dSFreq / 2);
    }

    filterParametersChanged();

    updateFilterPlot();
}

//=============================================================================================================

void FilterDesignView::setFrom(double dFrom)
{
    m_pUi->m_doubleSpinBox_from->setValue(dFrom);
    filterParametersChanged();
}

//=============================================================================================================

void FilterDesignView::setTo(double dTo)
{
    m_pUi->m_doubleSpinBox_to->setValue(dTo);
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
    saveSettings();
}

//=============================================================================================================

void FilterDesignView::saveSettings()
{
    if (m_sSettingsPath.isEmpty()) {
        return;
    }

    QSettings settings("MNECPP");

    settings.setValue(m_sSettingsPath + QString("/FilterDesignView/filterFrom"), m_filterKernel.highpass_freq());
    settings.setValue(m_sSettingsPath + QString("/FilterDesignView/filterTo"), m_filterKernel.lowpass_freq());
    settings.setValue(m_sSettingsPath + QString("/FilterDesignView/filterOrder"), m_filterKernel.filter_order());
    settings.setValue(m_sSettingsPath + QString("/FilterDesignView/filterDesignMethod"), FilterKernel::design_method_index(m_filterKernel.design_method().name()));
    settings.setValue(m_sSettingsPath + QString("/FilterDesignView/filterTransition"), m_filterKernel.parks_width() * (m_filterKernel.sampling_frequency() / 2));
    settings.setValue(m_sSettingsPath + QString("/FilterDesignView/filterChannelType"), getChannelType());
    settings.setValue(m_sSettingsPath + QString("/FilterDesignView/Position"), this->pos());
}

//=============================================================================================================

void FilterDesignView::loadSettings()
{
    if (m_sSettingsPath.isEmpty()) {
        return;
    }

    QSettings settings("MNECPP");

    //Set stored filter settings from last session
    m_pUi->m_doubleSpinBox_to->setValue(settings.value(m_sSettingsPath + QString("/FilterDesignView/filterTo"), 40.0).toDouble());
    m_pUi->m_doubleSpinBox_from->setValue(settings.value(m_sSettingsPath + QString("/FilterDesignView/filterFrom"), 1.0).toDouble());
    m_pUi->m_spinBox_filterTaps->setValue(settings.value(m_sSettingsPath + QString("/FilterDesignView/filterOrder"), 128).toInt());
    m_pUi->m_comboBox_designMethod->setCurrentIndex(settings.value(m_sSettingsPath + QString("/FilterDesignView/filterDesignMethod"), FilterKernel::design_method_index("Cosine")).toInt());
    m_pUi->m_doubleSpinBox_transitionband->setValue(settings.value(m_sSettingsPath + QString("/FilterDesignView/filterTransition"), 0.1).toDouble());
    m_pUi->m_comboBox_filterApplyTo->setCurrentText(settings.value(m_sSettingsPath + QString("/FilterDesignView/filterChannelType"), "All").toString());

    QPoint pos = settings.value(m_sSettingsPath + QString("/FilterDesignView/Position"), QPoint(100, 100)).toPoint();

    QRect screenRect = QApplication::primaryScreen()->geometry();
    if (!screenRect.contains(pos) && QGuiApplication::screens().size() == 1) {
        move(QPoint(100, 100));
    } else {
        move(pos);
    }
}

//=============================================================================================================

void FilterDesignView::updateGuiMode(GuiMode mode)
{
    switch (mode) {
        case GuiMode::Clinical:
            break;
        default: // default is research mode
            break;
    }
}

//=============================================================================================================

void FilterDesignView::updateProcessingMode(ProcessingMode mode)
{
    switch (mode) {
        case ProcessingMode::Offline:
            break;
        default: // default is realtime mode
            break;
    }
}

//=============================================================================================================

void FilterDesignView::initSpinBoxes()
{
    connect(m_pUi->m_doubleSpinBox_from, &QDoubleSpinBox::editingFinished,
            this, &FilterDesignView::filterParametersChanged);

    connect(m_pUi->m_doubleSpinBox_to, &QDoubleSpinBox::editingFinished,
            this, &FilterDesignView::filterParametersChanged);

    connect(m_pUi->m_doubleSpinBox_transitionband, &QDoubleSpinBox::editingFinished,
            this, &FilterDesignView::filterParametersChanged);

    connect(m_pUi->m_spinBox_filterTaps, &QSpinBox::editingFinished,
            this, &FilterDesignView::filterParametersChanged);

    //Intercept events from the spin boxes to get control over key events
    m_pUi->m_doubleSpinBox_from->installEventFilter(this);
    m_pUi->m_doubleSpinBox_to->installEventFilter(this);
    m_pUi->m_doubleSpinBox_transitionband->installEventFilter(this);
}

//=============================================================================================================

void FilterDesignView::initButtons()
{
    connect(m_pUi->m_pushButton_exportPlot, &QPushButton::released,
            this, &FilterDesignView::onBtnExportFilterPlot);

    connect(m_pUi->m_pushButton_exportFilter, &QPushButton::released,
            this, &FilterDesignView::onBtnExportFilterCoefficients);

    connect(m_pUi->m_pushButton_loadFilter, &QPushButton::released,
            this, &FilterDesignView::onBtnLoadFilter);
}

//=============================================================================================================

void FilterDesignView::initComboBoxes()
{
    for (const FilterParameter& filterMethod : FilterKernel::design_methods) {
        m_pUi->m_comboBox_designMethod->addItem(QString::fromStdString(filterMethod.name()));
    }

    connect(m_pUi->m_comboBox_designMethod, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &FilterDesignView::changeStateSpinBoxes);

    //Initial selection is a bandpass and Cosine design method
    m_pUi->m_doubleSpinBox_from->setVisible(true);
    m_pUi->m_label_lowpass->setVisible(true);

    m_pUi->m_doubleSpinBox_to->setVisible(true);
    m_pUi->m_label_highpass->setVisible(true);

    m_pUi->m_spinBox_filterTaps->setVisible(true);
    m_pUi->m_label_filterTaps->setVisible(true);

    connect(m_pUi->m_comboBox_filterApplyTo, &QComboBox::currentTextChanged,
            this, &FilterDesignView::onSpinBoxFilterChannelType);
}

//=============================================================================================================

void FilterDesignView::initFilterPlot()
{
    updateFilterPlot();
}

//=============================================================================================================

void FilterDesignView::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return) {
        emit filterChannelTypeChanged(m_pUi->m_comboBox_filterApplyTo->currentText());
    }

    if ((event->modifiers() == Qt::ControlModifier && event->key() == Qt::Key_Z) || event->key() == Qt::Key_Delete) {
        emit filterChannelTypeChanged(m_pUi->m_comboBox_filterApplyTo->currentText());
    }
}

//=============================================================================================================

void FilterDesignView::updateFilterPlot()
{
    // The filter's own sampling frequency: a loaded filter can differ from the measurement's
    m_pUi->m_filterPlot->setFilter(m_filterKernel);
}

//=============================================================================================================

void FilterDesignView::changeStateSpinBoxes(int currentIndex)
{
    Q_UNUSED(currentIndex);

    //Change visibility of filter tap spin boxes depending on filter design method
    switch (m_pUi->m_comboBox_designMethod->currentIndex()) {
        case 0: //Cosine
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
    emit updateFilterFrom(m_pUi->m_doubleSpinBox_from->value());
    emit updateFilterTo(m_pUi->m_doubleSpinBox_to->value());

    //User defined filter parameters
    double from = m_pUi->m_doubleSpinBox_from->value();
    double to = m_pUi->m_doubleSpinBox_to->value();

    double trans_width = m_pUi->m_doubleSpinBox_transitionband->value();

    double bw = to - from;
    double center = from + bw / 2;

    double nyquistFrequency = m_dSFreq / 2;

    //Calculate the needed fft length
    m_iFilterTaps = m_pUi->m_spinBox_filterTaps->value();
    if (m_pUi->m_spinBox_filterTaps->value() % 2 != 0) {
        m_iFilterTaps--;
    }

    //set maximum and minimum for cut off frequency spin boxes
    m_pUi->m_doubleSpinBox_to->setMaximum(nyquistFrequency);
    m_pUi->m_doubleSpinBox_from->setMaximum(nyquistFrequency);
    m_pUi->m_doubleSpinBox_to->setMinimum(0);
    m_pUi->m_doubleSpinBox_from->setMinimum(0);

    if ((m_pUi->m_doubleSpinBox_to->value() < m_pUi->m_doubleSpinBox_from->value())) {
        m_pUi->m_doubleSpinBox_to->setValue(m_pUi->m_doubleSpinBox_from->value() + 1);
    }

    m_pUi->m_doubleSpinBox_to->setMinimum(m_pUi->m_doubleSpinBox_from->value());
    m_pUi->m_doubleSpinBox_from->setMaximum(m_pUi->m_doubleSpinBox_to->value());

    int iMethod = FilterKernel::design_method_index(m_pUi->m_comboBox_designMethod->currentText().toStdString());

    //Generate filters
    m_filterKernel = FilterKernel("Designed Filter",
                                  FilterKernel::filter_type_index("BPF"),
                                  m_iFilterTaps,
                                  center / nyquistFrequency,
                                  bw / nyquistFrequency,
                                  trans_width / nyquistFrequency,
                                  m_dSFreq,
                                  iMethod);

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

    if (!fileName.isEmpty()) {
        // The plot at its size on screen, as vector graphics or an image
        const viz::DisplayList figure = m_pUi->m_filterPlot->figure(m_pUi->m_filterPlot->size());
        if (fileName.contains(".svg")) {
            if (const auto written = viz::write_svg(figure, fileName.toStdU16String()); !written) {
                mne::Log::warning("{}", written.error().what());
            }
        }

        if (fileName.contains(".png")) {
            QImage image(m_pUi->m_filterPlot->size(), QImage::Format_ARGB32);
            image.fill(Qt::white);
            QPainter painter(&image);
            paintScene(painter, figure, m_pUi->m_filterPlot->font());
            painter.end();
            image.save(fileName);
        }
    }
}

//=============================================================================================================

void FilterDesignView::onBtnExportFilterCoefficients()
{
    //Generate appropriate name for the filter to be saved
    QString filtername;

    filtername = QString("%1_%2_%3_Fs%4").arg(QString::fromStdString(m_filterKernel.filter_type().name())).arg(static_cast<int>(m_filterKernel.highpass_freq())).arg(static_cast<int>(m_filterKernel.lowpass_freq())).arg(static_cast<int>(m_filterKernel.sampling_frequency()));

    //Do not pass m_filterKernel because this is most likely the User Defined filter which name should not change due to the filter model implementation. Hence use temporal copy of m_filterKernel.
    FilterKernel filterWriteTemp = m_filterKernel;
    filterWriteTemp.set_name(filtername.toStdString());

    QString fileName = QFileDialog::getSaveFileName(this,
                                                    "Save filter coefficients",
                                                    QString("%1/%2").arg(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation)).arg(filtername),
                                                    tr("Text file(*.txt)"));
    if (fileName.isEmpty()) {
        return;
    }
    if (const auto written = FilterIO::write_filter(path_from_utf8(fileName.toStdString()), filterWriteTemp); !written) {
        mne::Log::warning("{}", written.error().what());
    }
}

//=============================================================================================================

void FilterDesignView::onBtnLoadFilter()
{
    QString path = QFileDialog::getOpenFileName(this,
                                                QString("Load filter"),
                                                QString("./"),
                                                tr("txt files (*.txt)"));

    if (!path.isEmpty()) {
        //Replace old with new filter operator
        const auto filterLoadTemp = FilterIO::read_filter(path_from_utf8(path.toStdString()));
        if (!filterLoadTemp) {
            mne::Log::warning("{}", filterLoadTemp.error().what());
            return;
        }
        updateGuiFromFilter(*filterLoadTemp);

        updateFilterPlot();

        filterParametersChanged();

    } else {
        mne::Log::debug("Could not load filter.");
    }
}

//=============================================================================================================

void FilterDesignView::clearView()
{
}

//=============================================================================================================

double FilterDesignView::getFrom()
{
    return m_pUi->m_doubleSpinBox_from->value();
}

//=============================================================================================================

double FilterDesignView::getTo()
{
    return m_pUi->m_doubleSpinBox_to->value();
}

//=============================================================================================================

void FilterDesignView::updateGuiFromFilter(const mne::FilterKernel& filter)
{
    m_pUi->m_doubleSpinBox_from->setValue(filter.highpass_freq());
    m_pUi->m_doubleSpinBox_to->setValue(filter.lowpass_freq());
    m_pUi->m_spinBox_filterTaps->setValue(filter.filter_order());
    m_pUi->m_doubleSpinBox_transitionband->setValue(filter.parks_width() * (filter.sampling_frequency() / 2));

    m_pUi->m_comboBox_designMethod->setCurrentIndex(FilterKernel::design_method_index(filter.design_method().name()));
}

//=============================================================================================================

void FilterDesignView::guiStyleChanged(mne::viz::qt::AbstractView::StyleMode style)
{
    Q_UNUSED(style)
    updateFilterPlot();
}
