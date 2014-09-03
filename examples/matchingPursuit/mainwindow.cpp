//=============================================================================================================
/**
* @file     mainwindow.cpp
* @author   Martin Henfling <martin.henfling@tu-ilmenau.de>;
*           Daniel Knobl <daniel.knobl@tu-ilmenau.de>;
*           Sebastian Krause <sebastian.krause@tu-ilmenau.de>
* @version  1.0
* @date     July, 2014
*
* @section  LICENSE
*
* Copyright (C) 2014, Martin Henfling, Daniel Knobl and Sebastian Krause. All rights reserved.
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
* @brief    Implementation of MainWindow class.
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <iostream>
#include <vector>
#include <math.h>
#include <fiff/fiff.h>
#include <mne/mne.h>
#include <utils/mp/atom.h>
#include <utils/mp/adaptivemp.h>
#include <utils/mp/fixdictmp.h>
#include <disp/plot.h>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "math.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "editorwindow.h"
#include "ui_editorwindow.h"
#include "formulaeditor.h"
#include "ui_formulaeditor.h"
#include "enhancededitorwindow.h"
#include "ui_enhancededitorwindow.h"
#include "processdurationmessagebox.h"
#include "ui_processdurationmessagebox.h"
#include "treebaseddictwindow.h"
#include "ui_treebaseddictwindow.h"
#include "settingwindow.h"
#include "ui_settingwindow.h"

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QTableWidgetItem>
#include <QFileDialog>
#include <QHeaderView>
#include <QMessageBox>
#include <QMap>
#include <QtConcurrent>

#include "QtGui"

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNELIB;
using namespace UTILSLIB;
using namespace DISPLIB;

//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

bool _tbv_is_loading = false;
bool _auto_change = false;
bool _was_partialchecked = false;
bool _come_from_sample_count = false;
bool _come_from_from =false;

qreal _from = 47.005f;
qreal _to = 48.000f;
qreal _signal_energy = 0;
qreal _signal_maximum = 0;
qreal _signal_negative_scale = 0;
qreal _max_pos = 0;
qreal _max_neg = 0;
qreal _draw_factor = 0;
qint32 _sample_rate = 1;

QString _save_path = "";
QString _file_name = "";
QMap<qint32, bool> _select_channel_map;
QMap<qint32, bool> _select_atoms_map;

QList<QColor> _colors;
QList<QColor> _original_colors;
QList<GaborAtom> _adaptive_atom_list;
QList<FixDictAtom> _fix_dict_atom_list;

MatrixXd _datas;
MatrixXd _times;
MatrixXd _signal_matrix(0, 0);
MatrixXd _original_signal_matrix(0, 0);
MatrixXd _atom_sum_matrix(0, 0);
MatrixXd _residuum_matrix(0, 0);
MatrixXd _real_residuum_matrix(0, 0);

QTime _counter_time(0,0);
QTimer *_counter_timer = new QTimer();

QThread* mp_Thread;
AdaptiveMp *adaptive_Mp;
FixDictMp *fixDict_Mp ;

//*************************************************************************************************************
//=============================================================================================================
// MAIN
//=============================================================================================================


//*************************************************************************************************************************************
// constructor
MainWindow::MainWindow(QWidget *parent) :    QMainWindow(parent),    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    callGraphWindow = new GraphWindow();    
    callGraphWindow->setMinimumHeight(140);
    callGraphWindow->setMinimumWidth(500);
    callGraphWindow->setMaximumHeight(400);
    ui->l_Graph->addWidget(callGraphWindow);

    callAtomSumWindow = new AtomSumWindow();
    callAtomSumWindow->setMinimumHeight(140);
    callAtomSumWindow->setMinimumWidth(500);
    callAtomSumWindow->setMaximumHeight(400);
    ui->l_atoms->addWidget(callAtomSumWindow);

    callResidumWindow = new ResiduumWindow();
    callResidumWindow->setMinimumHeight(140);
    callResidumWindow->setMinimumWidth(500);
    callResidumWindow->setMaximumHeight(400);
    ui->l_res->addWidget(callResidumWindow);

    callYAxisWindow = new YAxisWindow();
    callYAxisWindow->setMinimumHeight(22);
    callYAxisWindow->setMinimumWidth(500);
    callYAxisWindow->setMaximumHeight(22);
    ui->l_YAxis->addWidget(callYAxisWindow);

    ui->progressBarCalc->setMinimum(0);         // set progressbar
    ui->progressBarCalc->setHidden(true);
    ui->splitter->setStretchFactor(1,4);    

    ui->lb_save_file->setHidden(true);
    ui->lb_from->setHidden(true);
    ui->dsb_from->setHidden(true);
    ui->lb_to->setHidden(true);
    ui->dsb_to->setHidden(true);
    ui->lb_samples->setHidden(true);
    ui->sb_sample_count->setHidden(true);
    ui->cb_all_select->setEnabled(false);

    // set result tableview
    ui->tbv_Results->setColumnCount(5);
    ui->tbv_Results->setHorizontalHeaderLabels(QString("energy\n[%];scale\n[sec];trans\n[sec];modu\n[Hz];phase\n[rad]").split(";"));
    ui->tbv_Results->setColumnWidth(0,55);
    ui->tbv_Results->setColumnWidth(1,45);
    ui->tbv_Results->setColumnWidth(2,40);
    ui->tbv_Results->setColumnWidth(3,40);
    ui->tbv_Results->setColumnWidth(4,40);    

    this->cb_model = new QStandardItemModel;
    connect(this->cb_model, SIGNAL(dataChanged ( const QModelIndex&, const QModelIndex&)), this, SLOT(cb_selection_changed(const QModelIndex&, const QModelIndex&)));
    connect(ui->tbv_Results->model(), SIGNAL(dataChanged ( const QModelIndex&, const QModelIndex&)), this, SLOT(tbv_selection_changed(const QModelIndex&, const QModelIndex&)));
    connect(_counter_timer, SIGNAL(timeout()), this, SLOT(on_time_out()));

    qRegisterMetaType<Eigen::MatrixXd>("MatrixXd");
    qRegisterMetaType<Eigen::VectorXd>("VectorXd");
    qRegisterMetaType<adaptive_atom_list>("adaptive_atom_list");
    qRegisterMetaType<fix_dict_atom_list>("fix_dict_atom_list");

    QDir dir("Matching-Pursuit-Toolbox");
    if(!dir.exists()) dir.mkdir(dir.absolutePath());

    fill_dict_combobox();

    QSettings settings;
    move(settings.value("pos", QPoint(200, 200)).toPoint());
    resize(settings.value("size", QSize(1050, 700)).toSize());
    this->restoreState(settings.value("window_state").toByteArray());
    ui->splitter->restoreState(settings.value("splitter_sizes").toByteArray());
}

//*************************************************************************************************************************************

MainWindow::~MainWindow()
{    
    delete ui;
}

//*************************************************************************************************************************************

void MainWindow::closeEvent(QCloseEvent * event)
{
    QSettings settings;
    if(!this->isMaximized())
    {
        settings.setValue("pos", pos());
        settings.setValue("size", size());
    }
    settings.setValue("splitter_sizes", ui->splitter->saveState());
    settings.setValue("window_state", this->saveState());
    settings.setValue("maximized", this->isMaximized());
    event->accept();
}

//*************************************************************************************************************************************

void MainWindow::fill_dict_combobox()
{    
    QDir dir("Matching-Pursuit-Toolbox");
    QStringList filterList;
    filterList.append("*.dict");
    QFileInfoList fileList =  dir.entryInfoList(filterList);

    ui->cb_Dicts->clear();
    for(int i = 0; i < fileList.length(); i++)
        ui->cb_Dicts->addItem(QIcon(":/images/icons/DictIcon.png"), fileList.at(i).baseName());
}

//*************************************************************************************************************************************

void MainWindow::open_file()
{
    QFileDialog* fileDia;    
    QString temp_file_name = fileDia->getOpenFileName(this, "Please select signal file.",QDir::currentPath(),"(*.fif *.txt)");
    if(temp_file_name.isNull()) return;

    _file_name = temp_file_name;
     this->cb_model->clear();
    this->cb_items.clear();

    ui->sb_sample_rate->setEnabled(true);

    QFile file(_file_name);
    if (!file.open(QIODevice::ReadOnly))
    {
        QMessageBox::warning(this, tr("Error"),
        tr("error: disable to open signal file."));
        return;
    }
    file.close();    
    _colors.clear();
    _colors.append(QColor(0, 0, 0));

    if(_file_name.endsWith(".fif", Qt::CaseInsensitive))
    {        
        ui->dsb_from->setValue(47.005f);
        ui->dsb_to->setValue(48.000f);
        _from = 47.005f;
        //read_fiff_ave(_file_name);
        read_fiff_file(_file_name);
        ui->lb_from->setHidden(false);
        ui->dsb_from->setHidden(false);
        ui->lb_to->setHidden(false);
        ui->dsb_to->setHidden(false);
        ui->lb_samples->setHidden(false);
        ui->sb_sample_count->setHidden(false);
        ui->sb_sample_count->setValue((ui->dsb_to->value() - ui->dsb_from->value()) * ui->sb_sample_rate->value());
    }
    else
    {
        _from = 0;
        _signal_matrix.resize(0,0);
        read_matlab_file(_file_name);
        ui->lb_from->setHidden(true);
        ui->dsb_from->setHidden(true);
        ui->lb_to->setHidden(true);
        ui->dsb_to->setHidden(true);
        ui->lb_samples->setHidden(true);
        ui->sb_sample_count->setHidden(true);
    }

    _original_signal_matrix.resize(_signal_matrix.rows(), _signal_matrix.cols());
    _original_signal_matrix = _signal_matrix;
    ui->tbv_Results->setRowCount(0);

    for(qint32 channels = 0; channels < _signal_matrix.cols(); channels++)
    {
        _colors.append(QColor::fromHsv(qrand() % 256, 255, 190));

        this->cb_item = new QStandardItem;

        this->cb_item->setText(QString("Channel %1").arg(channels));
        this->cb_item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
        this->cb_item->setData(Qt::Checked, Qt::CheckStateRole);
        this->cb_model->insertRow(channels, this->cb_item);
        this->cb_items.push_back(this->cb_item);
        _select_channel_map.insert(channels, true);
    }

    this->cb_item = new QStandardItem;

    this->cb_item->setText("alle Channels an/ab wählen");
    this->cb_item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
    this->cb_item->setData(Qt::Checked, Qt::CheckStateRole);
    this->cb_model->appendRow(this->cb_item);
    this->cb_items.push_back(this->cb_item);

    ui->cb_channels->setModel(this->cb_model);
    _original_colors = _colors;
    _atom_sum_matrix.resize(_signal_matrix.rows(), _signal_matrix.cols()); //resize
    _residuum_matrix.resize(_signal_matrix.rows(), _signal_matrix.cols()); //resize

    update();   
}

//*************************************************************************************************************************************

void MainWindow::cb_selection_changed(const QModelIndex& topLeft, const QModelIndex& bottomRight)
{
    QStandardItem* cb_item = this->cb_items[topLeft.row()];
    if(topLeft.row() == ui->cb_channels->count() - 1)
    {
        if(cb_item->checkState() == Qt::Checked)
        {
            for(qint32 i = 0; i < ui->cb_channels->count() - 1; i++)
                if(this->cb_items[i]->checkState() == Qt::Unchecked)
                    this->cb_items[i]->setData(Qt::Checked, Qt::CheckStateRole);
        }
        else
        {
            for(qint32 i = 0; i < ui->cb_channels->count() - 1; i++)
                if(this->cb_items[i]->checkState() == Qt::Checked)
                    this->cb_items[i]->setData(Qt::Unchecked, Qt::CheckStateRole);
        }
        return;
    }

    ui->tbv_Results->setRowCount(0);

    if(cb_item->checkState() == Qt::Unchecked)
        _select_channel_map[topLeft.row()] = false;
    else if(cb_item->checkState() == Qt::Checked)
        _select_channel_map[topLeft.row()] = true;

    qint32 size = 0;

    for(qint32 i = 0; i < _original_signal_matrix.cols(); i++)    
        if(_select_channel_map[i] == true)
            size++;


    _signal_matrix.resize(_original_signal_matrix.rows(), size);
    _atom_sum_matrix.resize(_original_signal_matrix.rows(), size);
    _residuum_matrix.resize(_original_signal_matrix.rows(), size);    

    _colors.clear();
    qint32 selected_chn = 0;

    for(qint32 channels = 0; channels < _original_signal_matrix.cols(); channels++)    
        if(_select_channel_map[channels] == true)
        {
            _colors.append(_original_colors.at(channels));
            _signal_matrix.col(selected_chn) = _original_signal_matrix.col(channels);            
            selected_chn++;
        }
    update();
}

//*************************************************************************************************************************************

void MainWindow::read_fiff_ave(QString file_name)
{
    QList<QIODevice*> t_listSampleFilesIn;
    t_listSampleFilesIn.append(new QFile(file_name));
    FiffIO p_fiffIO(t_listSampleFilesIn);

    //std::cout << p_fiffIO << std::endl;

    //Read raw data samples
    p_fiffIO.m_qlistRaw[0]->read_raw_segment_times(_datas, _times, _from, _to);

    ui->sb_sample_rate->setValue(600);//raw.info.sfreq);
    ui->sb_sample_rate->setEnabled(false);
    _sample_rate = 600; //ui->sb_sample_rate->value();

    qint32 cols = 5;
    if(_datas.cols() <= 5)   cols = _datas.cols();
    _signal_matrix.resize(_datas.cols(),cols);

    for(qint32 channels = 0; channels < cols; channels++)
        _signal_matrix.col(channels) = _datas.row(channels);
}

//*************************************************************************************************************************************

qint32 MainWindow::read_fiff_file(QString fileName)
{
    QFile t_fileRaw(fileName);    
    bool keep_comp = true;


    //   Setup for reading the raw data    
    FiffRawData raw(t_fileRaw);


    //   Set up pick list: MEG + STI 014 - bad channels
    QStringList include;
    include << "STI 014";
    bool want_meg   = true;
    bool want_eeg   = false;
    bool want_stim  = false;
    RowVectorXi picks = raw.info.pick_types(want_meg, want_eeg, want_stim, include, raw.info.bads);

    //   Read a data segment
    //   times output argument is optional
    if (!raw.read_raw_segment_times(_datas, _times, _from, _to, picks))
    {
       printf("Could not read raw segment.\n");
       return -1;
    }
    printf("Read %d samples.\n",(qint32)_datas.cols());

    ui->sb_sample_rate->setValue(raw.info.sfreq);    
    ui->sb_sample_rate->setEnabled(false);
    _sample_rate = ui->sb_sample_rate->value();

    //ToDo: read all channels, or only a few?!
    qint32 rows = 305;
    if(_datas.rows() <= rows)   rows = _datas.rows();
    _signal_matrix.resize(_datas.cols(),rows);

    for(qint32 channels = 0; channels < rows; channels++)
        _signal_matrix.col(channels) = _datas.row(channels);

    return 0;
}

//*************************************************************************************************************************************

void MainWindow::read_fiff_file_new(QString file_name)
{
    this->cb_model->clear();
    this->cb_items.clear();
    _colors.clear();
    _colors.append(QColor(0, 0, 0));

    read_fiff_file(file_name);

    _original_signal_matrix.resize(_signal_matrix.rows(), _signal_matrix.cols());
    _original_signal_matrix = _signal_matrix;
    ui->tbv_Results->setRowCount(0);

    for(qint32 channels = 0; channels < _signal_matrix.cols(); channels++)
    {
        _colors.append(QColor::fromHsv(qrand() % 256, 255, 190));

        this->cb_item = new QStandardItem;

        this->cb_item->setText(QString("Channel %1").arg(channels));
        this->cb_item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
        this->cb_item->setData(Qt::Checked, Qt::CheckStateRole);
        this->cb_model->insertRow(channels, this->cb_item);
        this->cb_items.push_back(this->cb_item);
        _select_channel_map.insert(channels, true);
    }
    ui->cb_channels->setModel(this->cb_model);
    _original_colors = _colors;
    _atom_sum_matrix.resize(_signal_matrix.rows(), _signal_matrix.cols()); //resize
    _residuum_matrix.resize(_signal_matrix.rows(), _signal_matrix.cols()); //resize

    update();
}

//*************************************************************************************************************************************

void MainWindow::read_matlab_file(QString fileName)
{
    QFile file(fileName);
    QString contents;
    QList<QString> strList;
    file.open(QIODevice::ReadOnly);
    while(!file.atEnd())
    {
        strList.append(file.readLine(0).constData());
    }
    int rowNumber = 0;
    _signal_matrix.resize(strList.length(), 1);
    file.close();
    file.open(QIODevice::ReadOnly);
    while(!file.atEnd())
    {
        contents = file.readLine(0).constData();

        bool isFloat;
        qreal value = contents.toFloat(&isFloat);
        if(!isFloat)
        {
            QString errorSignal = QString("The signal could not completly read. Line %1 from file %2 coud not be readed.").arg(rowNumber).arg(fileName);
            QMessageBox::warning(this, tr("error"),
            errorSignal);
            return;
        }
        _signal_matrix(rowNumber, 0) = value;
        rowNumber++;
    }


    file.close();
    _signal_energy = 0;
    for(qint32 i = 0; i < _signal_matrix.rows(); i++)
        _signal_energy += (_signal_matrix(i, 0) * _signal_matrix(i, 0));
}

//*************************************************************************************************************************************

void GraphWindow::paintEvent(QPaintEvent* event)
{
    paint_signal(_signal_matrix, this->size());
}

//*************************************************************************************************************************************

void GraphWindow::paint_signal(MatrixXd signalMatrix, QSize windowSize)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillRect(0,0,windowSize.width(),windowSize.height(),QBrush(Qt::white));


    if(signalMatrix.rows() > 0 && signalMatrix.cols() > 0)
    {
        qint32 borderMarginHeigth = 15;     // reduce paintspace in GraphWindow of borderMargin pixels
        qint32 borderMarginWidth = 5;       // reduce paintspace in GraphWindow of borderMargin pixels
        qint32 i = 0;
        qreal maxNeg = 0;                   // smalest signalvalue
        qreal maxPos = 0;                   // highest signalvalue
        qreal absMin = 0;                   // minimum of abs(maxNeg and maxPos)
        qint32 drawFactor = 0;              // shift factor for decimal places (linear)
        qint32 startDrawFactor = 1;         // shift factor for decimal places (exponential-base 10)
        qint32 decimalPlace = 0;            // decimal places for axis title
        QList<QPolygonF> polygons;          // points for drawing the signal
        MatrixXd internSignalMatrix = signalMatrix; // intern representation of y-axis values of the signal (for painting only)

        // paint window white
        painter.fillRect(0,0,windowSize.width(),windowSize.height(),QBrush(Qt::white));

        // find min and max of signal

        for(qint32 channels = 0; channels < _signal_matrix.cols(); channels++)
        {
            i = 0;
            while(i < signalMatrix.rows())
            {
                if(signalMatrix(i, channels) > maxPos)
                    maxPos = signalMatrix(i, channels);

                if(signalMatrix(i, channels) < maxNeg )
                    maxNeg = signalMatrix(i, channels);
                i++;
            }
        }

        if(maxPos > fabs(maxNeg)) absMin = maxNeg;        // find absolute minimum of (maxPos, maxNeg)
        else     absMin = maxPos;

        if(absMin != 0)                                   // absMin must not be zero
        {
            while(true)                                   // shift factor for decimal places?
            {
                if(fabs(absMin) < 1)                      // if absMin > 1 , no shift of decimal places nescesary
                {
                    absMin = absMin * 10;
                    drawFactor++;                         // shiftfactor counter
                }
                if(fabs(absMin) >= 1) break;
            }
        }
        _draw_factor = drawFactor;  // to globe draw_factor

        // shift of decimal places with drawFactor for all signalpoints and save to intern list
        while(drawFactor > 0)
        {

            for(qint32 channel = 0; channel < signalMatrix.cols(); channel++)
                for(qint32 sample = 0; sample < signalMatrix.rows(); sample++)
                    internSignalMatrix(sample, channel) *= 10;

            startDrawFactor = startDrawFactor * 10;
            decimalPlace++;
            maxPos = maxPos * 10;
            maxNeg = maxNeg * 10;
            drawFactor--;
        }

        _max_pos = maxPos;      // to globe max_pos
        _max_neg = maxNeg;      // to globe min_pos

        qreal maxmax;
        // absolute signalheight
        if(maxNeg <= 0)     maxmax = maxPos - maxNeg;
        else  maxmax = maxPos + maxNeg;

        _signal_maximum = maxmax;

        // scale axis title
        //qreal scaleXText = (qreal)signalMatrix.rows() / _sample_rate / (qreal)20;     // divide signallength
        qreal scaleYText = (qreal)maxmax / (qreal)10;
        qint32 negScale =  floor((maxNeg * 10 / maxmax)+0.5);
        _signal_negative_scale = negScale;
        //find lenght of text of y-axis for shift of y-axis to the right (so the text will stay readable and is not painted into the y-axis
        qint32 maxStrLenght = 55;

        while((windowSize.width() - maxStrLenght -borderMarginWidth) % 20)borderMarginWidth++;

        // scale signal
        qreal scaleX = ((qreal)(windowSize.width() - maxStrLenght - borderMarginWidth))/ (qreal)signalMatrix.rows();
        qreal scaleY = (qreal)(windowSize.height() - borderMarginHeigth) / (qreal)maxmax;

        //scale axis
        qreal scaleXAchse = (qreal)(windowSize.width() - maxStrLenght - borderMarginWidth) / (qreal)20;
        qreal scaleYAchse = (qreal)(windowSize.height() - borderMarginHeigth) / (qreal)10;

        // position of title of x-axis
        qint32 xAxisTextPos = 8;
        if(maxNeg == 0) xAxisTextPos = -10; // if signal only positiv: titles above axis

        i = 1;
        while(i <= 11)
        {
            qreal scaledYText = negScale * scaleYText / (qreal)startDrawFactor;           // scalevalue y-axis
            QString string  = QString::number(scaledYText, 'g', 3);                 // scalevalue as string

            if(negScale == 0)                                                       // x-Axis reached (y-value = 0)
            {
                // append scaled signalpoints
                for(qint32 channel = 0; channel < signalMatrix.cols(); channel++)   // over all Channels
                {
                    QPolygonF poly;
                    qint32 h = 0;
                    while(h < signalMatrix.rows())
                    {
                        poly.append(QPointF((h * scaleX) + maxStrLenght,  -((internSignalMatrix(h, channel) * scaleY + ((i - 1) * scaleYAchse)-(windowSize.height()) + borderMarginHeigth / 2))));
                        h++;
                    }
                    polygons.append(poly);
                }

                // paint x-axis
                qint32 j = 1;
                while(j <= 21)
                {
                    if(fmod(j, 4.0) == 0)
                    {
                        QPen pen(Qt::darkGray, 0.5, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);
                        painter.setPen(pen);
                        painter.drawLine(j * scaleXAchse + maxStrLenght, -(((i - 1) * scaleYAchse)-(windowSize.height() - borderMarginHeigth / 2 - windowSize.height())), j * scaleXAchse + maxStrLenght , -(((i - 1) * scaleYAchse)-(windowSize.height() - borderMarginHeigth / 2 + windowSize.height())));   // scalelines
                    }                   
                    QPen pen(Qt::black, 1, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);
                    painter.setPen(pen);
                    painter.drawLine(j * scaleXAchse + maxStrLenght, -(((i - 1) * scaleYAchse)-(windowSize.height() - borderMarginHeigth / 2 - 2)), j * scaleXAchse + maxStrLenght , -(((i - 1) * scaleYAchse)-(windowSize.height() - borderMarginHeigth / 2 + 2)));   // scalelines

                    j++;
                }
                painter.drawLine(maxStrLenght, -(((i - 1) * scaleYAchse)-(windowSize.height()) + borderMarginHeigth / 2), windowSize.width()-5, -(((i - 1) * scaleYAchse)-(windowSize.height()) + borderMarginHeigth / 2));
            }

            painter.drawText(3, -((i - 1) * scaleYAchse - windowSize.height()) - borderMarginHeigth/2 + 4, string);     // paint scalevalue y-axis
            painter.drawLine(maxStrLenght - 2, -(((i - 1) * scaleYAchse)-(windowSize.height()) + borderMarginHeigth / 2), maxStrLenght + 2, -(((i - 1) * scaleYAchse)-(windowSize.height()) + borderMarginHeigth / 2));  // scalelines y-axis
            i++;
            negScale++;
        }

        painter.drawLine(maxStrLenght, 2, maxStrLenght, windowSize.height() - 2);     // paint y-axis



        for(qint32 channel = 0; channel < signalMatrix.cols(); channel++)             // Butterfly
        {
            QPen pen(_colors.at(channel), 0.5, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);
            painter.setPen(pen);
            painter.drawPolyline(polygons.at(channel));                               // paint signal
        }
    }
    painter.end();    
}

//*************************************************************************************************************************************

 void AtomSumWindow::paintEvent(QPaintEvent* event)
{
   paint_atom_sum(_atom_sum_matrix, this->size(), _signal_maximum, _signal_negative_scale);
}

//*************************************************************************************************************************************

void AtomSumWindow::paint_atom_sum(MatrixXd atom_matrix, QSize windowSize, qreal signalMaximum, qreal signalNegativeMaximum)
{
    // paint window white
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillRect(0,0,windowSize.width(),windowSize.height(),QBrush(Qt::white));

    // can also checked of zerovector, then you paint no empty axis
    if(atom_matrix.rows() > 0 && atom_matrix.cols() > 0  && _signal_matrix.rows() > 0 && _signal_matrix.cols() > 0)
    {
        qint32 borderMarginHeigth = 15;                     // reduce paintspace in GraphWindow of borderMargin pixels
        qint32 borderMarginWidth = 5;                       // reduce paintspace in GraphWindow of borderMargin pixels
        qint32 i = 0;
        qreal maxNeg = _max_neg;                            // smalest signalvalue
        qint32 drawFactor = _draw_factor;                   // shift factor for decimal places (linear)
        qint32 startDrawFactor = 1;                         // shift factor for decimal places (exponential-base 10)
        qint32 decimalPlace = 0;                            // decimal places for axis title
        QList<QPolygonF> polygons;                          // points for drawing the signal
        MatrixXd internSignalMatrix = atom_matrix;          // intern representation of y-axis values of the signal (for painting only)


        while(drawFactor > 0)
        {
            for(qint32 channels = 0; channels < atom_matrix.cols(); channels++)
                for(qint32 sample = 0; sample < atom_matrix.rows(); sample++)
                    internSignalMatrix(sample, channels) *= 10;

            startDrawFactor = startDrawFactor * 10;
            decimalPlace++;

            drawFactor--;
        }


        // scale axis title
        //qreal scaleXText = (qreal)atom_matrix.rows() / (qreal)_sample_rate / (qreal)20;     // divide signallegnth
        qreal scaleYText = (qreal)signalMaximum / (qreal)10;

        //find lenght of text of y-axis for shift of y-axis to the right (so the text will stay readable and is not painted into the y-axis
        qint32 maxStrLenght = 55;


        while((windowSize.width() - maxStrLenght -borderMarginWidth) % 20)borderMarginWidth++;

        // scale signal
        qreal scaleX = ((qreal)(windowSize.width() - maxStrLenght - borderMarginWidth))/ (qreal)atom_matrix.rows();
        qreal scaleY = (qreal)(windowSize.height() - borderMarginHeigth) / (qreal)signalMaximum;

        //scale axis
        qreal scaleXAchse = (qreal)(windowSize.width() - maxStrLenght - borderMarginWidth) / (qreal)20;
        qreal scaleYAchse = (qreal)(windowSize.height() - borderMarginHeigth) / (qreal)10;

        // position of title of x-axis
        qint32 xAxisTextPos = 8;
        if(maxNeg == 0) xAxisTextPos = -10; // if signal only positiv: titles above axis

        i = 1;
        while(i <= 11)
        {
            QString string;

            qreal scaledYText = signalNegativeMaximum * scaleYText / (qreal)startDrawFactor;    // scala Y-axis
            string  = QString::number(scaledYText, 'g', 3);                                     // scala as string

            if(signalNegativeMaximum == 0)                                                      // x-Axis reached (y-value = 0)
            {
                // append scaled signalpoints
                for(qint32 channel = 0; channel < atom_matrix.cols(); channel++)                // over all Channels
                {
                    // append scaled signalpoints
                    QPolygonF poly;
                    qint32 h = 0;
                    while(h < atom_matrix.rows())
                    {
                        poly.append(QPointF((h * scaleX) + maxStrLenght,  -((internSignalMatrix(h, channel) * scaleY + ((i - 1) * scaleYAchse)-(windowSize.height()) + borderMarginHeigth / 2))));
                        h++;
                    }
                    polygons.append(poly);
                }                
                // paint x-axis
                qint32 j = 1;
                while(j <= 21)
                {
                    if(fmod(j, 4.0) == 0)
                    {
                        QPen pen(Qt::darkGray, 0.5, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);
                        painter.setPen(pen);
                        painter.drawLine(j * scaleXAchse + maxStrLenght, -(((i - 1) * scaleYAchse)-(windowSize.height() - borderMarginHeigth / 2 - windowSize.height())), j * scaleXAchse + maxStrLenght , -(((i - 1) * scaleYAchse)-(windowSize.height() - borderMarginHeigth / 2 + windowSize.height())));   // scalelines
                    }
                    QPen pen(Qt::black, 1, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);
                    painter.setPen(pen);
                    painter.drawLine(j * scaleXAchse + maxStrLenght, -(((i - 1) * scaleYAchse)-(windowSize.height() - borderMarginHeigth / 2 - 2)), j * scaleXAchse + maxStrLenght , -(((i - 1) * scaleYAchse)-(windowSize.height() - borderMarginHeigth / 2 + 2)));   // scalelines

                    j++;
                }
                painter.drawLine(maxStrLenght, -(((i - 1) * scaleYAchse)-(windowSize.height()) + borderMarginHeigth / 2), windowSize.width()-5, -(((i - 1) * scaleYAchse)-(windowSize.height()) + borderMarginHeigth / 2));
            }

            painter.drawText(3, -((i - 1) * scaleYAchse - windowSize.height()) - borderMarginHeigth/2 + 4, string);     // paint scalvalue Y-axis
            painter.drawLine(maxStrLenght - 2, -(((i - 1) * scaleYAchse)-(windowSize.height()) + borderMarginHeigth / 2), maxStrLenght + 2, -(((i - 1) * scaleYAchse)-(windowSize.height()) + borderMarginHeigth / 2));  // scalelines y-axis
            i++;
            signalNegativeMaximum++;
        }
        painter.drawLine(maxStrLenght, 2, maxStrLenght, windowSize.height() - 2);     // paint y-axis

        for(qint32 channel = 0; channel < atom_matrix.cols(); channel++)             // Butterfly
        {
            QPen pen(_colors.at(channel), 0.5, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);
            painter.setPen(pen);
            painter.drawPolyline(polygons.at(channel));                               // paint signal
        }
    }
    painter.end();
}

//*************************************************************************************************************************************

void ResiduumWindow::paintEvent(QPaintEvent* event)
{
   paint_residuum(_residuum_matrix, this->size(), _signal_maximum, _signal_negative_scale);
}

//*************************************************************************************************************************************

void ResiduumWindow::paint_residuum(MatrixXd residuum_matrix, QSize windowSize, qreal signalMaximum, qreal signalNegativeMaximum)
{
    // paint window white
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillRect(0,0,windowSize.width(),windowSize.height(),QBrush(Qt::white));

    if(residuum_matrix.rows() > 0 && residuum_matrix.cols() > 0 && _signal_matrix.rows() > 0 && _signal_matrix.cols() > 0)
    {
        qint32 borderMarginHeigth = 15;                 // reduce paintspace in GraphWindow of borderMargin pixels
        qint32 borderMarginWidth = 5;                   // reduce paintspace in GraphWindow of borderMargin pixels
        qint32 i = 0;
        qreal maxNeg = _max_neg;                        // smalest signalvalue vom AusgangsSignal
        qint32 drawFactor = _draw_factor;               // shift factor for decimal places (linear)
        qint32 startDrawFactor = 1;                     // shift factor for decimal places (exponential-base 10)
        qint32 decimalPlace = 0;                        // decimal places for axis title
        QList<QPolygonF> polygons;                      // points for drawing the signal
        MatrixXd internSignalVector = residuum_matrix;  // intern representation of y-axis values of the signal (for painting only)

        while(drawFactor > 0)
        {
            for(qint32 channels = 0; channels < residuum_matrix.cols(); channels++)
                for(qint32 sample = 0; sample < residuum_matrix.rows(); sample++)
                    internSignalVector(sample,channels) *= 10;

            startDrawFactor = startDrawFactor * 10;
            decimalPlace++;

            drawFactor--;
        }

        // scale axis title
        //qreal scaleXText = (qreal)residuum_matrix.rows() /  (qreal)_sample_rate / (qreal)20;     // divide signallegnth
        qreal scaleYText = (qreal)signalMaximum / (qreal)10;

        //find lenght of text of y-axis for shift of y-axis to the right (so the text will stay readable and is not painted into the y-axis
        qint32 maxStrLenght = 55;

        while((windowSize.width() - maxStrLenght -borderMarginWidth) % 20)borderMarginWidth++;

        // scale signal
        qreal scaleX = ((qreal)(windowSize.width() - maxStrLenght - borderMarginWidth))/ (qreal)residuum_matrix.rows();
        qreal scaleY = (qreal)(windowSize.height() - borderMarginHeigth) / (qreal)signalMaximum;

        //scale axis
        qreal scaleXAchse = (qreal)(windowSize.width() - maxStrLenght - borderMarginWidth) / (qreal)20;
        qreal scaleYAchse = (qreal)(windowSize.height() - borderMarginHeigth) / (qreal)10;

        // position of title of x-axis
        qint32 xAxisTextPos = 8;
        if(maxNeg == 0) xAxisTextPos = -10; // if signal only positiv: titles above axis

        i = 1;
        while(i <= 11)
        {            
            qreal scaledYText = signalNegativeMaximum * scaleYText / (qreal)startDrawFactor;        // scalevalue y-axis
            QString string  = QString::number(scaledYText, 'g', 3);                          // scalevalue as string

            if(signalNegativeMaximum == 0)                                                          // x-axis reached (y-value = 0)
            {
                // append scaled signalpoints
                for(qint32 channel = 0; channel < residuum_matrix.cols(); channel++)       // over all Channels
                {
                    // append scaled signalpoints
                    QPolygonF poly;                    
                    qint32 h = 0;
                    while(h < residuum_matrix.rows())
                    {
                        poly.append(QPointF((h * scaleX) + maxStrLenght,  -((internSignalVector(h, channel) * scaleY + ((i - 1) * scaleYAchse)-(windowSize.height()) + borderMarginHeigth / 2))));
                        h++;
                    }
                    polygons.append(poly);
                }                
                qint32 j = 1;
                while(j <= 21)
                {
                    if(fmod(j, 4.0) == 0)
                    {
                        QPen pen(Qt::darkGray, 0.5, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);
                        painter.setPen(pen);
                        painter.drawLine(j * scaleXAchse + maxStrLenght, -(((i - 1) * scaleYAchse)-(windowSize.height() - borderMarginHeigth / 2 - windowSize.height())), j * scaleXAchse + maxStrLenght , -(((i - 1) * scaleYAchse)-(windowSize.height() - borderMarginHeigth / 2 + windowSize.height())));   // scalelines
                    }                   
                    QPen pen(Qt::black, 1, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);
                    painter.setPen(pen);
                    painter.drawLine(j * scaleXAchse + maxStrLenght, -(((i - 1) * scaleYAchse)-(windowSize.height() - borderMarginHeigth / 2 - 2)), j * scaleXAchse + maxStrLenght , -(((i - 1) * scaleYAchse)-(windowSize.height() - borderMarginHeigth / 2 + 2)));   // scalelines

                    j++;
                }
                // paint x-axis               
                painter.drawLine(maxStrLenght, -(((i - 1) * scaleYAchse)-(windowSize.height()) + borderMarginHeigth / 2), windowSize.width()-5, -(((i - 1) * scaleYAchse)-(windowSize.height()) + borderMarginHeigth / 2));
            }

            painter.drawText(3, -((i - 1) * scaleYAchse - windowSize.height()) - borderMarginHeigth/2 + 4, string);     // paint scalevalue y-axis
            painter.drawLine(maxStrLenght - 2, -(((i - 1) * scaleYAchse)-(windowSize.height()) + borderMarginHeigth / 2), maxStrLenght + 2, -(((i - 1) * scaleYAchse)-(windowSize.height()) + borderMarginHeigth / 2));  // scalelines y-axis
            i++;
            signalNegativeMaximum++;
        }

        painter.drawLine(maxStrLenght, 2, maxStrLenght, windowSize.height() - 2);       // paint y-axis

        for(qint32 channel = 0; channel < residuum_matrix.cols(); channel++)            // Butterfly
        {
            QPen pen(_colors.at(channel), 0.5, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);
            painter.setPen(pen);
            painter.drawPolyline(polygons.at(channel));                               // paint signal
        }
    }
    painter.end();
}

//*************************************************************************************************************************************

void YAxisWindow::paintEvent(QPaintEvent* event)
{
   paint_axis(_signal_matrix, this->size());
}

//*************************************************************************************************************************************

void YAxisWindow::paint_axis(MatrixXd signalMatrix, QSize windowSize)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillRect(0,0,windowSize.width(),windowSize.height(),QBrush(Qt::white));

    if(signalMatrix.rows() > 0 && signalMatrix.cols() > 0)
    {
        qint32 borderMarginWidth = 15;
        while((windowSize.width() - 55 - borderMarginWidth) % 20)borderMarginWidth++;
        qreal scaleXText = (qreal)signalMatrix.rows() /  (qreal)_sample_rate / (qreal)20;     // divide signallegnth
        qreal scaleXAchse = (qreal)(windowSize.width() - 55 - borderMarginWidth) / (qreal)20;

        qint32 j = 0;
        while(j <= 21)
        {
            QString str;
            painter.drawText(j * scaleXAchse + 45, 20, str.append(QString::number(j * scaleXText + _from, 'f', 2)));  // scalevalue as string
            painter.drawLine(j * scaleXAchse + 55, 5 + 2, j * scaleXAchse + 55 , 5 - 2);                            // scalelines
            j++;
        }
        painter.drawText(5 , 20, "[sec]");  // unit
        painter.drawLine(5, 5, windowSize.width()-5, 5);  // paint y-line
    }
}

//*************************************************************************************************************************************

// starts MP-algorithm
void MainWindow::on_btt_Calc_clicked()
{
    TruncationCriterion criterion;

    if(ui->chb_Iterations->isChecked() && !ui->chb_ResEnergy->isChecked())
        criterion = Iterations;
    if(ui->chb_Iterations->isChecked() && ui->chb_ResEnergy->isChecked())
        criterion = Both;
    if(ui->chb_ResEnergy->isChecked() && !ui->chb_Iterations->isChecked())
        criterion = SignalEnergy;

    if(ui->btt_Calc->text()== "calculate")
    {
        ui->progressBarCalc->setValue(0);
        ui->progressBarCalc->setHidden(false);

        if(_signal_matrix.rows() == 0)
        {
            QString title = "Warning";
            QString text = "No signalfile found.";
            QMessageBox msgBox(QMessageBox::Warning, title, text, QMessageBox::Ok, this);
            msgBox.exec();
            return;
        }

        if(ui->chb_Iterations->checkState()  == Qt::Unchecked && ui->chb_ResEnergy->checkState() == Qt::Unchecked)
        {
            QString title = "Error";
            QString text = "No truncation criterion choose.";
            QMessageBox msgBox(QMessageBox::Warning, title, text, QMessageBox::Ok, this);
            msgBox.exec();
            return;
        }

        if(((ui->dsb_energy->value() <= 1 && ui->dsb_energy->isEnabled()) && (ui->sb_Iterations->value() >= 500 && ui->sb_Iterations->isEnabled())) || (ui->dsb_energy->value() <= 1 && ui->dsb_energy->isEnabled() && !ui->sb_Iterations->isEnabled()) || (ui->sb_Iterations->value() >= 500 && ui->sb_Iterations->isEnabled() && !ui->dsb_energy->isEnabled()) )
        {
            QSettings settings;
            if(settings.value("show_warnings", true).toBool())
            {
                processdurationmessagebox* msgBox = new processdurationmessagebox(this);
                msgBox->setModal(true);
                msgBox->exec();
                msgBox->close();
            }
        }

        ui->frame->setEnabled(false);
        ui->btt_OpenSignal->setEnabled(false);
        ui->btt_Calc->setText("cancel");
        ui->cb_channels->setEnabled(false);
        ui->cb_all_select->setEnabled(false);
        ui->dsb_from->setEnabled(false);
        ui->dsb_to->setEnabled(false);
        ui->sb_sample_count ->setEnabled(false);

        ui->tbv_Results->setRowCount(0);
        ui->lb_IterationsProgressValue->setText("0");
        ui->lb_RestEnergieResiduumValue->setText("0");
        _adaptive_atom_list.clear();
        _fix_dict_atom_list.clear();
        _residuum_matrix = _signal_matrix;
        _atom_sum_matrix = MatrixXd::Zero(_signal_matrix.rows(), _signal_matrix.cols());
        update();

        _counter_time = QTime(0,0);
        _counter_timer->setInterval(100);
        _counter_timer->start();

        if(ui->rb_OwnDictionary->isChecked())
        {
            ui->tbv_Results->setColumnCount(2);
            ui->tbv_Results->setHorizontalHeaderLabels(QString("energy\n[%];atom").split(";"));
            ui->tbv_Results->setColumnWidth(0,55);
            ui->tbv_Results->setColumnWidth(1,280);

            // ToDo: size from dict
            //QFile ownDict(QString("Matching-Pursuit-Toolbox/%1.dict").arg(ui->cb_Dicts->currentText()));
            //_atom_sum_matrix = MatrixXd::Zero(256,1);
            //QFuture<void> f1 = QtConcurrent::run(&mpCalc, ownDict, _signal_matrix.col(0), ui->sb_Iterations->value());
            //f1.waitForFinished();
            calc_fix_mp(QString("Matching-Pursuit-Toolbox/%1.dict").arg(ui->cb_Dicts->currentText()), _signal_matrix.col(0), criterion);
        }
        else if(ui->rb_adativMp->isChecked())
        {
            ui->tbv_Results->setColumnCount(5);
            ui->tbv_Results->setHorizontalHeaderLabels(QString("energy\n[%];scale\n[sec];trans\n[sec];modu\n[Hz];phase\n[rad]").split(";"));
            ui->tbv_Results->setColumnWidth(0,55);
            ui->tbv_Results->setColumnWidth(1,45);
            ui->tbv_Results->setColumnWidth(2,40);
            ui->tbv_Results->setColumnWidth(3,40);
            ui->tbv_Results->setColumnWidth(4,40);
            calc_adaptiv_mp(_signal_matrix, criterion);
        }
        /* //ToDo: splitter
        QList<qint32> sizes = ui->splitter->sizes();

        sizes.insert(0, ui->tbv_Results->size().width() + 10);
        ui->splitter->setSizes(sizes);
        */
    }

    //paused Thread
    else if(ui->btt_Calc->text() == "cancel")
    {
        //if(mp_Thread)
            mp_Thread->requestInterruption();

        //else if(fixDict_Mp_Thread)
        //    fixDict_Mp_Thread->requestInterruption();

        ui->btt_Calc->setText("wait...");
        //return;
    }

}

//*************************************************************************************************************

void MainWindow::on_time_out()
{
    _counter_time = _counter_time.addMSecs(100);
    ui->lb_timer->setText(_counter_time.toString("hh:mm:ss.zzz"));
    _counter_timer->start();
}

//*************************************************************************************************************

void MainWindow::recieve_result(qint32 current_iteration, qint32 max_iterations, qreal current_energy, qreal max_energy, MatrixXd residuum,
                                adaptive_atom_list adaptive_atom_res_list, fix_dict_atom_list fix_dict_atom_res_list)
{
    _tbv_is_loading = true;
    //_row_count++;

    qreal percent = ui->dsb_energy->value();
    qreal residuum_energy = 100 * (max_energy - current_energy) / max_energy;

    //remaining energy and iterations update
    ui->lb_IterationsProgressValue->setText(QString::number(current_iteration));
    ui->lb_RestEnergieResiduumValue->setText(QString::number(residuum_energy, 'f', 2) + "%");
    //current atoms list update
    if(fix_dict_atom_res_list.isEmpty())
    {
        GaborAtom temp_atom = adaptive_atom_res_list.last();
        qreal percent_atom_energy = 100 * temp_atom.energy / max_energy;
        qreal phase = temp_atom.phase_list.first();
        if(temp_atom.phase_list.first() > 2*PI)
            phase = temp_atom.phase_list.first() - 2*PI;

        QTableWidgetItem* atomEnergieItem = new QTableWidgetItem(QString::number(percent_atom_energy, 'f', 2));
        QTableWidgetItem* atomScaleItem = new QTableWidgetItem(QString::number(temp_atom.scale / ui->sb_sample_rate->value(), 'g', 3));
        QTableWidgetItem* atomTranslationItem = new QTableWidgetItem(QString::number(temp_atom.translation / qreal(ui->sb_sample_rate->value()) + _from, 'g', 4));
        QTableWidgetItem* atomModulationItem = new QTableWidgetItem(QString::number(temp_atom.modulation * _sample_rate / temp_atom.sample_count, 'g', 3));
        QTableWidgetItem* atomPhaseItem = new QTableWidgetItem(QString::number(phase, 'g', 3));

        atomEnergieItem->setFlags(Qt::ItemIsUserCheckable);
        atomScaleItem->setFlags(Qt::NoItemFlags);
        atomTranslationItem->setFlags(Qt::NoItemFlags);
        atomModulationItem->setFlags(Qt::NoItemFlags);
        atomPhaseItem->setFlags(Qt::NoItemFlags);
        atomEnergieItem->setCheckState(Qt::Checked);

        atomEnergieItem->setTextAlignment(0x0082);
        atomScaleItem->setTextAlignment(0x0082);
        atomTranslationItem->setTextAlignment(0x0082);
        atomModulationItem->setTextAlignment(0x0082);
        atomPhaseItem->setTextAlignment(0x0082);

        _adaptive_atom_list.append(temp_atom);
        qSort(_adaptive_atom_list.begin(),_adaptive_atom_list.end(), sort_Energie);
        qSort(adaptive_atom_res_list.begin(),adaptive_atom_res_list.end(), sort_Energie);

        //index = _adaptive_atom_list.indexOf(temp_atom);

        qint32 index = 0;
        while(index < adaptive_atom_res_list.length())
        {
            if(temp_atom.scale == adaptive_atom_res_list.at(index).scale
                    && temp_atom.modulation == adaptive_atom_res_list.at(index).modulation
                    && temp_atom.translation == adaptive_atom_res_list.at(index).translation
                    && temp_atom.energy == adaptive_atom_res_list.at(index).energy)
                break;
            index++;
        }

        ui->tbv_Results->insertRow(index);

        ui->tbv_Results->setItem(index, 0, atomEnergieItem);
        ui->tbv_Results->setItem(index, 1, atomScaleItem);
        ui->tbv_Results->setItem(index, 2, atomTranslationItem);
        ui->tbv_Results->setItem(index, 3, atomModulationItem);
        ui->tbv_Results->setItem(index, 4, atomPhaseItem);
    }
    else if(adaptive_atom_res_list.isEmpty())
    {
        FixDictAtom temp_atom = fix_dict_atom_res_list.last();
        ui->tbv_Results->setRowCount(fix_dict_atom_res_list.length());

        qreal percent_atom_energy = 100 * temp_atom.energy / max_energy;

        QTableWidgetItem* atom_energie_item = new QTableWidgetItem(QString::number(percent_atom_energy, 'f', 2));
        QTableWidgetItem* atom_name_item = new QTableWidgetItem(fix_dict_atom_res_list.last().atom_formula);


        atom_energie_item->setFlags(Qt::ItemIsUserCheckable);
        atom_name_item->setFlags(Qt::NoItemFlags);

        atom_energie_item->setCheckState(Qt::Checked);

        atom_energie_item->setTextAlignment(0x0082);
        atom_name_item->setTextAlignment(0x0082);

         _fix_dict_atom_list.append(temp_atom);

        ui->tbv_Results->setItem(ui->tbv_Results->rowCount()- 1, 0, atom_energie_item);
        ui->tbv_Results->setItem(ui->tbv_Results->rowCount() - 1, 1, atom_name_item);
    }


    //update residuum and atom sum for painting and later save to hdd
    _residuum_matrix = residuum;
    _atom_sum_matrix = _signal_matrix - _residuum_matrix;

    //progressbar update
    qint32 prgrsbar_adapt = 99;

    if(max_iterations > 1999 && current_iteration < 100)
             ui->progressBarCalc->setMaximum(100);
    if(ui->chb_ResEnergy->isChecked() && (current_iteration >= (prgrsbar_adapt)) && (max_energy - current_energy) > (0.01 * percent * max_energy))
        ui->progressBarCalc->setMaximum(current_iteration + 5);
    if(max_iterations < 1999)
        ui->progressBarCalc->setMaximum(max_iterations);

    ui->progressBarCalc->setValue(current_iteration);

    if(((current_iteration == max_iterations) || (max_energy - current_energy) < (0.01 * percent * max_energy))&&ui->chb_ResEnergy->isChecked())
        ui->progressBarCalc->setValue(ui->progressBarCalc->maximum());

    update();
    _tbv_is_loading = false;
}

//*************************************************************************************************************

void MainWindow::tbv_selection_changed(const QModelIndex& topLeft, const QModelIndex& bottomRight)
{
    bool all_selected = true;
    bool all_deselected = true;

    if(_tbv_is_loading) return;

    for(qint32 i = 0; i < ui->tbv_Results->rowCount() - 1; i++)     // last item is residuum
        if(ui->tbv_Results->item(i, 0)->checkState()) all_deselected = false;
        else all_selected = false;

    if(all_selected) ui->cb_all_select->setCheckState(Qt::Checked);
    else if(all_deselected) ui->cb_all_select->setCheckState(Qt::Unchecked);
    else ui->cb_all_select->setCheckState(Qt::PartiallyChecked);

    QTableWidgetItem* item = ui->tbv_Results->item(topLeft.row(), 0);
    if(topLeft.row() == ui->tbv_Results->rowCount() - 1)
    {
        if(item->checkState())
        {
            for(qint32 channels = 0; channels < _signal_matrix.cols(); channels++)            
                _atom_sum_matrix.col(channels) += _real_residuum_matrix.col(channels);            
        }
        else
        {
            for(qint32 channels = 0; channels < _signal_matrix.cols(); channels++)            
                _atom_sum_matrix.col(channels) -= _real_residuum_matrix.col(channels);            
        }
    }
    else
    {
        if(ui->tbv_Results->columnCount() > 2)
        {
            GaborAtom  atom = _adaptive_atom_list.at(topLeft.row());
            if(!_auto_change)
                _select_atoms_map[topLeft.row()] = item->checkState();

            if(item->checkState())
            {
                for(qint32 channels = 0; channels < _signal_matrix.cols(); channels++)
                {
                    _atom_sum_matrix.col(channels) += atom.max_scalar_list.at(channels) * atom.create_real(atom.sample_count, atom.scale, atom.translation, atom.modulation, atom.phase_list.at(channels));
                    _residuum_matrix.col(channels) -= atom.max_scalar_list.at(channels) * atom.create_real(atom.sample_count, atom.scale, atom.translation, atom.modulation, atom.phase_list.at(channels));
                }
            }
            else
            {
                for(qint32 channels = 0; channels < _signal_matrix.cols(); channels++)
                {
                    _atom_sum_matrix.col(channels) -= atom.max_scalar_list.at(channels) * atom.create_real(atom.sample_count, atom.scale, atom.translation, atom.modulation, atom.phase_list.at(channels));
                    _residuum_matrix.col(channels) += atom.max_scalar_list.at(channels) * atom.create_real(atom.sample_count, atom.scale, atom.translation, atom.modulation, atom.phase_list.at(channels));
                }
            }
        }
        else
        {
            FixDictAtom  atom = _fix_dict_atom_list.at(topLeft.row());
            if(!_auto_change)
                _select_atoms_map[topLeft.row()] = item->checkState();

            if(item->checkState())
            {
                for(qint32 channels = 0; channels < _signal_matrix.cols(); channels++)
                {
                    _atom_sum_matrix.col(channels) += atom.max_scalar_list.at(channels) * atom.vector_list.at(channels);
                    _residuum_matrix.col(channels) -= atom.max_scalar_list.at(channels) * atom.vector_list.at(channels);
                }
            }
            else
            {
                for(qint32 channels = 0; channels < _signal_matrix.cols(); channels++)
                {
                    _atom_sum_matrix.col(channels) -= atom.max_scalar_list.at(channels) * atom.vector_list.at(channels);
                    _residuum_matrix.col(channels) += atom.max_scalar_list.at(channels) * atom.vector_list.at(channels);
                }
            }
        }
    }
    update();
}

//*************************************************************************************************************

void MainWindow::calc_thread_finished()
{
    _tbv_is_loading = true;

    _counter_timer->stop();
    ui->frame->setEnabled(true);
    ui->btt_OpenSignal->setEnabled(true);

    ui->btt_Calc->setText("calculate");
    ui->cb_channels->setEnabled(true);
    ui->cb_all_select->setEnabled(true);
    ui->dsb_from->setEnabled(true);
    ui->dsb_to->setEnabled(true);
    ui->sb_sample_count ->setEnabled(true);

    for(qint32 col = 0; col < ui->tbv_Results->columnCount(); col++)
        for(qint32 row = 0; row < ui->tbv_Results->rowCount(); row++)
        {
            if(col == 0)
                ui->tbv_Results->item(row, col)->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
            else
                ui->tbv_Results->item(row, col)->setFlags(Qt::ItemIsEnabled);
        }

    _real_residuum_matrix = _residuum_matrix;

    for(qint32 i = 0; i < ui->tbv_Results->rowCount(); i++)
        _select_atoms_map.insert(i, true);

    ui->tbv_Results->setRowCount(ui->tbv_Results->rowCount() + 1);

    QTableWidgetItem* energy_item = new QTableWidgetItem(ui->lb_RestEnergieResiduumValue->text().remove('%'));
    energy_item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
    energy_item->setCheckState(Qt::Unchecked);
    energy_item->setTextAlignment(0x0082);

    QTableWidgetItem* residuum_item = new QTableWidgetItem("residuum");
    residuum_item->setFlags(Qt::ItemIsEnabled);
    residuum_item->setTextAlignment(Qt::AlignCenter);

    ui->tbv_Results->setItem(ui->tbv_Results->rowCount() - 1, 0, energy_item);
    ui->tbv_Results->setItem(ui->tbv_Results->rowCount() - 1, 1, residuum_item);
    ui->tbv_Results->setSpan(ui->tbv_Results->rowCount() - 1, 1, 1, 4);

    _tbv_is_loading = false;

    update();
}

//*************************************************************************************************************

void MainWindow::calc_adaptiv_mp(MatrixXd signal, TruncationCriterion criterion)
{
    adaptive_Mp = new AdaptiveMp();
    qreal res_energy = ui->dsb_energy->value();

    //threading
    mp_Thread = new QThread;
    adaptive_Mp->moveToThread(mp_Thread);

    connect(this, SIGNAL(send_input(MatrixXd, qint32, qreal, bool, bool, qint32, qreal, qreal, qreal, qreal)),
            adaptive_Mp, SLOT(recieve_input(MatrixXd, qint32, qreal, bool, bool, qint32, qreal, qreal, qreal, qreal)));
    connect(adaptive_Mp, SIGNAL(current_result(qint32, qint32, qreal, qreal, MatrixXd, adaptive_atom_list, fix_dict_atom_list)),
                 this, SLOT(recieve_result(qint32, qint32, qreal, qreal, MatrixXd, adaptive_atom_list, fix_dict_atom_list)));
    connect(adaptive_Mp, SIGNAL(finished_calc()), mp_Thread, SLOT(quit()));
    connect(adaptive_Mp, SIGNAL(finished_calc()), adaptive_Mp, SLOT(deleteLater()));
    connect(mp_Thread, SIGNAL(finished()), this, SLOT(calc_thread_finished()));
    connect(mp_Thread, SIGNAL(finished()), mp_Thread, SLOT(deleteLater()));

    QSettings settings;
    bool fixphase = settings.value("fixPhase", false).toBool();
    bool isBoost = settings.value("isBoost", true).toBool();
    qint32 iterations = settings.value("adaptive_iterations", 1E3).toInt();
    qreal reflection = settings.value("adaptive_reflection", 1.00).toDouble();
    qreal expansion = settings.value("adaptive_expansion", 0.20).toDouble();
    qreal contraction = settings.value("adaptive_contraction", 0.5).toDouble();
    qreal fullcontraction = settings.value("adaptive_fullcontraction", 0.50).toDouble();
    switch(criterion)
    {
        case Iterations:        
            emit send_input(signal, ui->sb_Iterations->value(), qreal(MININT32), fixphase, isBoost, iterations,
                            reflection, expansion, contraction, fullcontraction);
            mp_Thread->start();        
            break;

        case SignalEnergy:        
            emit send_input(signal, MAXINT32, res_energy, fixphase, isBoost, iterations,
                            reflection, expansion, contraction, fullcontraction);
            mp_Thread->start();        
            break;

        case Both:
            emit send_input(signal, ui->sb_Iterations->value(), res_energy,fixphase, isBoost, iterations,
                            reflection, expansion, contraction, fullcontraction);
            mp_Thread->start();        
            break;
    }       
}

//************************************************************************************************************************************

void MainWindow::calc_fix_mp(QString path, MatrixXd signal, TruncationCriterion criterion)
{
    fixDict_Mp = new FixDictMp();
    qreal res_energy = ui->dsb_energy->value();

    //threading
    mp_Thread = new QThread;
    fixDict_Mp->moveToThread(mp_Thread);    

    connect(this, SIGNAL(send_input_fix_dict(MatrixXd, qint32, qreal, QString)),
            fixDict_Mp, SLOT(recieve_input(MatrixXd, qint32, qreal, QString)));
    connect(fixDict_Mp, SIGNAL(current_result(qint32, qint32, qreal, qreal, MatrixXd, adaptive_atom_list, fix_dict_atom_list)),
                  this, SLOT(recieve_result(qint32, qint32, qreal, qreal, MatrixXd, adaptive_atom_list, fix_dict_atom_list)));
    connect(fixDict_Mp, SIGNAL(finished_calc()), mp_Thread, SLOT(quit()));
    connect(fixDict_Mp, SIGNAL(finished_calc()), fixDict_Mp, SLOT(deleteLater()));
    connect(mp_Thread, SIGNAL(finished()), this, SLOT(calc_thread_finished()));
    connect(mp_Thread, SIGNAL(finished()), mp_Thread, SLOT(deleteLater()));

    switch(criterion)
    {
        case Iterations:
            emit send_input_fix_dict(signal, ui->sb_Iterations->value(), qreal(MININT32),
                                     QString("Matching-Pursuit-Toolbox/%1.dict").arg(ui->cb_Dicts->currentText()));
            mp_Thread->start();
            break;

        case SignalEnergy:
            emit send_input_fix_dict(signal, MAXINT32, res_energy,
                                     QString("Matching-Pursuit-Toolbox/%1.dict").arg(ui->cb_Dicts->currentText()));
            mp_Thread->start();
            break;

        case Both:
            emit send_input_fix_dict(signal, ui->sb_Iterations->value(), res_energy,
                                     QString("Matching-Pursuit-Toolbox/%1.dict").arg(ui->cb_Dicts->currentText()));
            mp_Thread->start();
            break;
    }
}

//*****************************************************************************************************************

// Opens Dictionaryeditor
void MainWindow::on_actionW_rterbucheditor_triggered()
{        
    EditorWindow *editor_window = new EditorWindow(this);
    connect(editor_window, SIGNAL(dict_saved()), this, SLOT(on_dicts_saved()));

    editor_window->show();
}

//*****************************************************************************************************************

// opens advanced Dictionaryeditor
void MainWindow::on_actionErweiterter_W_rterbucheditor_triggered()
{
    Enhancededitorwindow *x = new Enhancededitorwindow();
    x->show();
}

//*****************************************************************************************************************

// opens formula editor
void MainWindow::on_actionAtomformeleditor_triggered()
{
    Formulaeditor *x = new Formulaeditor();
    x->show();
}

//*****************************************************************************************************************

// opens Filedialog for read signal (contextmenue)
void MainWindow::on_actionNeu_triggered()
{
    open_file();
}

//*****************************************************************************************************************

// opens Filedialog for read signal (button)
void MainWindow::on_btt_OpenSignal_clicked()
{
    open_file();
}

//*****************************************************************************************************************

void MainWindow::on_actionCreate_treebased_dictionary_triggered()
{
    TreebasedDictWindow *x = new TreebasedDictWindow();
    x->show();
}

//*****************************************************************************************************************

void MainWindow::on_sb_sample_rate_editingFinished()
{
    _sample_rate = ui->sb_sample_rate->value();
}

//*****************************************************************************************************************

void MainWindow::on_dsb_from_editingFinished()
{   
    _from = ui->dsb_from->value();
    read_fiff_file_new(_file_name);
}

//*****************************************************************************************************************

void MainWindow::on_dsb_to_editingFinished()
{   
    _to = ui->dsb_to->value();
    read_fiff_file_new(_file_name);
}

//*****************************************************************************************************************

void MainWindow::on_dsb_from_valueChanged(double arg1)
{
    _come_from_from = true;
    qreal var = (_to - arg1) * ui->sb_sample_rate->value();
    if(ui->dsb_to->value() <= arg1 || var < 64 || var > 4096)
        ui->dsb_from->setValue(_from);
    else
        ui->sb_sample_count->setValue(var);
    _come_from_from = false;
}

//*****************************************************************************************************************

void MainWindow::on_dsb_to_valueChanged(double arg1)
{
    qreal var  = (arg1 - _from) * ui->sb_sample_rate->value();
    if(ui->dsb_from->value() >= arg1 || var < 64 || var > 4096)
        ui->dsb_to->setValue(_to);

    if(!_come_from_sample_count)
        ui->sb_sample_count->setValue(var);
}

//*****************************************************************************************************************

void MainWindow::on_sb_sample_count_valueChanged(int arg1)
{
    _come_from_sample_count = true;
    if(!_come_from_from)
        ui->dsb_to->setValue( _from  + ((qreal)arg1 / (qreal)ui->sb_sample_rate->value()));
    _come_from_sample_count = false;
}

//*****************************************************************************************************************

void MainWindow::on_sb_sample_count_editingFinished()
{
    if(!_come_from_from)
    {
        _to = ui->dsb_to->value();
        read_fiff_file_new(_file_name);
    }
}

//*****************************************************************************************************************

void MainWindow::on_cb_all_select_clicked()
{
    if(_tbv_is_loading) return;

    if( ui->cb_all_select->checkState() == Qt::Unchecked && !_was_partialchecked)
    {
        ui->cb_all_select->setCheckState(Qt::PartiallyChecked);
        _was_partialchecked = true;
    }
    else if(ui->cb_all_select->checkState() == Qt::Checked && !_was_partialchecked)
    {
        ui->cb_all_select->setCheckState(Qt::Unchecked);
        _was_partialchecked = false;
    }

    _auto_change = true;

    if(ui->cb_all_select->checkState() == Qt::Checked)
        for(qint32 i = 0; i < ui->tbv_Results->rowCount() - 1; i++)     // last item is residuum
            ui->tbv_Results->item(i, 0)->setCheckState(Qt::Checked);
    else if(ui->cb_all_select->checkState() == Qt::Unchecked)
        for(qint32 i = 0; i < ui->tbv_Results->rowCount() - 1; i++)     // last item is residuum
            ui->tbv_Results->item(i, 0)->setCheckState(Qt::Unchecked);
    else
    {
        for(qint32 i = 0; i < ui->tbv_Results->rowCount() - 1; i++)     // last item is residuum
            if(_select_atoms_map[i] == true)
                ui->tbv_Results->item(i, 0)->setCheckState(Qt::Checked);
            else
                ui->tbv_Results->item(i, 0)->setCheckState(Qt::Unchecked);
    }


    bool all_selected = true;
    bool all_deselected = true;
    for(qint32 i = 0; i < ui->tbv_Results->rowCount() - 1; i++)         // last item is residuum
        if(ui->tbv_Results->item(i, 0)->checkState())
            all_deselected = false;
        else
            all_selected = false;

    if(all_selected)
        ui->cb_all_select->setCheckState(Qt::Checked);
    else if(all_deselected)
    {
        ui->cb_all_select->setCheckState(Qt::Unchecked);
        _was_partialchecked = true;
    }
    else ui->cb_all_select->setCheckState(Qt::PartiallyChecked);

    _auto_change = false;
}

//*****************************************************************************************************************

void MainWindow::on_actionSettings_triggered()
{
    settingwindow *set = new settingwindow();
    set->show();
}

//*****************************************************************************************************************

void MainWindow::on_dicts_saved()
{
    fill_dict_combobox();
}

//*****************************************************************************************************************

void MainWindow::on_actionSpeicher_triggered()
{
    if(_file_name.isEmpty())
    {
        QMessageBox::warning(this, tr("Error"),
        tr("error: No file for save."));
        return;
    }

    if(ui->tbv_Results->rowCount() == 0)
    {
        QMessageBox::warning(this, tr("Error"),
        tr("error: No results for save."));
        return;
    }

    if(_save_path.isEmpty())
    {
        QFileDialog* fileDia;
        QString save_name = "";
        QStringList saveList = _file_name.split('/').last().split('.').first().split('_');
        for(int i = 0; i < saveList.length(); i++)
        {
            if(i == saveList.length() - 1)
                save_name += "mp_" + saveList.at(i);
            else
                save_name += saveList.at(i) + "_";
        }
        _save_path = fileDia->getSaveFileName(this, "Save file as...", QDir::currentPath() + "/" + save_name,"(*.fif)");
        if(_save_path.isEmpty()) return;
    }
    save_fif_file();
}

//*****************************************************************************************************************

void MainWindow::on_actionSpeicher_unter_triggered()
{
    if(_file_name.isEmpty())
    {
        QMessageBox::warning(this, tr("Error"),
        tr("error: No file for save."));
        return;
    }

    if(ui->tbv_Results->rowCount() == 0)
    {
        QMessageBox::warning(this, tr("Error"),
        tr("error: No results for save."));
        return;
    }

    QFileDialog* fileDia;
    QString save_name = "";
    QStringList saveList = _file_name.split('/').last().split('.').first().split('_');
    for(int i = 0; i < saveList.length(); i++)
    {
        if(i == saveList.length() - 1)
            save_name += "mp_" + saveList.at(i);
        else
            save_name += saveList.at(i) + "_";
    }

    _save_path = fileDia->getSaveFileName(this, "Save file as...", QDir::currentPath() + "/" + save_name,"(*.fif)");
    if(_save_path.isEmpty()) return;
    else save_fif_file();
}

//*****************************************************************************************************************

void MainWindow::save_fif_file()
{
    //change ui
    ui->lb_timer->setHidden(true);
    ui->cb_all_select->setHidden(true);
    ui->lb_save_file->setHidden(false);


    QFile t_fileIn(_file_name);
    QFile t_fileOut(_save_path);

    //
    //   Setup for reading the raw data
    //
    FiffRawData raw(t_fileIn);

    //
    //   Set up pick list: MEG + STI 014 - bad channels
    //
    //
    QStringList include;
    include << "STI 014";
    bool want_meg   = true;
    bool want_eeg   = false;
    bool want_stim  = false;
    RowVectorXi picks = raw.info.pick_types(want_meg, want_eeg, want_stim, include, raw.info.bads);

    MatrixXd cals;
    FiffStream::SPtr outfid = Fiff::start_writing_raw(t_fileOut,raw.info, cals, picks);

    //
    //   Set up the reading parameters
    //
    fiff_int_t from = raw.first_samp;
    fiff_int_t to = raw.last_samp;
    float quantum_sec = 10.0f;//read and write in 10 sec junks
    fiff_int_t quantum = ceil(quantum_sec*raw.info.sfreq);  //   To read the whole file at once set quantum     = to - from + 1;

    //************************************************************************************

    //
    //   Read and write all the data
    //
    bool first_buffer = true;
    fiff_int_t first, last;
    MatrixXd data;
    MatrixXd times;
    qreal start_change = _from * raw.info.sfreq;    // start of change
    qreal end_change = _to * raw.info.sfreq + 1;    // end of change

    ui->progressBarCalc->setValue(0);
    ui->progressBarCalc->setMinimum(0);
    ui->progressBarCalc->setMaximum(to);

    // from 0 to start of change
    for(first = from; first < start_change; first+=quantum)
    {
        last = first+quantum-1;
        if (last > start_change)
        {
            last = start_change;
        }
        if (!raw.read_raw_segment(data ,times, first, last, picks))
        {
                printf("error during read_raw_segment\n");
                QMessageBox::warning(this, tr("Error"),
                tr("error: Save unsucessful."));
                return;
        }
        printf("Writing...");
        if (first_buffer)
        {
           if (first > 0)
               outfid->write_int(FIFF_FIRST_SAMPLE,&first);
           first_buffer = false;
        }
        outfid->write_raw_buffer(data,cals);
        printf("[done]\n");

        ui->progressBarCalc->setValue(first);
    }

    //************************************************************************************

    // from start of change to end of change
    if (!raw.read_raw_segment(data, times, start_change ,end_change,picks))
    {
            printf("error during read_raw_segment\n");
            QMessageBox::warning(this, tr("Error"),
            tr("error: Save unsucessful."));
            return;
    }

    qint32 index = 0;
    for(qint32 channels = 0; channels < data.rows(); channels++)
    {
        if(_select_channel_map[channels])
        {
            data.row(channels) =  _atom_sum_matrix.col(index)  ;
            index++;
        }
    }

    printf("Writing new data...");
    outfid->write_raw_buffer(data,cals);
    printf("[done]\n");

    //************************************************************************************

    // from end of change to end
    for(first = end_change; first < to; first+=quantum)
    {
        last = first+quantum-1;
        if (last > to)
        {
            last = to;
        }
        if (!raw.read_raw_segment(data,times,first,last, picks))
        {
                printf("error during read_raw_segment\n");
                QMessageBox::warning(this, tr("Error"),
                tr("error: Save unsucessful."));
                return;
        }
        printf("Writing...");
        outfid->write_raw_buffer(data,cals);
        printf("[done]\n");
        ui->progressBarCalc->setValue(first);
    }
    ui->progressBarCalc->setValue(to);

    printf("Writing...");
    outfid->write_raw_buffer(data,cals);
    printf("[done]\n");

    outfid->finish_writing_raw();
    printf("Finished\n");

    ui->lb_timer->setHidden(false);
    ui->cb_all_select->setHidden(false);
    ui->lb_save_file->setHidden(true);
}

//*****************************************************************************************************************

bool MainWindow::sort_Energie(const GaborAtom atom_1, const GaborAtom atom_2)
{
    return (atom_1.energy > atom_2.energy);
}


