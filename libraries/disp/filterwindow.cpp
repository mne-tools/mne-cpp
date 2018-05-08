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

#include "ui_filterwindowwidget.h"
#include "filterwindow.h"

#include <iostream>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QGraphicsScene>
#include <QDate>
#include <QFileDialog>
#include <QStandardPaths>
#include <QKeyEvent>
#include <QSvgGenerator>


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

FilterWindow::FilterWindow(QWidget *parent, Qt::WindowFlags type)
: QWidget(parent, type)
, ui(new Ui::FilterWindowWidget)
, m_iWindowSize(4016)
, m_iFilterTaps(512)
, m_dSFreq(600)
{
    ui->setupUi(this);

    initCheckBoxes();
    initSpinBoxes();
    initButtons();
    initComboBoxes();
    initMVC();
    initFilters();
    initFilterPlot();
}


//*************************************************************************************************************

FilterWindow::~FilterWindow()
{
    delete ui;
}


//*************************************************************************************************************

void FilterWindow::init(double dSFreq)
{
    m_dSFreq = dSFreq;

    filterParametersChanged();

    //Init m_filterData with designed filter and add to model
    m_pFilterDataModel->addFilter(m_filterData);

    //Update min max of spin boxes to nyquist
    double samplingFrequency = m_dSFreq;
    double nyquistFrequency = samplingFrequency/2;

    ui->m_doubleSpinBox_highpass->setMaximum(nyquistFrequency);
    ui->m_doubleSpinBox_lowpass->setMaximum(nyquistFrequency);

    if(ui->m_doubleSpinBox_highpass->value()>m_dSFreq/2)
        ui->m_doubleSpinBox_highpass->setValue(m_dSFreq/2);

    if(ui->m_doubleSpinBox_lowpass->value()>m_dSFreq/2)
        ui->m_doubleSpinBox_lowpass->setValue(m_dSFreq/2);

    updateFilterPlot();
}


//*************************************************************************************************************

void FilterWindow::setWindowSize(int iWindowSize)
{
    m_iWindowSize = iWindowSize;

    //Only set even numbers -> otherwise cosine design method gives wrong results
    if(m_iWindowSize%2!=0)
        m_iWindowSize--;

    //Update filter depending on new window size
    filterParametersChanged();
}


//*************************************************************************************************************

void FilterWindow::setMaxFilterTaps(int iMaxNumberFilterTaps)
{
    if(iMaxNumberFilterTaps%2!=0)
        iMaxNumberFilterTaps--;

    if(iMaxNumberFilterTaps>512)
        iMaxNumberFilterTaps = 512;

    ui->m_spinBox_filterTaps->setMaximum(iMaxNumberFilterTaps);
    ui->m_spinBox_filterTaps->setMinimum(16);

    //Update filter depending on new window size
    filterParametersChanged();
}


//*************************************************************************************************************

void FilterWindow::setSamplingRate(double dSamplingRate)
{
    m_dSFreq = dSamplingRate;

    if(ui->m_doubleSpinBox_highpass->value()>m_dSFreq/2)
        ui->m_doubleSpinBox_highpass->setValue(m_dSFreq/2);

    if(ui->m_doubleSpinBox_lowpass->value()>m_dSFreq/2)
        ui->m_doubleSpinBox_lowpass->setValue(m_dSFreq/2);

    filterParametersChanged();
}


//*************************************************************************************************************

void FilterWindow::setFilterParameters(double hp, double lp, int order, int type, int designMethod, double transition, bool activateFilter, const QString &sChannelType)
{
    ui->m_doubleSpinBox_highpass->setValue(lp);
    ui->m_doubleSpinBox_lowpass->setValue(hp);
    ui->m_spinBox_filterTaps->setValue(order);

    if(type == 0)
        ui->m_comboBox_filterType->setCurrentText("Lowpass");
    if(type == 1)
        ui->m_comboBox_filterType->setCurrentText("Highpass");
    if(type == 2)
        ui->m_comboBox_filterType->setCurrentText("Bandpass");
    if(type == 3)
        ui->m_comboBox_filterType->setCurrentText("Notch");

    if(designMethod == 0)
        ui->m_comboBox_designMethod->setCurrentText("Tschebyscheff");
    if(designMethod == 1)
        ui->m_comboBox_designMethod->setCurrentText("Cosine");

    ui->m_doubleSpinBox_transitionband->setValue(transition);

    for(int i=0; i<m_lActivationCheckBoxList.size(); i++) {
        if(m_lActivationCheckBoxList.at(i)->text() == "Activate user designed filter")
            m_lActivationCheckBoxList.at(i)->setChecked(activateFilter);
    }

    ui->m_comboBox_filterApplyTo->setCurrentText(sChannelType);

    filterActivated(activateFilter);

    filterParametersChanged();
}


//*************************************************************************************************************

QList<FilterData> FilterWindow::getCurrentFilter()
{
    //Get active filters
    QList<FilterData> activeFilters = m_pFilterDataModel->data( m_pFilterDataModel->index(0,8), FilterDataModelRoles::GetActiveFilters).value<QList<FilterData> >();

    return activeFilters;
}


//*************************************************************************************************************

FilterData FilterWindow::getUserDesignedFilter()
{
    return m_filterData;
}


//*************************************************************************************************************

QList<QCheckBox*> FilterWindow::getActivationCheckBoxList()
{
    return m_lActivationCheckBoxList;
}


//*************************************************************************************************************

QString FilterWindow::getChannelType()
{
    return ui->m_comboBox_filterApplyTo->currentText();
}


//*************************************************************************************************************

bool FilterWindow::userDesignedFiltersIsActive()
{
    for(int i = 0; i < m_lActivationCheckBoxList.size(); i++) {
        if(m_lActivationCheckBoxList.at(i)->text() == "Activate user designed filter")
            return m_lActivationCheckBoxList.at(i)->isChecked();
    }

    return false;
}


//*************************************************************************************************************

void FilterWindow::initCheckBoxes()
{
}


//*************************************************************************************************************

void FilterWindow::initSpinBoxes()
{
    ui->m_doubleSpinBox_lowpass->setValue(5.0);
    ui->m_doubleSpinBox_highpass->setValue(50.0);
    ui->m_doubleSpinBox_transitionband->setValue(4.0);

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
    connect(ui->m_pushButton_exportPlot,&QPushButton::released,
                this,&FilterWindow::onBtnExportFilterPlot);

    connect(ui->m_pushButton_exportFilter,&QPushButton::released,
                this,&FilterWindow::onBtnExportFilterCoefficients);

    connect(ui->m_pushButton_loadFilter,&QPushButton::released,
                this,&FilterWindow::onBtnLoadFilter);
}


//*************************************************************************************************************

void FilterWindow::initComboBoxes()
{
    connect(ui->m_comboBox_designMethod,static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
                this,&FilterWindow::changeStateSpinBoxes);

    connect(ui->m_comboBox_filterType,static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
                this,&FilterWindow::changeStateSpinBoxes);

    //Initial selection is a bandpass and Cosine design method
    ui->m_doubleSpinBox_lowpass->setVisible(true);
    ui->m_label_lowpass->setVisible(true);

    ui->m_doubleSpinBox_highpass->setVisible(true);
    ui->m_label_highpass->setVisible(true);

    ui->m_spinBox_filterTaps->setVisible(true);
    ui->m_label_filterTaps->setVisible(true);

    connect(ui->m_comboBox_filterApplyTo, &QComboBox::currentTextChanged,
            this, &FilterWindow::onSpinBoxFilterChannelType);

    ui->m_comboBox_filterApplyTo->setCurrentIndex(1);
}


//*************************************************************************************************************

void FilterWindow::initFilterPlot()
{
    m_pFilterPlotScene = FilterPlotScene::SPtr::create(ui->m_graphicsView_filterPlot, this);

    ui->m_graphicsView_filterPlot->setScene(m_pFilterPlotScene.data());

    filterSelectionChanged(m_pFilterDataModel->index(m_pFilterDataModel->rowCount()-1,0), QModelIndex());
}


//*************************************************************************************************************

void FilterWindow::initMVC()
{
    m_pFilterDataModel = FilterDataModel::SPtr(new FilterDataModel(this));
    m_pFilterDataDelegate = FilterDataDelegate::SPtr(new FilterDataDelegate(this));

    //Unncomment this if zou want the tableview
    ui->m_tableView_filterDataView->hide();
//    ui->m_tableView_filterDataView->setModel(m_pFilterDataModel.data());
//    ui->m_tableView_filterDataView->setItemDelegate(m_pFilterDataDelegate.data());
//    ui->m_tableView_filterDataView->resizeColumnToContents(0);

//    //Only show the names of the filter and activity check boxes
//    ui->m_tableView_filterDataView->verticalHeader()->hide();
//    ui->m_tableView_filterDataView->hideColumn(2);
//    ui->m_tableView_filterDataView->hideColumn(3);
//    ui->m_tableView_filterDataView->hideColumn(4);
//    //ui->m_tableView_filterDataView->hideColumn(5);
//    //ui->m_tableView_filterDataView->hideColumn(6);
//    ui->m_tableView_filterDataView->hideColumn(7);
//    ui->m_tableView_filterDataView->hideColumn(8);
//    ui->m_tableView_filterDataView->hideColumn(9);

//    //Connect selection in in filter table view to handle user changing the filter and updating the filter plot scene
//    connect(ui->m_tableView_filterDataView->selectionModel(),&QItemSelectionModel::currentRowChanged,
//                this, &FilterWindow::filterSelectionChanged);

    //Connect filter data model to updateFilterActivationWidget
    connect(m_pFilterDataModel.data(),&FilterDataModel::dataChanged,
                this, &FilterWindow::updateDefaultFiltersActivation);
}


//*************************************************************************************************************

void FilterWindow::initFilters()
{
    //Init filter data model with all default filters located in the resource/general directory
//    m_lDefaultFilters << "NOTCH_60Hz_Fs1kHz"
//                   << "NOTCH_50Hz_Fs1kHz"
//                   << "BP_1Hz_70Hz_Fs1kHz"
//                   << "BP_1Hz_40Hz_Fs1kHz";

    for(int i = 0; i<m_lDefaultFilters.size(); i++) {
        FilterData tmpFilter;
        QString fileName = m_lDefaultFilters.at(i);
        fileName.append(".txt");
        QString path = QCoreApplication::applicationDirPath() + fileName.prepend("/resources/general/default_filters/");

        if(FilterIO::readFilter(path, tmpFilter))
            m_pFilterDataModel->addFilter(tmpFilter);
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
        emit applyFilter(ui->m_comboBox_filterApplyTo->currentText());

    if((event->modifiers() == Qt::ControlModifier && event->key() == Qt::Key_Z) || event->key() == Qt::Key_Delete)
        emit applyFilter(ui->m_comboBox_filterApplyTo->currentText());
}


//*************************************************************************************************************

bool FilterWindow::eventFilter(QObject *obj, QEvent *event)
{
    if(obj == ui->m_doubleSpinBox_highpass || obj == ui->m_doubleSpinBox_lowpass || obj == ui->m_doubleSpinBox_transitionband) {
        if (event->type() == QEvent::KeyPress) {
            QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);

            if((keyEvent->modifiers() == Qt::ControlModifier && keyEvent->key() == Qt::Key_Z)/* || keyEvent->key() == Qt::Key_Delete*/)
                emit applyFilter(ui->m_comboBox_filterApplyTo->currentText());
            else // standard event processing
                return QObject::eventFilter(obj, event);

            return true;
        } else {
            // standard event processing
            return QObject::eventFilter(obj, event);
        }
    }

    for(int i=0; i<m_lActivationCheckBoxList.size(); i++) {
        if(obj == m_lActivationCheckBoxList.at(i) ) {
            if (event->type() == QEvent::HoverEnter) {
                int filterModelRowIndex = -1;
                for(int z=0; z<m_pFilterDataModel->rowCount(); z++) {
                    QString checkBoxText = m_lActivationCheckBoxList.at(i)->text();
                    if(checkBoxText == "Activate user designed filter")
                        checkBoxText = "User Design";

                    if(m_pFilterDataModel->data(m_pFilterDataModel->index(z,1), FilterDataModelRoles::GetFilterName).toString() == checkBoxText)
                        filterModelRowIndex = z;
                }

                //Get filter from model and set as current filter
                QModelIndex index = m_pFilterDataModel->index(filterModelRowIndex, 7);
                m_filterData = m_pFilterDataModel->data(index, FilterDataModelRoles::GetFilter).value<FilterData>();
                updateFilterPlot();

                return true;
            } else {
                // standard event processing
                return QObject::eventFilter(obj, event);
            }
        }
    }

    return true;
}


//*************************************************************************************************************

void FilterWindow::updateDefaultFiltersActivation(const QModelIndex & topLeft, const QModelIndex & bottomRight, const QVector<int> & roles)
{
    Q_UNUSED(topLeft);
    Q_UNUSED(bottomRight);
    Q_UNUSED(roles);

    QList<FilterData> allFilters = m_pFilterDataModel->data(m_pFilterDataModel->index(0,9), FilterDataModelRoles::GetAllFilters).value<QList<FilterData> >();

    if(m_lActivationCheckBoxList.size()==allFilters.size())
        return;

    while(!ui->m_layout_defaultFilterActivation->isEmpty()) {
        ui->m_layout_defaultFilterActivation->removeItem(ui->m_layout_defaultFilterActivation->itemAt(0));
    }

    m_lActivationCheckBoxList.clear();

    for(int i = 0; i<allFilters.size(); i++) {
        //Check for user designed filter. This needs to be done because there only should be one filter in the model which holds the user designed filter.
        //Otherwise everytime a filter is designed a new filter would be added to this model -> too much storage consumption.
        if(allFilters.at(i).m_sName != "User Design") {
            QCheckBox *checkBox = new QCheckBox(allFilters.at(i).m_sName);
            connect(checkBox,&QCheckBox::toggled,
                        this,&FilterWindow::onChkBoxFilterActivation);

            checkBox->installEventFilter(this);

            m_lActivationCheckBoxList.append(checkBox);

            ui->m_layout_defaultFilterActivation->addWidget(checkBox);
        } else {
            QCheckBox *checkBox = new QCheckBox("Activate user designed filter");
            connect(checkBox,&QCheckBox::toggled,
                        this,&FilterWindow::onChkBoxFilterActivation);

            checkBox->installEventFilter(this);

            m_lActivationCheckBoxList.prepend(checkBox);

            ui->m_layout_designFilter->addWidget(checkBox,6,0,1,2);
        }
    }

    emit activationCheckBoxListChanged(m_lActivationCheckBoxList);
}


//*************************************************************************************************************

void FilterWindow::updateFilterPlot()
{
    //Update the filter of the scene
    m_pFilterPlotScene->updateFilter(m_filterData,
                                     m_filterData.m_sFreq, //Pass the filters sampling frequency, not the one from the fiff info. Reason: sFreq from a loaded filter could be different
                                     ui->m_doubleSpinBox_lowpass->value(),
                                     ui->m_doubleSpinBox_highpass->value());

    ui->m_graphicsView_filterPlot->fitInView(m_pFilterPlotScene->itemsBoundingRect(), Qt::KeepAspectRatio);
}


//*************************************************************************************************************

void FilterWindow::changeStateSpinBoxes(int currentIndex)
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

void FilterWindow::filterParametersChanged()
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

//    std::cout<<"fftLength: "<<fftLength<<std::endl;
//    std::cout<<"m_iWindowSize: "<<m_iWindowSize<<std::endl;
//    std::cout<<"m_iWindowSize + ui->m_spinBox_filterTaps->value() * 4: "<<m_iWindowSize + ui->m_spinBox_filterTaps->value() * 4<<std::endl;

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
    if(ui->m_comboBox_designMethod->currentText() == "Tschebyscheff")
        dMethod = FilterData::Tschebyscheff;

    if(ui->m_comboBox_designMethod->currentText() == "Cosine")
        dMethod = FilterData::Cosine;

    //Generate filters
    QSharedPointer<FilterData> userDefinedFilterOperator;

    //Note: Always use "User Design" as filter name for user designed filters, which are stored in the model. This needs to be done because there only should be one filter in this model which holds the user designed filter.
    //Otherwise everytime a filter is designed a new filter would be added to this model -> too much storage consumption.
    if(ui->m_comboBox_filterType->currentText() == "Lowpass") {
        userDefinedFilterOperator = QSharedPointer<FilterData>(
                                                new FilterData("User Design",
                                                               FilterData::LPF,
                                                               m_iFilterTaps,
                                                               lowpassHz/nyquistFrequency,
                                                               0.2,
                                                               (double)trans_width/nyquistFrequency,
                                                               samplingFrequency,
                                                               fftLength,
                                                               dMethod));
    }

    if(ui->m_comboBox_filterType->currentText() == "Highpass") {
        userDefinedFilterOperator = QSharedPointer<FilterData>(
                                        new FilterData("User Design",
                                                        FilterData::HPF,
                                                        m_iFilterTaps,
                                                        highpassHz/nyquistFrequency,
                                                        0.2,
                                                        (double)trans_width/nyquistFrequency,
                                                        samplingFrequency,
                                                        fftLength,
                                                        dMethod));
    }

    if(ui->m_comboBox_filterType->currentText() == "Bandpass") {
        userDefinedFilterOperator = QSharedPointer<FilterData>(
                   new FilterData("User Design",
                                  FilterData::BPF,
                                  m_iFilterTaps,
                                  (double)center/nyquistFrequency,
                                  (double)bw/nyquistFrequency,
                                  (double)trans_width/nyquistFrequency,
                                  samplingFrequency,
                                  fftLength,
                                  dMethod));
    }

    //Replace old with new filter operator
    m_filterData = *userDefinedFilterOperator.data();

    //set user designed filter in filter data model
    QVariant variant;
    variant.setValue(m_filterData);

    m_pFilterDataModel->setData(m_pFilterDataModel->index(0,7), variant, FilterDataModelRoles::SetUserDesignedFilter);

    QList<FilterData> activeFilters = m_pFilterDataModel->data(m_pFilterDataModel->index(0,8), FilterDataModelRoles::GetActiveFilters).value<QList<FilterData> >();

    emit filterChanged(activeFilters);

    //update filter plot
    updateFilterPlot();
}


//*************************************************************************************************************

void FilterWindow::onSpinBoxFilterChannelType(QString channelType)
{
    //Apply filter
    emit applyFilter(channelType);
}


//*************************************************************************************************************

void FilterWindow::onBtnExportFilterPlot()
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

void FilterWindow::onBtnExportFilterCoefficients()
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

void FilterWindow::onBtnLoadFilter()
{
    QString path = QFileDialog::getOpenFileName(this,
                                                QString("Load filter"),
                                                QString("./"),
                                                tr("txt files (*.txt)"));

    if(!path.isEmpty()) {
        //Replace old with new filter operator
        FilterData filterLoadTemp;

        if(!FilterIO::readFilter(path, filterLoadTemp))
            return;

        m_pFilterDataModel->addFilter(filterLoadTemp);

        QList<FilterData> activeFilters = m_pFilterDataModel->data( m_pFilterDataModel->index(0,8), FilterDataModelRoles::GetActiveFilters).value<QList<FilterData> >();
        emit filterChanged(activeFilters);

        m_lDefaultFilters << filterLoadTemp.m_sName;

        updateFilterPlot();
    }
    else
        qDebug()<<"Could not load filter.";
}


//*************************************************************************************************************

void FilterWindow::onChkBoxFilterActivation(bool state)
{
    //Check default filters
    for(int i=0; i<m_lActivationCheckBoxList.size(); i++) {
        QVariant variant;
        variant.setValue(m_lActivationCheckBoxList.at(i)->isChecked());

        QString checkBoxText = m_lActivationCheckBoxList.at(i)->text();
        if(checkBoxText == "Activate user designed filter")
            checkBoxText = "User Design";

        int filterModelRowIndex = -1;
        for(int z=0; z<m_pFilterDataModel->rowCount(); z++) {
            if(m_pFilterDataModel->data( m_pFilterDataModel->index(z,1), FilterDataModelRoles::GetFilterName).toString() == checkBoxText)
                filterModelRowIndex = z;
        }

        if(filterModelRowIndex != -1)
            m_pFilterDataModel->setData(m_pFilterDataModel->index(filterModelRowIndex,0), variant, Qt::EditRole);
    }

    QList<FilterData> activeFilters = m_pFilterDataModel->data(m_pFilterDataModel->index(0,8), FilterDataModelRoles::GetActiveFilters).value<QList<FilterData> >();

    std::cout<<"activeFilters.size(): "<<activeFilters.size()<<std::endl;

    emit filterChanged(activeFilters);
    emit filterActivated(state);
}


//*************************************************************************************************************

void FilterWindow::filterSelectionChanged(const QModelIndex &current, const QModelIndex &previous)
{
    Q_UNUSED(previous);

    //Get filter from model and set as current filter
    QModelIndex index = m_pFilterDataModel->index(current.row(), 7);

    m_filterData = m_pFilterDataModel->data(index, FilterDataModelRoles::GetFilter).value<FilterData>();

    QList<FilterData> activeFilters = m_pFilterDataModel->data( m_pFilterDataModel->index(0,8), FilterDataModelRoles::GetActiveFilters).value<QList<FilterData> >();
    emit filterChanged(activeFilters);

    updateFilterPlot();
}



