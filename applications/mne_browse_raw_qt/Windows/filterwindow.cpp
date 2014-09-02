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
    QWidget(parent, Qt::Window),
    ui(new Ui::FilterWindowWidget),
    m_pMainWindow(static_cast<MainWindow*>(parent))
{
    ui->setupUi(this);

    initSpinBoxes();
    initButtons();
    initComboBoxes();
    initFilterPlot();

    m_iWindowSize = m_qSettings.value("RawModel/window_size").toInt();
    m_iFilterTaps = m_qSettings.value("RawModel/num_filter_taps").toInt();
}


//*************************************************************************************************************

FilterWindow::~FilterWindow()
{
    delete ui;
}


//*************************************************************************************************************

void FilterWindow::initSpinBoxes()
{
    connect(ui->m_doubleSpinBox_lowpass,static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
                this,&FilterWindow::changeFilterParamters);

    connect(ui->m_doubleSpinBox_highpass,static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
                this,&FilterWindow::changeFilterParamters);

    connect(ui->m_doubleSpinBox_transitionband,static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
                this,&FilterWindow::changeFilterParamters);
}


//*************************************************************************************************************

void FilterWindow::initButtons()
{
    connect(ui->m_pushButton_applyFilter,&QPushButton::clicked,
                this,&FilterWindow::applyFilterToAll);
}


//*************************************************************************************************************

void FilterWindow::initComboBoxes()
{
    connect(ui->m_comboBox_filterType,static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
                this,&FilterWindow::changeStateSpinBoxes);

    changeStateSpinBoxes(0);
}


//*************************************************************************************************************

void FilterWindow::initFilterPlot()
{
    ui->m_graphicsView_filterPlot->setScene(&m_graphicsScene);
    m_graphicsScene.addText("TEST");
}


//*************************************************************************************************************

void FilterWindow::changeStateSpinBoxes(int currentIndex)
{
    //Change visibility of sin boxes depending on filter type
    switch(currentIndex) {
        case 0: //Lowpass
            ui->m_doubleSpinBox_lowpass->setVisible(true);
            ui->m_label_lowpass->setVisible(true);
            ui->m_label_lowpass->setText("Cut-Off (Hz)");

            ui->m_doubleSpinBox_highpass->setVisible(false);
            ui->m_label_highpass->setVisible(false);
            ui->m_doubleSpinBox_highpass->setEnabled(false);
            break;

        case 1: //Highpass
            ui->m_doubleSpinBox_highpass->setVisible(true);
            ui->m_label_highpass->setVisible(true);
            ui->m_label_highpass->setText("Cut-Off (Hz)");

            ui->m_doubleSpinBox_lowpass->setVisible(false);
            ui->m_label_lowpass->setVisible(false);
            ui->m_doubleSpinBox_highpass->setEnabled(true);
            break;

        case 2: //Bandpass
            ui->m_doubleSpinBox_highpass->setVisible(true);
            ui->m_label_highpass->setVisible(true);
            ui->m_doubleSpinBox_lowpass->setVisible(true);
            ui->m_label_lowpass->setText("Cut-Off Low (Hz)");

            ui->m_label_lowpass->setVisible(true);
            ui->m_doubleSpinBox_lowpass->setEnabled(true);
            ui->m_doubleSpinBox_highpass->setEnabled(true);
            ui->m_label_highpass->setText("Cut-Off High (Hz)");
            break;
    }
}

//*************************************************************************************************************

void FilterWindow::changeFilterParamters()
{
    qDebug()<<"changeFilterParamters()";

    //User defined filter parameters
    double lowpassHz = ui->m_doubleSpinBox_lowpass->value();
    double highpassHz = ui->m_doubleSpinBox_highpass->value();
    double center = lowpassHz+highpassHz/2;
    double trans_width = ui->m_doubleSpinBox_transitionband->value();
    double bw = highpassHz/lowpassHz;
    double nyquist_freq = m_pMainWindow->m_pRawModel->m_fiffInfo.sfreq;

    QSharedPointer<MNEOperator> userDefinedFilterOperator;

    if(ui->m_comboBox_filterType->currentText() == "Lowpass") {
        userDefinedFilterOperator = QSharedPointer<MNEOperator>(
                   new FilterOperator("User defined",FilterOperator::LPF,m_iFilterTaps,lowpassHz/nyquist_freq,0.2,(double)trans_width/nyquist_freq,(m_iWindowSize+m_iFilterTaps)));
    }

    if(ui->m_comboBox_filterType->currentText() == "Highpass") {
        userDefinedFilterOperator = QSharedPointer<MNEOperator>(
                   new FilterOperator("User defined",FilterOperator::HPF,m_iFilterTaps,highpassHz/nyquist_freq,0.2,(double)trans_width/nyquist_freq,(m_iWindowSize+m_iFilterTaps)));
    }

    if(ui->m_comboBox_filterType->currentText() == "Bandpass") {
        userDefinedFilterOperator = QSharedPointer<MNEOperator>(
                   new FilterOperator("User defined (See 'Adjust/Filter')",FilterOperator::BPF,m_iFilterTaps,(double)center/nyquist_freq,(double)bw/nyquist_freq,(double)trans_width/nyquist_freq,(m_iWindowSize+m_iFilterTaps)));
    }

    //Replace old with new filter operator
    QMutableMapIterator<QString,QSharedPointer<MNEOperator> > it(m_pMainWindow->m_pRawModel->m_Operators);
    while(it.hasNext()) {
        it.next();
        if(it.key() == "User defined (See 'Adjust/Filter')") {
            it.setValue(userDefinedFilterOperator);
        }
    }

    //update filter plot
}


//*************************************************************************************************************

void FilterWindow::applyFilterToAll()
{
    QMutableMapIterator<QString,QSharedPointer<MNEOperator> > it(m_pMainWindow->m_pRawModel->m_Operators);

    while(it.hasNext()) {
        it.next();
        if(it.key() == "User defined (See 'Adjust/Filter')") {
            m_pMainWindow->m_pRawModel->applyOperator(QModelIndexList(),it.value());
        }
    }
}

