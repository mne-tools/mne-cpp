//=============================================================================================================
/**
 * @file     mainwindow.cpp
 * @author   Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Gabriel B Motta <gabrielbenmotta@gmail.com>;
 *           Daniel Knobl <Daniel.Knobl@tu-ilmenau.de>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Martin Henfling <Martin.Henfling@tu-ilmenau.de>
 * @version  1.0
 * @date     July, 2014
 *
 * @section  LICENSE
 *
 * Copyright (C) 2014, Lars Debor, Christoph Dinh, Gabriel B Motta, Daniel Knobl, Lorenz Esch, 
 *                     Martin Henfling. All rights reserved.
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
 * @brief    Definition of MainWindow class.
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

#include "math.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"


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
#include <qtconcurrentrun.h>

#include <QtGui>
#include <QGraphicsTextItem>
#include <QGraphicsLineItem>

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

bool _new_paint;
bool _has_file;

fiff_int_t _from;
fiff_int_t _to;

fiff_int_t _first_sample;
fiff_int_t _last_sample;
fiff_int_t _press_pos;

qreal _offset_time;
qint32 _samplecount;
qreal _max_pos;
qreal _max_neg;
qreal _sample_rate;
qreal _signal_maximum;
qreal _signal_negative_scale;
qreal _border_margin_height;
qint32 _x_axis_height;

QList<QColor> _colors;
QStringList _matlab_channels;
MatrixXd _signal_matrix;
MatrixXd _atom_sum_matrix;
MatrixXd _residuum_matrix;
FiffEvoked _pick_evoked;

QTimer *_counter_timer;
QThread* mp_Thread;
AdaptiveMp *adaptive_Mp;
FixDictMp *fixDict_Mp ;
Formulaeditor *_formula_editor;
EditorWindow *_editor_window;
Enhancededitorwindow *_enhanced_editor_window;
settingwindow *_setting_window;
TreebasedDictWindow *_treebased_dict_window;
const QSize* psize = new QSize(10, 10);


//*************************************************************************************************************
//=============================================================================================================
// MAIN
//=============================================================================================================
MainWindow::MainWindow(QWidget *parent) :    QMainWindow(parent),    ui(new Ui::MainWindow)
{

    ui->setupUi(this);
    ui->tabWidget->setPalette(*(new QPalette(Qt::green)));
    ui->tabWidget->removeTab(1);
    ui->tabWidget->tabBar()->tabButton(0, QTabBar::LeftSide)->resize(0, 0);
    connect(ui->tabWidget, SIGNAL(tabCloseRequested(int)), this, SLOT(on_close_tab_button(int)));

    this->setMinimumSize(1280, 640);
    callGraphWindow = new GraphWindow();
    callGraphWindow->setMouseTracking(true);
    callGraphWindow->setMinimumHeight(140);
    callGraphWindow->setMinimumWidth(500);
    ui->l_Graph->addWidget(callGraphWindow);


    connect(callGraphWindow, SIGNAL(read_new()), this, SLOT(on_mouse_button_release()));

    callAtomSumWindow = new AtomSumWindow();
    callAtomSumWindow->setMouseTracking(true);
    callAtomSumWindow->setMinimumHeight(140);
    callAtomSumWindow->setMinimumWidth(500);
    ui->l_atoms->addWidget(callAtomSumWindow);

    callResidumWindow = new ResiduumWindow();
    callResidumWindow->setMouseTracking(true);
    callResidumWindow->setMinimumHeight(140);
    callResidumWindow->setMinimumWidth(500);
    ui->l_res->addWidget(callResidumWindow);

    callXAxisWindow = new XAxisWindow();
    callXAxisWindow->setMaximumHeight(0);
    callXAxisWindow->setToolTip("timeline");
    ui->l_XAxis->addWidget(callXAxisWindow);
    // set progressbar
    ui->progressBarCalc->setHidden(true);
    ui->progress_bar_save->setHidden(true);

    ui->splitter->setStretchFactor(1,4);

    ui->lb_from->setHidden(true);
    ui->dsb_from->setHidden(true);
    ui->lb_to->setHidden(true);
    ui->dsb_to->setHidden(true);
    ui->lb_samples->setHidden(true);
    ui->sb_sample_count->setHidden(true);
    ui->cb_all_select->setHidden(true);
    ui->lb_timer->setHidden(true);

    // set result tableview
    ui->tbv_Results->setColumnCount(5);
    ui->tbv_Results->setHorizontalHeaderLabels(QString("energy\n[%];scale\n[sec];trans\n[sec];modu\n[Hz];phase\n[rad]").split(";"));
    ui->tbv_Results->setColumnWidth(0,55);
    ui->tbv_Results->setColumnWidth(1,45);
    ui->tbv_Results->setColumnWidth(2,40);
    ui->tbv_Results->setColumnWidth(3,40);
    ui->tbv_Results->setColumnWidth(4,40);

    is_saved = true;
    is_calulating = false;
    _new_paint = true;
    _sample_rate = 1;
    _counter_timer = new QTimer();

    this->cb_model = new QStandardItemModel;
    connect(this->cb_model, SIGNAL(dataChanged ( const QModelIndex&, const QModelIndex&)), this, SLOT(cb_selection_changed(const QModelIndex&, const QModelIndex&)));
    connect(ui->tbv_Results->model(), SIGNAL(dataChanged ( const QModelIndex&, const QModelIndex&)), this, SLOT(tbv_selection_changed(const QModelIndex&, const QModelIndex&)));
    connect(_counter_timer, SIGNAL(timeout()), this, SLOT(on_time_out()));

    qRegisterMetaType<source_file_type>("source_file_type");
    qRegisterMetaType<Eigen::MatrixXd>("MatrixXd");
    qRegisterMetaType<Eigen::VectorXd>("VectorXd");
    qRegisterMetaType<Eigen::RowVectorXi>("RowVectorXi");
    qRegisterMetaType<adaptive_atom_list>("adaptive_atom_list");
    qRegisterMetaType<fix_dict_atom_list>("fix_dict_atom_list");
    qRegisterMetaType<FIFFLIB::fiff_int_t>("fiff_int_t");
    qRegisterMetaType<select_map>("select_map");

    QDir dir(QDir::homePath() + "/" + "Matching-Pursuit-Toolbox");
    if(!dir.exists())dir.mkdir(".");
    fill_dict_combobox();

    QSettings settings;
    move(settings.value("pos", QPoint(200, 200)).toPoint());
    resize(settings.value("size", QSize(1050, 700)).toSize());
    this->restoreState(settings.value("window_state").toByteArray());
    ui->splitter->restoreState(settings.value("splitter_sizes").toByteArray());
    last_open_path = settings.value("last_open_path", QDir::homePath()+ "/" + "Matching-Pursuit-Toolbox").toString();
    last_save_path = settings.value("last_save_path", QDir::homePath()+ "/" + "Matching-Pursuit-Toolbox").toString();

    //infolabel is invisible
    if(!settings.value("show_infos", true).toBool())
    {
        ui->lb_info_content->setHidden(true);
        ui->lb_info->setHidden(true);
        ui->lb_figure_of_merit->setHidden(true);
    }

    adaptive_Mp = Q_NULLPTR;
    fixDict_Mp = Q_NULLPTR;
    mp_Thread = Q_NULLPTR;
}

//*************************************************************************************************************************************

MainWindow::~MainWindow()
{    
    if(_editor_window != NULL)
        delete _editor_window;
    if(_enhanced_editor_window != NULL)
        delete _enhanced_editor_window;
    if(_setting_window != NULL)
        delete _setting_window;
    if(_formula_editor != NULL)
        delete _formula_editor;
    if(_treebased_dict_window != NULL)
        delete _treebased_dict_window;

    delete callAtomSumWindow;
    delete callGraphWindow;
    delete callResidumWindow;
    delete callXAxisWindow;

    delete this->cb_model;
    delete ui;
}

//*************************************************************************************************************************************

SaveFifFile::SaveFifFile(){}

//*************************************************************************************************************************************

SaveFifFile::~SaveFifFile(){}

//*************************************************************************************************************************************

void MainWindow::closeEvent(QCloseEvent * event)
{

    if(ui->progress_bar_save->isVisible())
    {
        if(QMessageBox::question(this, "warning", "Saving fif-file is still in progress. Are you sure you want to quit?") == QMessageBox::Yes)
           event->accept();
        else
        {
            event->ignore();
            return;
        }
    }

    if(is_calulating)
    {
        if(QMessageBox::question(this, "warning", "Calculation is still in progress. Are you sure you want to quit?") == QMessageBox::Yes)
           event->accept();
        else
        {
            event->ignore();
            return;
        }
    }

    QSettings settings;
    if(settings.value("show_warnings",true).toBool() && !is_saved)
    {
        QString text = "Warning, your changes have not been saved.\nTo close this window and discard your changes\nclick OK otherwise click Cancel and save the current changes.";
        DeleteMessageBox *msg_box = new DeleteMessageBox(text, "Warning...", "OK", "Cancel");
        msg_box->setModal(true);
        qint32 result = msg_box->exec();

        if(result == 0)
        {
            msg_box->close();
            event->ignore();
            return;
        }
        msg_box->close();
    }

    /* ToDo: call threadinterrupt
    if(mp_Thread != NULL)
        if(mp_Thread->isRunning())
            emit mp_Thread->requestInterruption();
    */

    if(!this->isMaximized())
    {
        settings.setValue("pos", pos());
        settings.setValue("size", size());
    }
    settings.setValue("splitter_sizes", ui->splitter->saveState());
    settings.setValue("window_state", this->saveState());
    settings.setValue("maximized", this->isMaximized());
    settings.setValue("last_open_path", last_open_path);
    settings.setValue("last_save_path", last_save_path);
    event->accept();
}

//*************************************************************************************************************************************

void MainWindow::open_file()
{
    QString temp_file_name = QFileDialog::getOpenFileName(this, "Please select signal file.", last_open_path,"(*.fif *.txt)");
    if(temp_file_name.isNull()) return;

    QStringList string_list = temp_file_name.split('/');
    last_open_path = "";
    for(qint32 i = 0; i < string_list.length() - 1; i++)
        last_open_path += string_list.at(i) + '/';
    file_name = temp_file_name;
    this->cb_model->clear();
    this->cb_items.clear();

    this->setWindowTitle(string_list.last().append(" - Matching-Pursuit-Toolbox"));//show current file in header of mainwindow

    ui->dsb_sample_rate->setEnabled(true);

    QFile file(file_name);
    if (!file.open(QIODevice::ReadOnly))
    {
        QMessageBox::warning(this, tr("Error"),
        tr("error: unable to open signal file."));
        this->setWindowTitle("Matching-Pursuit-Toolbox");
        return;
    }
    file.close();

    _colors.clear();
    _colors.append(QColor(0, 0, 0));
    _pick_evoked.clear();
    _offset_time = 0;
    _from = -1;
    if(file_name.endsWith(".fif", Qt::CaseInsensitive))
    {
        _has_file = true;

        if(!read_fiff_ave(file_name))
            if(!read_fiff_file(file_name))
            {
                this->setWindowTitle("Matching-Pursuit-Toolbox");
                _has_file = false;
                return;
            }

        ui->dsb_sample_rate->setEnabled(false);
    }
    else
    {
        _has_file = true;

        if(!read_matlab_file(file_name))
        {
            this->setWindowTitle("Matching-Pursuit-Toolbox");
            _has_file = false;
            return;
        }

        ui->dsb_sample_rate->setEnabled(true);
    }

    //initial
    original_signal_matrix = _signal_matrix;
    fill_channel_combobox();
    read_fiff_changed = true;

    last_from = _from;
    last_to = _to;
    last_sample_count = _to - _from + 1;

    ui->dsb_from->setMaximum((_last_sample - 63) / _sample_rate);
    ui->dsb_to->setMinimum((_first_sample + 63) / _sample_rate);
    ui->lb_from->setToolTip(QString("minimum: %1 seconds").arg(_first_sample / _sample_rate));
    ui->lb_to->setToolTip(QString("maximum: %1 seconds").arg(_last_sample / _sample_rate));
    ui->dsb_from->setToolTip(QString("sample: %1").arg(_from));
    ui->dsb_to->setToolTip(QString("sample: %1").arg(_to));
    ui->sb_sample_count->setToolTip(QString("epoch: %1 sec").arg((_to - _from + 1) / _sample_rate));
    ui->lb_samples->setToolTip(QString("min: 64 (%1 sec)\nmax: 4096 (%2 sec)").arg(64 / _sample_rate).arg(4096 / _sample_rate));

    ui->dsb_from->setValue((_from / _sample_rate) + _offset_time);
    ui->dsb_to->setValue(_to / _sample_rate + _offset_time);
    _samplecount = _to - _from + 1;
    ui->sb_sample_count->setValue(_to - _from + 1);

    read_fiff_changed = false;

    ui->lb_from->setHidden(false);
    ui->dsb_from->setHidden(false);
    ui->lb_to->setHidden(false);
    ui->dsb_to->setHidden(false);
    ui->lb_samples->setHidden(false);
    ui->sb_sample_count->setHidden(false);
    ui->tbv_Results->setRowCount(0);
    ui->lb_figure_of_merit->setHidden(true);
    callXAxisWindow->setMinimumHeight(22);
    callXAxisWindow->setMaximumHeight(22);
    ui->actionTFplot->setEnabled(false);


    _atom_sum_matrix = MatrixXd::Zero(_signal_matrix.rows(), _signal_matrix.cols()); //resize
    _residuum_matrix = MatrixXd::Zero(_signal_matrix.rows(), _signal_matrix.cols()); //resize

    ui->progressBarCalc->reset();
    ui->progressBarCalc->setVisible(false);
    ui->lb_signal_energy->clear();
    ui->lb_approx_energy->clear();
    ui->lb_residual_energy->clear();
    ui->lb_signal_energy_text->clear();
    ui->lb_approx_energy_text->clear();
    ui->lb_residual_energy_text->clear();
    ui->lb_info_content->clear();
    ui->cb_all_select->setHidden(true);
    ui->lb_timer->setHidden(true);
    ui->actionSpeicher->setEnabled(false);
    ui->actionSpeicher_unter->setEnabled(false);
    ui->actionExport->setEnabled(false);
    if(_signal_matrix.cols() == 0) ui->btt_Calc->setEnabled(false);
    else ui->btt_Calc->setEnabled(true);
    has_warning = false;

    _new_paint = true;

    update();
}

//*************************************************************************************************************************************

bool MainWindow::read_fiff_ave(QString file_name)
{
    QFile t_fileEvoked(file_name);

    fiff_int_t setno = 0;
    QPair<QVariant, QVariant> baseline(QVariant(), 0);
    FiffEvoked evoked(t_fileEvoked, setno, baseline);    
    if(evoked.isEmpty())
        return false;

    QSettings settings;
    QMap<QString, QVariant> chn_name_map;
    for(qint32 m = 0; m < 4; m++)
        chn_name_map.insert(QString("MEG;EEG;STI;EOG").split(';').at(m), true);
    chn_name_map = settings.value("channel_names", chn_name_map).toMap();

    QStringList pick_list;

    QMap<QString, QVariant>::const_iterator i;
    for (i = chn_name_map.constBegin(); i != chn_name_map.constEnd(); ++i)
        if(i.value().toBool())
            pick_list.append(i.key());

    //   Set up pick list: MEG + STI 014 - bad channels
    QStringList include;
    include << "STI 014";
    bool want_meg   = chn_name_map["MEG"].toBool();
    bool want_eeg   = chn_name_map["EEG"].toBool();
    bool want_stim  = chn_name_map["STI"].toBool();


    QStringList filter_list;
    for(qint32 i = 0; i < evoked.info.ch_names.length(); i++)
    {
        for(qint32 k = 0; k < pick_list.length(); k++)
            if(evoked.info.ch_names.at(i).contains(pick_list.at(k)))
                filter_list.append(evoked.info.ch_names.at(i));
    }

    _pick_evoked = evoked.pick_channels(filter_list);
    picks = _pick_evoked.info.pick_types(want_meg, want_eeg, want_stim);
    pick_info = _pick_evoked.info.pick_info();

    if(_pick_evoked.isEmpty())
        return false;

    ui->dsb_sample_rate->setValue(_pick_evoked.info.sfreq);
    _sample_rate = _pick_evoked.info.sfreq;


    _signal_matrix = MatrixXd::Zero(_pick_evoked.data.cols(), _pick_evoked.data.rows());

    for(qint32 channels = 0; channels <  _pick_evoked.data.rows(); channels++)
        _signal_matrix.col(channels) = _pick_evoked.data.row(channels);


    _offset_time = _pick_evoked.times[0];
    _from = 0;
    _first_sample = _pick_evoked.first;
    _to =  _signal_matrix.rows() - 1;
    _last_sample = _pick_evoked.last;

    reference_matrix = _signal_matrix;

    file_type = AVE;
    return true;
}

//*************************************************************************************************************************************

void MainWindow::read_fiff_ave_new()
{
    qint32 row_number = 0;
    qint32 selected_chn = 0;

    qint32 size = 0;
    for(qint32 i = 0; i < reference_matrix.cols(); i++)
        if(select_channel_map[i] == true)
            size++;

    _signal_matrix = MatrixXd::Zero(_to - _from, size);

    _colors.clear();
    for(qint32 channels = 0; channels < reference_matrix.cols(); channels++)
        if(select_channel_map[channels] == true)
        {
            row_number = 0;
            _colors.append(original_colors.at(channels));
            for(qint32 i = _from; i < _to; i++)
            {
                _signal_matrix(row_number, selected_chn) = reference_matrix(i, channels);
                row_number++;
            }
            selected_chn++;
        }

    //resize original signal matrix so that all channels are still in memory and only time is changed
    original_signal_matrix = MatrixXd::Zero(_signal_matrix.rows(), reference_matrix.cols());
    for(qint32 channels = 0; channels < reference_matrix.cols(); channels++)
    {
        row_number = 0;
        for(qint32 i = _from; i < _to; i++ )
        {
            original_signal_matrix(row_number, channels) = reference_matrix(i, channels);
            row_number++;
        }
    }

    _atom_sum_matrix = MatrixXd::Zero(_signal_matrix.rows(), _signal_matrix.cols()); //resize
    _residuum_matrix = MatrixXd::Zero(_signal_matrix.rows(), _signal_matrix.cols()); //resize

    ui->tbv_Results->clearContents();
    ui->tbv_Results->setRowCount(0);
    ui->actionSpeicher->setEnabled(false);
    ui->actionSpeicher_unter->setEnabled(false);
    ui->lb_info_content->clear();
    ui->cb_all_select->setHidden(true);
    ui->lb_timer->setHidden(true);
    ui->progressBarCalc->setHidden(true);
    ui->actionExport->setEnabled(false);
    ui->lb_figure_of_merit->setHidden(true);
    ui->lb_signal_energy->clear();
    ui->lb_approx_energy->clear();
    ui->lb_residual_energy->clear();
    ui->lb_signal_energy_text->clear();
    ui->lb_approx_energy_text->clear();
    ui->lb_residual_energy_text->clear();

    has_warning = false;
    _new_paint = true;
    update();
}

//*************************************************************************************************************************************

bool MainWindow::read_fiff_file(QString fileName)
{
    //   Setup for reading the raw data
    QFile t_fileRaw(fileName);
    FiffRawData raw(t_fileRaw);

    //save channel names to settings
    QSettings settings;
    QMap<QString, QVariant> chn_name_map;
    for(qint32 m = 0; m < 4; m++)
        chn_name_map.insert(QString("MEG;EEG;STI;EOG").split(';').at(m), true);

    chn_name_map = settings.value("channel_names", chn_name_map).toMap();
    QString next_name;

    for(qint32 k = 0; k < raw.info.ch_names.length(); k++)
    {
        bool found_no_new_name = false;
        next_name = raw.info.ch_names.at(k).split(" ").first();
        if(chn_name_map.contains(next_name))
        {
            found_no_new_name = true;
            break;
        }
        if(!found_no_new_name)
            chn_name_map.insert(next_name, true);
    }

    settings.setValue("channel_names", chn_name_map);

    //   Set up pick list: MEG + STI 014 - bad channels
    QStringList include;
    include << "STI 014";
    bool want_meg   = chn_name_map["MEG"].toBool();
    bool want_eeg   = chn_name_map["EEG"].toBool();
    bool want_stim  = chn_name_map["STI"].toBool();

    picks = raw.info.pick_types(want_meg, want_eeg, want_stim/*, include /*, raw.info.bads*/);

    //save fiff data borders global
    _first_sample = raw.first_samp;    
    _last_sample = raw.last_samp;    

    if(_from == -1)
    {
        _from = _first_sample;
        if(_from + 511 <= _last_sample) _to = _from + 511;
        else _to = _last_sample;
    }

    ui->dsb_sample_rate->setValue(raw.info.sfreq);    
    _sample_rate = raw.info.sfreq;

    pick_info = raw.info.pick_info(picks);
    //   Read a data segment
    //   times output argument is optional
    if (!raw.read_raw_segment(datas, times, _from, _to, picks))
    {
       QMessageBox::critical(this, "Error", "error reading fif-file. Could not read raw segment.");
       printf("Could not read raw segment.\n");
       return false;
    }
    printf("Read %d samples.\n",(qint32)datas.cols());

    //signal must be filled with datas like this, because data.col == signal.row and vice versa
    _signal_matrix = MatrixXd::Zero(datas.cols(),datas.rows());

    for(qint32 channels = 0; channels < datas.rows(); channels++)
        _signal_matrix.col(channels) = datas.row(channels);

    file_type = RAW;
    return true;
}

//*************************************************************************************************************************************

void MainWindow::read_fiff_file_new(QString file_name)
{  
    qint32 selected_chn = 0;
    read_fiff_file(file_name);
    original_signal_matrix = _signal_matrix;

    qint32 size = 0;
    for(qint32 i = 0; i < original_signal_matrix.cols(); i++)
        if(select_channel_map[i] == true)
            size++;

    _colors.clear();
    _signal_matrix = MatrixXd::Zero(original_signal_matrix.rows(), size);

    for(qint32 channels = 0; channels < original_signal_matrix.cols(); channels++)
        if(select_channel_map[channels] == true)
        {
            _colors.append(original_colors.at(channels));
            _signal_matrix.col(selected_chn) = original_signal_matrix.col(channels);
            selected_chn++;
        }

    _atom_sum_matrix = MatrixXd::Zero(_signal_matrix.rows(), _signal_matrix.cols()); //resize
    _residuum_matrix = MatrixXd::Zero(_signal_matrix.rows(), _signal_matrix.cols()); //resize

    ui->tbv_Results->clearContents();
    ui->tbv_Results->setRowCount(0);
    ui->actionSpeicher->setEnabled(false);
    ui->actionSpeicher_unter->setEnabled(false);
    ui->lb_info_content->clear();
    ui->cb_all_select->setHidden(true);    
    ui->lb_timer->setHidden(true);
    ui->progressBarCalc->setHidden(true);
    ui->actionExport->setEnabled(false);
    ui->lb_figure_of_merit->setHidden(true);
    ui->lb_signal_energy->clear();
    ui->lb_approx_energy->clear();
    ui->lb_residual_energy->clear();
    ui->lb_signal_energy_text->clear();
    ui->lb_approx_energy_text->clear();
    ui->lb_residual_energy_text->clear();

    has_warning = false;
    _new_paint = true;
    update();
}

//*************************************************************************************************************************************

bool MainWindow::read_matlab_file(QString fileName)
{
    QFile file(fileName);
    if(file.open(QIODevice::ReadOnly))
    {

        QTextStream stream(&file);
        bool isFloat;

        _matlab_channels = stream.readAll().split('\n', QString::SkipEmptyParts);

        for(qint32 k = 0; k < _matlab_channels.length(); k++)
        {
            qint32 row_number = 0;
            QStringList signal_samples = _matlab_channels.at(k).split(',', QString::SkipEmptyParts);

            if(k==0)
            {
                _first_sample = 0;
                _last_sample = signal_samples.length() - 1;
                _from = _first_sample;

                if(_from + 511 <= _last_sample)
                    _to = _from + 511;
                else
                    _to = _last_sample;

                _signal_matrix = MatrixXd::Zero(_to - _from + 1, _matlab_channels.length());
            }

            for(qint32 i = _from; i <= _to; i++)
            {
                qreal value = signal_samples.at(i).toFloat(&isFloat);
                if(!isFloat)
                {
                    _signal_matrix = MatrixXd::Zero(0, 0);
                    QMessageBox::critical(this, "error", QString("error reading matlab file. Could not read line %1 from file %2.").arg(i).arg(fileName));
                    //reset ui
                    ui->tbv_Results->clear();
                    ui->tbv_Results->clearContents();
                    ui->tbv_Results->setRowCount(0);
                    ui->tbv_Results->setColumnCount(0);
                    ui->btt_Calc->setDisabled(true);
                    //reset progressbar text color to black
                    pal.setColor(QPalette::Text, Qt::black);
                    ui->progressBarCalc->setPalette(pal);
                    ui->progressBarCalc->setFormat("residual energy: 100%            iterations: 0");
                    //reset infolabels
                    ui->lb_signal_energy->setHidden(true);
                    ui->lb_signal_energy_text->setHidden(true);
                    ui->lb_approx_energy->setHidden(true);
                    ui->lb_approx_energy_text->setHidden(true);
                    ui->lb_residual_energy->setHidden(true);
                    ui->lb_residual_energy_text->setHidden(true);

                    is_white = false;

                    is_saved = false;
                    has_warning = false;
                    return false;
                }
                _signal_matrix(row_number, k) = value;
                row_number++;
            }
        }
        file.close();
    }
    else
    {
        QMessageBox::warning(this, tr("Error"),
        tr("Unable to open matlab file."));
        return false;
    }
    _sample_rate = 1;
    ui->dsb_sample_rate->setValue(_sample_rate);

    file_type = TXT;

    return true;
}

//*************************************************************************************************************************************

void MainWindow::read_matlab_file_new()
{
    bool isFloat;
    qint32 selected_chn = 0;

    _signal_matrix = MatrixXd::Zero(_to - _from + 1, _matlab_channels.length());

    for(qint32 k = 0;k < _matlab_channels.length(); k++)
    {
        qint32 row_number = 0;
        QStringList signal_samples = _matlab_channels.at(k).split(',', QString::SkipEmptyParts);

        for(qint32 i = _from; i <= _to; i++)
        {
            qreal value = signal_samples.at(i).toFloat(&isFloat);
            if(!isFloat)
                _signal_matrix = MatrixXd::Zero(0, 0);

            _signal_matrix(row_number, k) = value;
            row_number++;
        }
    }

    original_signal_matrix = _signal_matrix;

    qint32 size = 0;
    for(qint32 i = 0; i < original_signal_matrix.cols(); i++)
        if(select_channel_map[i] == true)
            size++;

    _colors.clear();
    _signal_matrix = MatrixXd::Zero(original_signal_matrix.rows(), size);

    for(qint32 channels = 0; channels < original_signal_matrix.cols(); channels++)
        if(select_channel_map[channels] == true)
        {
            _colors.append(original_colors.at(channels));
            _signal_matrix.col(selected_chn) = original_signal_matrix.col(channels);
            selected_chn++;
        }


    _atom_sum_matrix = MatrixXd::Zero(_signal_matrix.rows(), _signal_matrix.cols()); //resize
    _residuum_matrix = MatrixXd::Zero(_signal_matrix.rows(), _signal_matrix.cols()); //resize

    ui->tbv_Results->clearContents();
    ui->tbv_Results->setRowCount(0);
    ui->actionSpeicher->setEnabled(false);
    ui->actionSpeicher_unter->setEnabled(false);
    ui->lb_info_content->clear();
    ui->cb_all_select->setHidden(true);
    ui->lb_timer->setHidden(true);
    ui->progressBarCalc->setHidden(true);
    ui->actionExport->setEnabled(false);
    ui->lb_figure_of_merit->setHidden(true);
    ui->lb_signal_energy->clear();
    ui->lb_approx_energy->clear();
    ui->lb_residual_energy->clear();
    ui->lb_signal_energy_text->clear();
    ui->lb_approx_energy_text->clear();
    ui->lb_residual_energy_text->clear();

    has_warning = false;
    _new_paint = true;
    update();
}
//*************************************************************************************************************************************

void MainWindow::fill_dict_combobox()
{
    QDir dir(QDir::homePath() + "/" + "Matching-Pursuit-Toolbox");
    QStringList filterList;
    filterList.append("*.dict");
    QFileInfoList fileList =  dir.entryInfoList(filterList);

    ui->cb_Dicts->clear();
    for(int i = 0; i < fileList.length(); i++)
        ui->cb_Dicts->addItem(QIcon(":/images/icons/DictIcon.png"), fileList.at(i).baseName());

    //dis-/enable button calc, if dicts are existing
    if(ui->cb_Dicts->itemText(0) == "" && ui->rb_OwnDictionary->isChecked())
        ui->btt_Calc->setEnabled(false);
    else if(_has_file && ui->rb_OwnDictionary->isChecked())
        ui->btt_Calc->setEnabled(true);

}

//*************************************************************************************************************************************

void MainWindow::fill_channel_combobox()
{
    //select all channels item
    this->cb_item = new QStandardItem;
    this->cb_item->setText("de/select all channels");
    this->cb_item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
    this->cb_item->setData(Qt::Checked, Qt::CheckStateRole);
    this->cb_model->appendRow(this->cb_item);
    this->cb_items.push_back(this->cb_item);

    QSettings settings;

    QStringList chn_names;
    chn_names.clear();
    if(!pick_info.isEmpty())
        chn_names = pick_info.ch_names;
    else
        for(qint32 i = 0; i < _signal_matrix.cols(); i++)
            chn_names.append(QString::number(i));



    //cout << "chn_name_lenght: " << chn_names.length() << "\n";
    //cout << "channelcount: " << _signal_matrix.cols() << "\n";
    for(qint32 channels = 1; channels <= _signal_matrix.cols(); channels++)
    {
        if(settings.value("pastell_colors", false).toBool())
            _colors.append(QColor::fromHsv(qrand() % 256, 255, 190));
        else
            _colors.append(QColor::fromRgb(qrand() / ((qreal)RAND_MAX + 300) * 255, qrand() / ((qreal)RAND_MAX + 300) * 255, qrand() / ((qreal)RAND_MAX + 300) * 255));
        //channel item
        this->cb_item = new QStandardItem;
        this->cb_item->setText(chn_names.at(channels - 1));
        this->cb_item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
        this->cb_item->setData(Qt::Checked, Qt::CheckStateRole);
        select_channel_map.insert(channels - 1, true);
        if(!pick_info.isEmpty())
        {
            for(qint32 k = 0; k < pick_info.bads.length(); k++)
                if(pick_info.bads.at(k) == this->cb_item->text())
                {
                   select_channel_map[channels - 1] = false;
                   this->cb_item->setData(Qt::Unchecked, Qt::CheckStateRole);
                   this->cb_item->setBackground(QColor::fromRgb(0x0F0, 0x080, 0x080, 0x00FF));
                   break;
                }
        }
        this->cb_items.push_back(this->cb_item);
        this->cb_model->insertRow(channels, this->cb_item);
    }

    original_colors = _colors;

    qint32 size = 0;
    for(qint32 i = 0; i < original_signal_matrix.cols(); i++)
        if(select_channel_map[i] == true)
            size++;


    _signal_matrix = MatrixXd::Zero(original_signal_matrix.rows(), size);
    _atom_sum_matrix = MatrixXd::Zero(original_signal_matrix.rows(), size);
    _residuum_matrix = MatrixXd::Zero(original_signal_matrix.rows(), size);

    _colors.clear();
    qint32 selected_chn = 0;

    for(qint32 channels = 0; channels < original_signal_matrix.cols(); channels++)
        if(select_channel_map[channels] == true)
        {
            _colors.append(original_colors.at(channels));
            _signal_matrix.col(selected_chn) = original_signal_matrix.col(channels);
            selected_chn++;
        }
    ui->cb_channels->setModel(this->cb_model);
}

//*************************************************************************************************************************************

void MainWindow::cb_selection_changed(const QModelIndex& topLeft, const QModelIndex& bottomRight)
{
    Q_UNUSED(bottomRight);

    QStandardItem* cb_item = this->cb_items[topLeft.row()];
    if(topLeft.row() == 0)
    {
        if(cb_item->checkState() == Qt::Checked)
        {
            for(qint32 i = 1; i < ui->cb_channels->count(); i++)
                if(this->cb_items[i]->checkState() == Qt::Unchecked)
                    this->cb_items[i]->setData(Qt::Checked, Qt::CheckStateRole);
        }
        else
        {
            for(qint32 i = 1; i < ui->cb_channels->count(); i++)
                if(this->cb_items[i]->checkState() == Qt::Checked)
                    this->cb_items[i]->setData(Qt::Unchecked, Qt::CheckStateRole);
        }
        return;
    }

    ui->tbv_Results->clearContents();
    ui->tbv_Results->setRowCount(0);
    ui->actionSpeicher->setEnabled(false);
    ui->actionSpeicher_unter->setEnabled(false);
    ui->actionExport->setEnabled(false);

    if(cb_item->checkState() == Qt::Unchecked)
        select_channel_map[topLeft.row() - 1] = false;
    else if(cb_item->checkState() == Qt::Checked)
        select_channel_map[topLeft.row() - 1] = true;

    qint32 size = 0;
    for(qint32 i = 0; i < original_signal_matrix.cols(); i++)
        if(select_channel_map[i] == true)
            size++;


    _signal_matrix = MatrixXd::Zero(original_signal_matrix.rows(), size);
    _atom_sum_matrix = MatrixXd::Zero(original_signal_matrix.rows(), size);
    _residuum_matrix = MatrixXd::Zero(original_signal_matrix.rows(), size);

    _colors.clear();
    qint32 selected_chn = 0;
    for(qint32 channels = 0; channels < original_signal_matrix.cols(); channels++)
        if(select_channel_map[channels] == true)
        {
            _colors.append(original_colors.at(channels));
            _signal_matrix.col(selected_chn) = original_signal_matrix.col(channels);
            selected_chn++;
        }

    if(_signal_matrix.cols() == 0) ui->btt_Calc->setEnabled(false);
    else ui->btt_Calc->setEnabled(true);

    _new_paint = true;
    update();
}

//*************************************************************************************************************************************

void GraphWindow::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    paint_signal(_signal_matrix, this->size());
}

//*************************************************************************************************************************************

void GraphWindow::paint_signal(MatrixXd signalMatrix, QSize windowSize)
{

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillRect(0,0,windowSize.width(),windowSize.height(),QBrush(Qt::white));     // paint window white
    painter.drawRect(0,0, windowSize.width(), windowSize.height());

    if(signalMatrix.rows() > 0 && signalMatrix.cols() > 0)
    {
        const qint32 maxStrLenght = 55; // max lenght in pixel of x-axis string
        qint32 borderMarginWidth = 15;  // reduce paintspace in GraphWindow of borderMargin pixels
        qreal maxPos = 0.0;             // highest signalvalue
        qreal maxNeg = 0.0;             // smalest signalvalue
        qreal maxmax = 0.0;             // absolute difference maxpos - maxneg
        qreal scaleYText = 0.0;
        qint32 negScale  = 0;

        _border_margin_height = 5 + windowSize.height() / 10;     // adapt bordermargin to avoid painting out of range

        if(_new_paint)
        {
            maxPos = signalMatrix.maxCoeff();
            maxNeg = signalMatrix.minCoeff();

            // absolute signalheight
            if(maxNeg <= 0) maxmax = maxPos - maxNeg;
            else  maxmax = maxPos + maxNeg;

            _max_pos = maxPos;          // to globe max_pos
            _max_neg = maxNeg;          // to globe min_pos
            _signal_maximum = maxmax;   // to globe abs_max
            _new_paint = false;
        }

        // scale axis text
        scaleYText = _signal_maximum / 10.0;

        if(_max_neg < 0)  negScale = floor((_max_neg * 10 / _signal_maximum) + 0.5);
        if(_max_pos <= 0) negScale = -10;
        _signal_negative_scale = negScale;  // to globe _signal_negative_scale

        // scale signal
        qreal scaleX = (windowSize.width() - maxStrLenght - borderMarginWidth) / qreal(signalMatrix.rows() - 1);
        qreal scaleY = (windowSize.height() - _border_margin_height) / _signal_maximum;

        //scale axis
        qreal scaleXAchse = (windowSize.width() - maxStrLenght - borderMarginWidth) / 20.0;
        qreal scaleYAchse = (windowSize.height() - _border_margin_height) / 10.0;

        for(qint32 i = 0; i < 11; i++)
        {
            if(negScale == 0)   // x-Axis reached (y-value = 0)
            {
                _x_axis_height = i * scaleYAchse - windowSize.height() + _border_margin_height / 2;   // position of x axis in height

                // append scaled signalpoints and paint signal
                for(qint32 channel = 0; channel < signalMatrix.cols(); channel++)   // over all Channels
                {
                    QPolygonF poly;
                    for(qint32 h = 0; h < signalMatrix.rows(); h++)
                        poly.append(QPointF((h * scaleX) + maxStrLenght, -(signalMatrix(h, channel) * scaleY + _x_axis_height)));
                    QPen pen(_colors.at(channel), 0.5, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);
                    painter.setPen(pen);
                    painter.drawPolyline(poly);                    
                }

                // paint x-axis
                for(qint32 j = 1; j < 21; j++)
                {
                    if(fmod(j, 4.0) == 0)   //draw light grey sectors
                    {
                        QPen pen(Qt::darkGray, 0.5, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);
                        painter.setPen(pen);
                        painter.drawLine(j * scaleXAchse + maxStrLenght, -(_x_axis_height - windowSize.height()),
                                         j * scaleXAchse + maxStrLenght , -(_x_axis_height + windowSize.height()));   // scalelines x-axis
                    }
                    QPen pen(Qt::black, 1, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);
                    painter.setPen(pen);
                    painter.drawLine(j * scaleXAchse + maxStrLenght, -(_x_axis_height - 2),
                                     j * scaleXAchse + maxStrLenght , -(_x_axis_height + 2));   // scalelines x-axis
                }
                painter.drawLine(maxStrLenght - 40, -_x_axis_height, windowSize.width()-5, -_x_axis_height);    // paint x-axis
            }
            painter.drawText(3, -(i * scaleYAchse - windowSize.height()) - _border_margin_height / 2 + 4, QString::number(negScale * scaleYText, 'g', 3));     // paint scalevalue y-axis

            painter.drawLine(maxStrLenght - 2, -((i * scaleYAchse)-(windowSize.height()) + _border_margin_height / 2),
                             maxStrLenght + 2, -((i * scaleYAchse)-(windowSize.height()) + _border_margin_height / 2));  // scalelines y-axis

            negScale++;
        }
        painter.drawLine(maxStrLenght, 2, maxStrLenght, windowSize.height() - 2);     // paint y-axis
    }
    painter.end();    
}

//*************************************************************************************************************************************

void AtomSumWindow::paintEvent(QPaintEvent* event)
{
     Q_UNUSED(event);
    paint_atom_sum(_atom_sum_matrix, this->size(), _signal_maximum, _signal_negative_scale);
}

//*************************************************************************************************************************************

void AtomSumWindow::paint_atom_sum(MatrixXd atom_matrix, QSize windowSize, qreal signalMaximum, qreal signalNegativeMaximum)
{
    // paint window white
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillRect(0,0,windowSize.width(),windowSize.height(),QBrush(Qt::white));
    painter.drawRect(0,0, windowSize.width(), windowSize.height());

    // can also checked of zerovector, then you paint no empty axis
    if(atom_matrix.rows() > 0 && atom_matrix.cols() > 0  && _signal_matrix.rows() > 0 && _signal_matrix.cols() > 0)
    {
        const qint32 maxStrLenght = 55;
        qint32 borderMarginWidth = 15;  // reduce paintspace in GraphWindow of borderMargin pixels

        // scale axis title
        qreal scaleYText = signalMaximum / 10.0;

        // scale signal
        qreal scaleX = (windowSize.width() - maxStrLenght - borderMarginWidth) / qreal(atom_matrix.rows() - 1);
        qreal scaleY = (windowSize.height() - _border_margin_height) / signalMaximum;

        //scale axis
        qreal scaleXAchse = (windowSize.width() - maxStrLenght - borderMarginWidth) / 20.0;
        qreal scaleYAchse = (windowSize.height() - _border_margin_height) / 10.0;

        for(qint32 i = 0; i < 11; i++)
        {
            if(signalNegativeMaximum == 0)                                          // x-Axis reached (y-value = 0)
            {
                // append scaled signalpoints and paint signal
                for(qint32 channel = 0; channel < atom_matrix.cols(); channel++)    // over all Channels
                {
                    QPolygonF poly;                   
                    for(qint32 h = 0; h < atom_matrix.rows(); h++)
                        poly.append(QPointF((h * scaleX) + maxStrLenght, -(atom_matrix(h, channel) * scaleY + _x_axis_height)));
                    QPen pen(_colors.at(channel), 0.5, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);
                    painter.setPen(pen);
                    painter.drawPolyline(poly);
                }                
                // paint x-axis                
                for(qint32 j = 1; j < 21; j++)
                {
                    if(fmod(j, 4.0) == 0)   //draw light grey sectors
                    {
                        QPen pen(Qt::darkGray, 0.5, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);
                        painter.setPen(pen);
                        painter.drawLine(j * scaleXAchse + maxStrLenght, -(_x_axis_height - windowSize.height()),
                                         j * scaleXAchse + maxStrLenght, -(_x_axis_height + windowSize.height()));   // scalelines x-axis
                    }
                    QPen pen(Qt::black, 1, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);
                    painter.setPen(pen);
                    painter.drawLine(j * scaleXAchse + maxStrLenght, -(_x_axis_height - 2),
                                     j * scaleXAchse + maxStrLenght, -(_x_axis_height + 2));   // scalelines x-axis
                }
                painter.drawLine(maxStrLenght - 40, -_x_axis_height, windowSize.width()-5, -_x_axis_height);    // paint x-axis
            }

            painter.drawText(3, -(i * scaleYAchse - windowSize.height()) - _border_margin_height / 2 + 4, QString::number(signalNegativeMaximum * scaleYText, 'g', 3));     // paint scalvalue Y-axis
            painter.drawLine(maxStrLenght - 2, -((i * scaleYAchse)-(windowSize.height()) + _border_margin_height / 2),
                             maxStrLenght + 2, -((i * scaleYAchse)-(windowSize.height()) + _border_margin_height / 2));  // scalelines y-axis

            signalNegativeMaximum++;
        }
        painter.drawLine(maxStrLenght, 2, maxStrLenght, windowSize.height() - 2);     // paint y-axis      
    }
    painter.end();
}

//*************************************************************************************************************************************

void ResiduumWindow::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    paint_residuum(_residuum_matrix, this->size(), _signal_maximum, _signal_negative_scale);
}

//*************************************************************************************************************************************

void ResiduumWindow::paint_residuum(MatrixXd residuum_matrix, QSize windowSize, qreal signalMaximum, qreal signalNegativeMaximum)
{
    // paint window white
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillRect(0,0,windowSize.width(),windowSize.height(),QBrush(Qt::white));
    painter.drawRect(0,0, windowSize.width(), windowSize.height());

    if(residuum_matrix.rows() > 0 && residuum_matrix.cols() > 0 && _signal_matrix.rows() > 0 && _signal_matrix.cols() > 0)
    {
        const qint32 maxStrLenght = 55;
        qint32 borderMarginWidth = 15;                             // reduce paintspace in GraphWindow of borderMargin pixels

        // scale axis title
        qreal scaleYText = signalMaximum / 10.0;

        // scale signal
        qreal scaleX = (windowSize.width() - maxStrLenght - borderMarginWidth) / qreal(residuum_matrix.rows() - 1);
        qreal scaleY = (windowSize.height() - _border_margin_height) / signalMaximum;

        //scale axis
        qreal scaleXAchse = (windowSize.width() - maxStrLenght - borderMarginWidth) / 20.0;
        qreal scaleYAchse = (windowSize.height() - _border_margin_height) / 10.0;

        for(qint32 i = 0; i < 11; i++)
        {
            if(signalNegativeMaximum == 0)                                              // x-axis reached (y-value = 0)
            {
                // append scaled signalpoints and paint signal
                for(qint32 channel = 0; channel < residuum_matrix.cols(); channel++)    // over all Channels
                {
                    QPolygonF poly;                    
                    for(qint32 h = 0; h < residuum_matrix.rows(); h++)
                        poly.append(QPointF(h * scaleX + maxStrLenght,  - (residuum_matrix(h, channel) * scaleY + _x_axis_height)));
                    QPen pen(_colors.at(channel), 0.5, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);
                    painter.setPen(pen);
                    painter.drawPolyline(poly);
                }
                // paint x-axis
                for(qint32 j = 1; j < 21; j++)
                {
                    if(fmod(j, 4.0) == 0)   //draw light grey sectors
                    {
                        QPen pen(Qt::darkGray, 0.5, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);
                        painter.setPen(pen);
                        painter.drawLine(j * scaleXAchse + maxStrLenght, -(_x_axis_height - windowSize.height()),
                                         j * scaleXAchse + maxStrLenght, -(_x_axis_height + windowSize.height()));   // scalelines x-axis
                    }                   
                    QPen pen(Qt::black, 1, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);
                    painter.setPen(pen);
                    painter.drawLine(j * scaleXAchse + maxStrLenght, -(_x_axis_height - 2),
                                     j * scaleXAchse + maxStrLenght, -(_x_axis_height + 2));   // scalelines x-axis
                }
                painter.drawLine(maxStrLenght - 40, -_x_axis_height, windowSize.width()-5, -_x_axis_height);    // paint x-axis
            }

            painter.drawText(3, -(i * scaleYAchse - windowSize.height()) - _border_margin_height/2 + 4, QString::number(signalNegativeMaximum * scaleYText, 'g', 3));     // paint scalevalue y-axis
            painter.drawLine(maxStrLenght - 2, -((i * scaleYAchse)-(windowSize.height()) + _border_margin_height / 2),
                             maxStrLenght + 2, -((i * scaleYAchse)-(windowSize.height()) + _border_margin_height / 2));  // scalelines y-axis

            signalNegativeMaximum++;
        }

        painter.drawLine(maxStrLenght, 2, maxStrLenght, windowSize.height() - 2);       // paint y-axis
    }
    painter.end();
}

//*************************************************************************************************************************************

void XAxisWindow::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    paint_axis(_signal_matrix, this->size());
}

//*************************************************************************************************************************************

void XAxisWindow::paint_axis(MatrixXd signalMatrix, QSize windowSize)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    if(signalMatrix.rows() > 0 && signalMatrix.cols() > 0)
    {
        const qint32 maxStrLenght = 55;
        qint32 borderMarginWidth = 15;
        qreal scaleXText = (signalMatrix.rows() - 1) /  _sample_rate / 20.0;                       // divide signallength
        qreal scaleXAchse = (windowSize.width() - maxStrLenght - borderMarginWidth) / 20.0;

        for(qint32 j = 0; j < 21; j++)
        {
            if(j == 20)
            {
                painter.drawText(j * scaleXAchse + 37, 20, QString::number(j * scaleXText + _from / _sample_rate + _offset_time, 'f', 2));    // scalevalue as string
                painter.drawLine(j * scaleXAchse + maxStrLenght, 5 + 2,
                                 j * scaleXAchse + maxStrLenght, 5 - 2);                    // scalelines
            }
            else
            {
                painter.drawText(j * scaleXAchse + 45, 20, QString::number(j * scaleXText + _from / _sample_rate  + _offset_time, 'f', 2));    // scalevalue as string
                painter.drawLine(j * scaleXAchse + maxStrLenght, 5 + 2,
                                 j * scaleXAchse + maxStrLenght, 5 - 2);                    // scalelines
            }
        }
        painter.drawText(5 , 20, "[sec]");  // unit
        painter.drawLine(5, 5, windowSize.width()-5, 5);  // paint y-line
    }
}

//*************************************************************************************************************************************

// starts MP-algorithm
void MainWindow::on_btt_Calc_clicked()
{
    truncation_criterion criterion;

    if(ui->chb_Iterations->isChecked() && !ui->chb_ResEnergy->isChecked()){
        criterion = Iterations;
    } else if(ui->chb_Iterations->isChecked() && ui->chb_ResEnergy->isChecked()){
        criterion = Both;
    } else if(ui->chb_ResEnergy->isChecked() && !ui->chb_Iterations->isChecked()){
        criterion = SignalEnergy;
    } else {
        criterion = Both;
        qDebug() << "Criterion not set. Defaulting to both.";
    }

    if(ui->btt_Calc->text()== "calculate")
    {
        ui->progressBarCalc->setValue(0);
        ui->progressBarCalc->setHidden(false);

        if(ui->chb_Iterations->checkState()  == Qt::Unchecked && ui->chb_ResEnergy->checkState() == Qt::Unchecked)
        {
            QMessageBox msgBox(QMessageBox::Warning, "Error", "No truncation criterion choosen.", QMessageBox::Ok, this);
            msgBox.exec();
            return;
        }

        if(((ui->dsb_energy->value() <= 1 && ui->dsb_energy->isEnabled()) && (ui->sb_Iterations->value() >= 500 && ui->sb_Iterations->isEnabled()))
                || (ui->dsb_energy->value() <= 1 && ui->dsb_energy->isEnabled() && !ui->sb_Iterations->isEnabled())
                || (ui->sb_Iterations->value() >= 500 && ui->sb_Iterations->isEnabled() && !ui->dsb_energy->isEnabled()) )
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

        ui->gb_trunc->setEnabled(false);
        ui->btt_OpenSignal->setEnabled(false);
        ui->btt_Calc->setText("cancel");
        ui->cb_channels->setEnabled(false);
        ui->cb_all_select->setEnabled(false);
        ui->dsb_from->setEnabled(false);
        ui->dsb_to->setEnabled(false);
        ui->sb_sample_count ->setEnabled(false);
        ui->tbv_Results->clearContents();
        ui->tbv_Results->setRowCount(0);
        ui->cb_all_select->setHidden(false);
        ui->lb_timer->setHidden(false);
        ui->actionExport->setEnabled(false);
        ui->lb_figure_of_merit->setHidden(true);
        ui->lb_info_content->clear();
        ui->actionTFplot->setEnabled(false);

        _adaptive_atom_list.clear();
        _fix_dict_atom_list.clear();

        //reset progressbar text color to black, reset infos
        pal.setColor(QPalette::Text, Qt::black);
        ui->progressBarCalc->setPalette(pal);
        ui->progressBarCalc->setFormat("residual energy: 100%            iterations: 0");

        ui->lb_signal_energy->setHidden(true);
        ui->lb_signal_energy_text->setHidden(true);
        ui->lb_approx_energy->setHidden(true);
        ui->lb_approx_energy_text->setHidden(true);
        ui->lb_residual_energy->setHidden(true);
        ui->lb_residual_energy_text->setHidden(true);

        is_white = false;

        is_saved = false;
        has_warning = false;

        _residuum_matrix = _signal_matrix;
        _atom_sum_matrix = MatrixXd::Zero(_signal_matrix.rows(), _signal_matrix.cols());      
        callAtomSumWindow->update();
        callResidumWindow->update();

        recieved_result_counter = 0;
        counter_time.start();
        _counter_timer->setInterval(100);
        _counter_timer->start();

        if(ui->rb_OwnDictionary->isChecked())
        {
            ui->tbv_Results->setColumnCount(2);
            ui->tbv_Results->setHorizontalHeaderLabels(QString("energy\n[%];atom").split(";"));
            ui->tbv_Results->setColumnWidth(0,55);
            ui->tbv_Results->setColumnWidth(1,280);
            max_tbv_header_width = 0;
            calc_fix_mp(QString(QDir::homePath() + "/" + "Matching-Pursuit-Toolbox/%1.dict").arg(ui->cb_Dicts->currentText()), _signal_matrix, criterion);
        }
        else if(ui->rb_adativMp->isChecked())
        {
            QList<qint32> sizes = ui->splitter->sizes();
            sizes.insert(0, 240);
            ui->splitter->setSizes(sizes);
            max_tbv_header_width = 150;

            ui->tbv_Results->setColumnCount(5);
            ui->tbv_Results->setHorizontalHeaderLabels(QString("energy\n[%];scale\n[sec];trans\n[sec];modu\n[Hz];phase\n[rad]").split(";"));
            ui->tbv_Results->setColumnWidth(0,55);
            ui->tbv_Results->setColumnWidth(1,45);
            ui->tbv_Results->setColumnWidth(2,40);
            ui->tbv_Results->setColumnWidth(3,40);
            ui->tbv_Results->setColumnWidth(4,50);

            calc_adaptiv_mp(_signal_matrix, criterion);
        }        
        is_calulating = true;
    }
    //cancel calculation thread
    else if(ui->btt_Calc->text() == "cancel")
    {
        emit mp_Thread->requestInterruption();
        ui->btt_Calc->setText("wait...");
    }
}

//*************************************************************************************************************

void MainWindow::on_time_out()
{
    QTime diff_time(0,0);
    diff_time = diff_time.addMSecs(counter_time.elapsed());
    ui->lb_timer->setText(diff_time.toString("hh:mm:ss.zzz"));
    _counter_timer->start();
}

//*************************************************************************************************************
void MainWindow::recieve_result(qint32 current_iteration, qint32 max_iterations, qreal current_energy, qreal max_energy, MatrixXd residuum,
                                adaptive_atom_list adaptive_atom_res_list, fix_dict_atom_list fix_dict_atom_res_list)
{
    tbv_is_loading = true;

    QSettings settings;
    qreal percent = ui->dsb_energy->value();
    residuum_energy = 100 * (max_energy - current_energy) / max_energy;
    signal_energy = max_energy;

    //current atoms list update
    if(fix_dict_atom_res_list.isEmpty())
    {
        GaborAtom temp_atom = adaptive_atom_res_list.last().last();
        QList<GaborAtom> temp_channel_list= adaptive_atom_res_list.last();
        qreal atom_energy = 0;

        for(qint32 i = 0; i < adaptive_atom_res_list.last().length(); i++)
        {
            atom_energy += adaptive_atom_res_list.last().at(i).energy;
        }


        qreal phase = temp_atom.phase;
        if(temp_atom.phase > 2*PI) phase -= 2*PI;

        QTableWidgetItem* atomEnergieItem = new QTableWidgetItem(QString::number(100 * atom_energy / max_energy, 'f', 2));
        QTableWidgetItem* atomScaleItem = new QTableWidgetItem(QString::number(temp_atom.scale / _sample_rate, 'g', 3));
        QTableWidgetItem* atomTranslationItem = new QTableWidgetItem(QString::number(temp_atom.translation / qreal(_sample_rate) + _from  / _sample_rate + _offset_time, 'g', 4));
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

        //_adaptive_atom_list.append(temp_atom); //old
        _adaptive_atom_list.append(adaptive_atom_res_list.last());

        //ToDo: sort_results for trial_separation
        if(settings.value("sort_results", true).toBool())// && !settings.value("trial_separation", false).toBool())
        {
            qSort(adaptive_atom_res_list.begin(), adaptive_atom_res_list.end(), sort_energy_adaptive);
            qSort(_adaptive_atom_list.begin(), _adaptive_atom_list.end(), sort_energy_adaptive);
        }

        // ToDo: qint32 index = adaptive_atom_res_list.indexOf(temp_atom);
        qint32 index = 0;
        while(index < _adaptive_atom_list.length())
        {
            if(temp_atom.scale == _adaptive_atom_list.at(index).last().scale
                    && temp_atom.modulation == _adaptive_atom_list.at(index).last().modulation
                    && temp_atom.translation == _adaptive_atom_list.at(index).last().translation
                    && temp_atom.energy == _adaptive_atom_list.at(index).last().energy)
                break;
            index++;
        }

        ui->tbv_Results->insertRow(index);
        ui->tbv_Results->setItem(index, 0, atomEnergieItem);
        ui->tbv_Results->setItem(index, 1, atomScaleItem);
        ui->tbv_Results->setItem(index, 2, atomTranslationItem);
        ui->tbv_Results->setItem(index, 3, atomModulationItem);
        ui->tbv_Results->setItem(index, 4, atomPhaseItem);

        //update residuum and atom sum for painting and later save to hdd
        for(qint32 i = 0; i < _signal_matrix.cols(); i++)
        {
            if(settings.value("trial_separation", false).toBool())
            {
                VectorXd atom_vec = temp_channel_list.at(i).max_scalar_product * temp_atom.create_real(temp_channel_list.at(i).sample_count,
                                                                                                                   temp_channel_list.at(i).scale,
                                                                                                                   temp_channel_list.at(i).translation,
                                                                                                                   temp_channel_list.at(i).modulation,
                                                                                                                   temp_channel_list.at(i).phase);
                _residuum_matrix.col(i) -= atom_vec;
                _atom_sum_matrix.col(i) += atom_vec;
                //_residuum_matrix = residuum;
            }
            else
            {
                VectorXd atom_vec = temp_atom.max_scalar_list.at(i) * temp_atom.create_real(temp_atom.sample_count, temp_atom.scale, temp_atom.translation, temp_atom.modulation, temp_atom.phase_list.at(i));
                _residuum_matrix.col(i) -= atom_vec;
                _atom_sum_matrix.col(i) += atom_vec;
            }
        }
    }
    else if(adaptive_atom_res_list.isEmpty())
    {
        FixDictAtom temp_atom = fix_dict_atom_res_list.last();
        fix_dict_atom_res_list.last().display_text = create_display_text(fix_dict_atom_res_list.last());
        temp_atom.display_text =  create_display_text(fix_dict_atom_res_list.last());

        QFont font;
        QFontMetrics fm(font);
        qint32 header_width = fm.width(fix_dict_atom_res_list.last().display_text) + 35;
        if(header_width > max_tbv_header_width)
        {
            ui->tbv_Results->setColumnWidth(1, header_width);
            max_tbv_header_width = header_width;
        }

        QTableWidgetItem* atom_energie_item = new QTableWidgetItem(QString::number(100 * temp_atom.energy / max_energy, 'f', 2));
        QTableWidgetItem* atom_name_item = new QTableWidgetItem(temp_atom.display_text);

        atom_energie_item->setFlags(Qt::ItemIsUserCheckable);
        atom_name_item->setFlags(Qt::NoItemFlags);
        atom_energie_item->setCheckState(Qt::Checked);

        atom_energie_item->setTextAlignment(0x0082);
        atom_name_item->setTextAlignment(0x0081);

        _fix_dict_atom_list.append(temp_atom);
        if(settings.value("sort_results", true).toBool())
        {
            qSort(fix_dict_atom_res_list.begin(),fix_dict_atom_res_list.end(), sort_energy_fix);
            qSort(_fix_dict_atom_list.begin(),_fix_dict_atom_list.end(), sort_energy_fix);
        }
        // ToDo: index = _adaptive_atom_list.indexOf(temp_atom);
        qint32 index = 0;
        while(index < fix_dict_atom_res_list.length())
        {
            if(temp_atom.display_text == fix_dict_atom_res_list.at(index).display_text)
                 break;
             index++;
        }
        ui->tbv_Results->insertRow(index);
        ui->tbv_Results->setItem(index, 0, atom_energie_item);
        ui->tbv_Results->setItem(index, 1, atom_name_item);

        //update residuum and atom sum for painting and later save to hdd

        for(qint32 i = 0; i < _signal_matrix.cols(); i++)
        {
            _residuum_matrix.col(i) -= temp_atom.max_scalar_list.at(i) * temp_atom.atom_samples;
            _atom_sum_matrix.col(i) += temp_atom.max_scalar_list.at(i) * temp_atom.atom_samples;
        }/*
        _residuum_matrix = residuum;
        _atom_sum_matrix = _signal_matrix - residuum;
    */}

    //progressbar update remaining energy and iterations update

    QString text = QString("residual energy: %0%            iterations: %1").arg(QString::number(residuum_energy, 'f', 2)).arg(current_iteration);
    ui->progressBarCalc->setFormat(text);
    qint32 prgrsbar_adapt = 99;
    if(max_iterations > 1999 && current_iteration < 100)
        ui->progressBarCalc->setMaximum(100);
    if(ui->chb_ResEnergy->isChecked() && (current_iteration >= (prgrsbar_adapt)) && (max_energy - current_energy) > (0.01 * percent * max_energy))
        ui->progressBarCalc->setMaximum(current_iteration + 5);
    if(max_iterations < 1999) ui->progressBarCalc->setMaximum(max_iterations);
    ui->progressBarCalc->setValue(current_iteration);

    if(((current_iteration == max_iterations) || (max_energy - current_energy) < (0.01 * percent * max_energy))&&ui->chb_ResEnergy->isChecked())
        ui->progressBarCalc->setValue(ui->progressBarCalc->maximum());

    if(ui->progressBarCalc->value() > ui->progressBarCalc->maximum() / 2 && !is_white)
    {
        pal.setColor(QPalette::Text, Qt::white);
        ui->progressBarCalc->setPalette(pal);
        is_white = true;
    }

    // update ui
    if(max_iterations > 10 && percent < 1 && _signal_matrix.cols() > 40 && recieved_result_counter % 10 == 0)
    {
        callAtomSumWindow->update();
        callResidumWindow->update();
    }
    else if(max_iterations > 5 && percent < 5 && _signal_matrix.cols() > 20 && recieved_result_counter % 5 == 0)
    {
        callAtomSumWindow->update();
        callResidumWindow->update();
    }
    else if(_signal_matrix.cols() < 20)
    {
        callAtomSumWindow->update();
        callResidumWindow->update();
    }

    tbv_is_loading = false;
    recieved_result_counter++;
}

//*************************************************************************************************************

void MainWindow::recieve_warnings(qint32 warning_number)
{
    QSettings settings;

    if(settings.value("show_infos", true).toBool())
    {
        ui->lb_info_content->setHidden(false);
        ui->lb_info->setHidden(false);

        QString text;
        if(warning_number == 1 && !has_warning)
        {
            text = "The dictionary does not have the appropriate atoms to approximate the signal more closely. Calculation terminated before reaching truncation criterion. ";
            ui->lb_info_content->setText(text);
            has_warning = true;
        }
        else if(warning_number == 2 && !has_warning)
        {
            text = "No matching sample count between atoms and signal. This may lead to discontinuities. ";
            ui->lb_info_content->setText(text);
            has_warning = true;
        }
        else if(has_warning && warning_number != 10)
        {
            text = "This dictionary does not fit the signals sample count (leads to discontinuities) and excludes atoms to reduce further residual energy. Calculation terminated before reaching truncation criterion. ";
            ui->lb_info_content->setText(text);
        }
        if(warning_number == 10)
        {
            text = ui->lb_info_content->text();
            text.append("Algorithm canceled by user interaction.");
            ui->lb_info_content->setText(text);
        }
        if(warning_number == 11)
        {
            text = QString("Simplex Iteration limit of %1 achieved, result may not be optimal. ").arg(settings.value("adaptive_iterations").toInt());
            ui->lb_info_content->setText(text);
        }

    }
}

//*************************************************************************************************************

void MainWindow::tbv_selection_changed(const QModelIndex& topLeft, const QModelIndex& bottomRight)
{
    Q_UNUSED(bottomRight);
    bool all_selected = true;
    bool all_deselected = true;
    QSettings settings;

    if(tbv_is_loading) return;

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
            {
                _atom_sum_matrix.col(channels) += real_residuum_matrix.col(channels);
                _residuum_matrix.col(channels) -= real_residuum_matrix.col(channels);
            }
            composed_energy += residuum_energy;
        }
        else
        {
            for(qint32 channels = 0; channels < _signal_matrix.cols(); channels++)
            {
                _atom_sum_matrix.col(channels) -= real_residuum_matrix.col(channels);
                _residuum_matrix.col(channels) += real_residuum_matrix.col(channels);
            }
            composed_energy -= residuum_energy;
        }
    }
    else
    {
        if(ui->tbv_Results->columnCount() > 2)
        {
            if(!settings.value("trial_separation", false).toBool())//normal adaptive mp with global bestmatching atom
            {
                GaborAtom  atom = _adaptive_atom_list.at(topLeft.row()).last();
                if(!auto_change)
                    select_atoms_map[topLeft.row()] = item->checkState();

                if(item->checkState())
                {
                    for(qint32 channels = 0; channels < _signal_matrix.cols(); channels++)
                    {
                        _atom_sum_matrix.col(channels) += atom.max_scalar_list.at(channels) * atom.create_real(atom.sample_count, atom.scale, atom.translation, atom.modulation, atom.phase_list.at(channels));
                        _residuum_matrix.col(channels) -= atom.max_scalar_list.at(channels) * atom.create_real(atom.sample_count, atom.scale, atom.translation, atom.modulation, atom.phase_list.at(channels));
                    }
                    composed_energy += 100 * atom.energy / signal_energy;
                }
                else
                {
                    for(qint32 channels = 0; channels < _signal_matrix.cols(); channels++)
                    {
                        _atom_sum_matrix.col(channels) -= atom.max_scalar_list.at(channels) * atom.create_real(atom.sample_count, atom.scale, atom.translation, atom.modulation, atom.phase_list.at(channels));
                        _residuum_matrix.col(channels) += atom.max_scalar_list.at(channels) * atom.create_real(atom.sample_count, atom.scale, atom.translation, atom.modulation, atom.phase_list.at(channels));
                    }
                    composed_energy -= 100 * atom.energy / signal_energy;
                }
            }
            else    //trial separation
            {
                if(item->checkState())
                {
                    for(qint32 channels = 0; channels < _signal_matrix.cols(); channels++)
                    {
                        GaborAtom  atom = _adaptive_atom_list.at(topLeft.row()).at(channels);
                        _atom_sum_matrix.col(channels) += atom.max_scalar_product * atom.create_real(atom.sample_count, atom.scale, atom.translation, atom.modulation, atom.phase);
                        _residuum_matrix.col(channels) -= atom.max_scalar_product * atom.create_real(atom.sample_count, atom.scale, atom.translation, atom.modulation, atom.phase);
                        composed_energy += 100 * atom.energy / signal_energy;
                    }
                }
                else
                {
                    for(qint32 channels = 0; channels < _signal_matrix.cols(); channels++)
                    {
                        GaborAtom  atom = _adaptive_atom_list.at(topLeft.row()).at(channels);
                        _atom_sum_matrix.col(channels) -= atom.max_scalar_product * atom.create_real(atom.sample_count, atom.scale, atom.translation, atom.modulation, atom.phase);
                        _residuum_matrix.col(channels) += atom.max_scalar_product * atom.create_real(atom.sample_count, atom.scale, atom.translation, atom.modulation, atom.phase);
                        composed_energy -= 100 * atom.energy / signal_energy;
                    }
                }

            }
        }
        else
        {
            FixDictAtom  atom = _fix_dict_atom_list.at(topLeft.row());
            if(!auto_change)
                select_atoms_map[topLeft.row()] = item->checkState();

            if(item->checkState())
            {
                for(qint32 channels = 0; channels < _signal_matrix.cols(); channels++)
                {
                    _atom_sum_matrix.col(channels) += atom.max_scalar_list.at(channels) * atom.atom_samples;
                    _residuum_matrix.col(channels) -= atom.max_scalar_list.at(channels) * atom.atom_samples;
                    composed_energy += 100 * atom.energy / signal_energy;
                }
            }
            else
            {
                for(qint32 channels = 0; channels < _signal_matrix.cols(); channels++)
                {
                    _atom_sum_matrix.col(channels) -= atom.max_scalar_list.at(channels) * atom.atom_samples;
                    _residuum_matrix.col(channels) += atom.max_scalar_list.at(channels) * atom.atom_samples;
                    composed_energy -= 100 * atom.energy / signal_energy;
                }
            }
        }
    }

    ui->lb_signal_energy_text->setText("absolute signal energy:");
    ui->lb_signal_energy->setText(QString::number(signal_energy, 'g', 2));
    ui->lb_approx_energy_text->setText("approximation energy:");
    ui->lb_approx_energy->setText(QString::number(std::fabs(composed_energy), 'f', 2) + "%");
    ui->lb_residual_energy_text->setText("remaining residual energy:");
    ui->lb_residual_energy->setText(QString::number(std::fabs(100.0 - composed_energy), 'f', 2) + "%");

    callAtomSumWindow->update();
    callResidumWindow->update();
}

//*************************************************************************************************************

void MainWindow::calc_thread_finished()
{
    is_calulating = false;
    tbv_is_loading = true;

    if(_fix_dict_atom_list.isEmpty() && !_adaptive_atom_list.isEmpty())
        ui->actionExport->setEnabled(true);

     ui->actionSpeicher->setEnabled(true);
     ui->actionSpeicher_unter->setEnabled(true);


    _counter_timer->stop();
    ui->gb_trunc->setEnabled(true);
    ui->btt_OpenSignal->setEnabled(true);
    ui->progressBarCalc->setValue(ui->progressBarCalc->maximum());

    pal.setColor(QPalette::Text, Qt::white);
    ui->progressBarCalc->setPalette(pal);
    is_white = true;

    ui->btt_Calc->setText("calculate");
    ui->cb_channels->setEnabled(true);
    ui->cb_all_select->setEnabled(true);
    ui->dsb_from->setEnabled(true);
    ui->dsb_to->setEnabled(true);    
    ui->sb_sample_count ->setEnabled(true);
    ui->actionTFplot->setEnabled(true);

    QList<qint32> sizes = ui->splitter->sizes();
    sizes.insert(0, max_tbv_header_width + 100);
    ui->splitter->setSizes(sizes);

    for(qint32 col = 0; col < ui->tbv_Results->columnCount(); col++)
        for(qint32 row = 0; row < ui->tbv_Results->rowCount(); row++)
        {
            if(col == 0)
                ui->tbv_Results->item(row, col)->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
            else
                ui->tbv_Results->item(row, col)->setFlags(Qt::ItemIsEnabled);
        }

    real_residuum_matrix = _residuum_matrix;

    for(qint32 i = 0; i < ui->tbv_Results->rowCount(); i++)
        select_atoms_map.insert(i, true);

    ui->tbv_Results->setRowCount(ui->tbv_Results->rowCount() + 1);

    QTableWidgetItem* energy_item = new QTableWidgetItem(QString::number(residuum_energy, 'f', 2));//ui->lb_RestEnergieResiduumValue->text().remove('%'));
    energy_item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
    energy_item->setCheckState(Qt::Unchecked);
    energy_item->setTextAlignment(0x0082);

    QTableWidgetItem* residuum_item = new QTableWidgetItem("residue");
    residuum_item->setFlags(Qt::ItemIsEnabled);
    residuum_item->setTextAlignment(Qt::AlignCenter);

    ui->tbv_Results->setItem(ui->tbv_Results->rowCount() - 1, 0, energy_item);
    ui->tbv_Results->setItem(ui->tbv_Results->rowCount() - 1, 1, residuum_item);
    ui->tbv_Results->setSpan(ui->tbv_Results->rowCount() - 1, 1, 1, 4);

    //calc energy infos
    composed_energy = 100 - residuum_energy;
    ui->lb_signal_energy_text->setText("absolute signal energy:");
    ui->lb_signal_energy->setText(QString::number(signal_energy, 'g', 2));
    ui->lb_approx_energy_text->setText("approximation energy:");
    ui->lb_approx_energy->setText(QString::number(std::fabs(composed_energy), 'f', 2) + "%");
    ui->lb_residual_energy_text->setText("remaining residual energy:");
    ui->lb_residual_energy->setText(QString::number(std::fabs(100.0 - composed_energy), 'f', 2) + "%");
    //show energy infos
    ui->lb_signal_energy->setHidden(false);
    ui->lb_signal_energy_text->setHidden(false);
    ui->lb_approx_energy->setHidden(false);
    ui->lb_approx_energy_text->setHidden(false);
    ui->lb_residual_energy->setHidden(false);
    ui->lb_residual_energy_text->setHidden(false);

    //figure of merit to evaluate the alogorithm, here we use correlation between signal and atom_sum_matrix
    QSettings settings;
    if(settings.value("show_infos", true).toBool())
    {
        qreal correlation = 0;
        qreal divisor_sig = 0;
        qreal divisor_app = 0;
        MatrixXd sig_no_mean = (_signal_matrix.array() - _signal_matrix.mean()).matrix();
        MatrixXd app_no_mean = (_atom_sum_matrix.array() - _atom_sum_matrix.mean()).matrix();
        for(qint32 channel = 0; channel < _atom_sum_matrix.cols(); channel++)
        {
            correlation += sig_no_mean.col(channel).dot(app_no_mean.col(channel));
            divisor_sig += sig_no_mean.col(channel).dot(sig_no_mean.col(channel));
            divisor_app += app_no_mean.col(channel).dot(app_no_mean.col(channel));
        }

        qreal divisor = sqrt(divisor_app * divisor_sig);
        correlation /= divisor;
        correlation *= 1000;
        qint32 corr = correlation;
        correlation = qreal(corr) / 1000;
        cout << "\ncorrelation:  "<<correlation;
        ui->lb_figure_of_merit->setText(QString("FOM: %1").arg((QString::number(correlation, 'f', 3))));
        ui->lb_figure_of_merit->setHidden(false);
    }

    tbv_is_loading = false;

    update();
}

//*************************************************************************************************************

void MainWindow::calc_adaptiv_mp(MatrixXd signal, truncation_criterion criterion)
{
    adaptive_Mp = new AdaptiveMp();
    qreal res_energy = ui->dsb_energy->value();

    //threading
    mp_Thread = new QThread;
    adaptive_Mp->moveToThread(mp_Thread);

    connect(this, SIGNAL(send_input(MatrixXd, qint32, qreal, bool, qint32, qint32, qreal, qreal, qreal, qreal, bool)),
            adaptive_Mp, SLOT(recieve_input(MatrixXd, qint32, qreal, bool, qint32, qint32, qreal, qreal, qreal, qreal, bool)));
    connect(adaptive_Mp, SIGNAL(current_result(qint32, qint32, qreal, qreal, MatrixXd, adaptive_atom_list, fix_dict_atom_list)),
                 this, SLOT(recieve_result(qint32, qint32, qreal, qreal, MatrixXd, adaptive_atom_list, fix_dict_atom_list)));
    connect(adaptive_Mp, SIGNAL(finished_calc()), mp_Thread, SLOT(quit()));
    connect(adaptive_Mp, SIGNAL(finished_calc()), adaptive_Mp, SLOT(deleteLater()));
    connect(mp_Thread, SIGNAL(finished()), this, SLOT(calc_thread_finished()));
    connect(mp_Thread, SIGNAL(finished()), mp_Thread, SLOT(deleteLater()));

    connect(adaptive_Mp, SIGNAL(send_warning(qint32)), this, SLOT(recieve_warnings(qint32)));

    QSettings settings;
    bool fixphase = settings.value("fixPhase", false).toBool();
    bool trial_separation = settings.value("trial_separation", false).toBool();
    qint32 boost = settings.value("boost", 100).toInt();
    qint32 iterations = settings.value("adaptive_iterations", 1E3).toInt();
    qreal reflection = settings.value("adaptive_reflection", 1.00).toDouble();
    qreal expansion = settings.value("adaptive_expansion", 0.20).toDouble();
    qreal contraction = settings.value("adaptive_contraction", 0.5).toDouble();
    qreal fullcontraction = settings.value("adaptive_fullcontraction", 0.50).toDouble();
    switch(criterion)
    {
        case Iterations:        
            emit send_input(signal, ui->sb_Iterations->value(), qreal(MININT32), fixphase, boost, iterations,
                            reflection, expansion, contraction, fullcontraction, trial_separation);
            mp_Thread->start();        
            break;

        case SignalEnergy:        
            emit send_input(signal, MAXINT32, res_energy, fixphase, boost, iterations,
                            reflection, expansion, contraction, fullcontraction, trial_separation);
            mp_Thread->start();        
            break;

        case Both:
            emit send_input(signal, ui->sb_Iterations->value(), res_energy, fixphase, boost, iterations,
                            reflection, expansion, contraction, fullcontraction, trial_separation);
            mp_Thread->start();        
            break;
    }       
}

//************************************************************************************************************************************

void MainWindow::calc_fix_mp(QString path, MatrixXd signal, truncation_criterion criterion)
{
    fixDict_Mp = new FixDictMp();
    qreal res_energy = ui->dsb_energy->value();

    //threading
    mp_Thread = new QThread;
    fixDict_Mp->moveToThread(mp_Thread);    

    connect(this, SIGNAL(send_input_fix_dict(MatrixXd, qint32, qreal, qint32, QString, qreal)),
            fixDict_Mp, SLOT(recieve_input(MatrixXd, qint32, qreal, qint32, QString, qreal)));
    connect(fixDict_Mp, SIGNAL(current_result(qint32, qint32, qreal, qreal, MatrixXd, adaptive_atom_list, fix_dict_atom_list)),
                  this, SLOT(recieve_result(qint32, qint32, qreal, qreal, MatrixXd, adaptive_atom_list, fix_dict_atom_list)));
    connect(fixDict_Mp, SIGNAL(finished_calc()), mp_Thread, SLOT(quit()));
    connect(fixDict_Mp, SIGNAL(finished_calc()), fixDict_Mp, SLOT(deleteLater()));
    connect(mp_Thread, SIGNAL(finished()), this, SLOT(calc_thread_finished()));
    connect(mp_Thread, SIGNAL(finished()), mp_Thread, SLOT(deleteLater()));

    connect(fixDict_Mp, SIGNAL(send_warning(qint32)), this, SLOT(recieve_warnings(qint32)));

    QSettings settings;
    qint32 boost = settings.value("boost_fixDict", 100).toInt();
    qreal delta_energy = settings.value("delta_energy", 0.0005).toDouble();

    switch(criterion)
    {
        case Iterations:
            emit send_input_fix_dict(signal, ui->sb_Iterations->value(), qreal(MININT32), boost, path, delta_energy);
            mp_Thread->start();
            break;

        case SignalEnergy:
            emit send_input_fix_dict(signal, MAXINT32, res_energy, boost, path, delta_energy);
            mp_Thread->start();
            break;

        case Both:
            emit send_input_fix_dict(signal, ui->sb_Iterations->value(), res_energy, boost, path, delta_energy);
            mp_Thread->start();
            break;
    }
}

//*************************************************************************************************************

QString MainWindow::create_display_text(const FixDictAtom& global_best_matching)
{
    QSettings settings;
    QString display_text;

    if(!settings.value("show_phys_params", false).toBool())
    {
        if(global_best_matching.type == GABORATOM)
        {
            display_text = QString("Gaboratom: scale: %0, translation: %1, modulation: %2, phase: %3")
                    .arg(QString::number(global_best_matching.gabor_atom.scale, 'f', 2))
                    .arg(QString::number(global_best_matching.translation, 'f', 2))
                    .arg(QString::number(global_best_matching.gabor_atom.modulation, 'f', 2))
                    .arg(QString::number(global_best_matching.gabor_atom.phase, 'f', 2));
        }
        else if(global_best_matching.type == CHIRPATOM)
        {
            display_text = QString("Chripatom: scale: %0, translation: %1, modulation: %2, phase: %3, chirp: %4")
                    .arg(QString::number(global_best_matching.chirp_atom.scale, 'f', 2))
                    .arg(QString::number(global_best_matching.translation, 'f', 2))
                    .arg(QString::number(global_best_matching.chirp_atom.modulation, 'f', 2))
                    .arg(QString::number(global_best_matching.chirp_atom.phase, 'f', 2))
                    .arg(QString::number(global_best_matching.chirp_atom.chirp, 'f', 2));
        }
        else if(global_best_matching.type == FORMULAATOM)
        {
            display_text = QString("%0:  transl: %1 a: %2, b: %3 c: %4, d: %5, e: %6, f: %7, g: %8, h: %9")
                    .arg(global_best_matching.atom_formula)
                    .arg(QString::number(global_best_matching.translation,    'f', 2))
                    .arg(QString::number(global_best_matching.formula_atom.a, 'f', 2))
                    .arg(QString::number(global_best_matching.formula_atom.b, 'f', 2))
                    .arg(QString::number(global_best_matching.formula_atom.c, 'f', 2))
                    .arg(QString::number(global_best_matching.formula_atom.d, 'f', 2))
                    .arg(QString::number(global_best_matching.formula_atom.e, 'f', 2))
                    .arg(QString::number(global_best_matching.formula_atom.f, 'f', 2))
                    .arg(QString::number(global_best_matching.formula_atom.g, 'f', 2))
                    .arg(QString::number(global_best_matching.formula_atom.h, 'f', 2));
        }
    }
    else
    {
        if(global_best_matching.type == GABORATOM)
        {
            qreal phase = global_best_matching.gabor_atom.phase;
            if(global_best_matching.gabor_atom.phase > 2*PI) phase -= 2*PI;

            display_text = QString("Gaboratom: scale: %0 sec, translation: %1 sec, modulation: %2 Hz, phase: %3 rad")
                    .arg(QString::number(global_best_matching.gabor_atom.scale / _sample_rate, 'f', 2))
                    .arg(QString::number((global_best_matching.translation + _from) / _sample_rate + _offset_time, 'f', 2))
                    .arg(QString::number(global_best_matching.gabor_atom.modulation * _sample_rate / global_best_matching.sample_count, 'f', 2))
                    .arg(QString::number(phase, 'f', 2));
        }
        else if(global_best_matching.type == CHIRPATOM)
        {
            qreal phase = global_best_matching.chirp_atom.phase;
            if(global_best_matching.chirp_atom.phase > 2*PI) phase -= 2*PI;

            display_text = QString("Chripatom: scale: %0 sec, translation: %1 sec, modulation: %2 Hz, phase: %3 rad, chirp: %4")
                    .arg(QString::number(global_best_matching.chirp_atom.scale  / _sample_rate, 'f', 2))
                    .arg(QString::number((global_best_matching.translation + _from) / _sample_rate + _offset_time, 'f', 2))
                    .arg(QString::number(global_best_matching.chirp_atom.modulation * _sample_rate / global_best_matching.sample_count, 'f', 2))
                    .arg(QString::number(phase, 'f', 2))
                    .arg(QString::number(global_best_matching.chirp_atom.chirp, 'f', 2));
        }
        else if(global_best_matching.type == FORMULAATOM)
        {
            display_text = QString("%0:  transl: %1 a: %2, b: %3 c: %4, d: %5, e: %6, f: %7, g: %8, h: %9")
                    .arg(global_best_matching.atom_formula)
                    .arg(QString::number((global_best_matching.translation + _from) / _sample_rate + _offset_time, 'f', 2))
                    .arg(QString::number(global_best_matching.formula_atom.a, 'f', 2))
                    .arg(QString::number(global_best_matching.formula_atom.b, 'f', 2))
                    .arg(QString::number(global_best_matching.formula_atom.c, 'f', 2))
                    .arg(QString::number(global_best_matching.formula_atom.d, 'f', 2))
                    .arg(QString::number(global_best_matching.formula_atom.e, 'f', 2))
                    .arg(QString::number(global_best_matching.formula_atom.f, 'f', 2))
                    .arg(QString::number(global_best_matching.formula_atom.g, 'f', 2))
                    .arg(QString::number(global_best_matching.formula_atom.h, 'f', 2));
        }
    }

    return display_text;
}

//*****************************************************************************************************************

// Opens Dictionaryeditor
void MainWindow::on_actionW_rterbucheditor_triggered()
{        
    if(_editor_window == NULL)
    {
        _editor_window = new EditorWindow();
        connect(_editor_window, SIGNAL(dict_saved()), this, SLOT(on_dicts_saved()));
    }
    if(!_editor_window->isVisible()) _editor_window->show();
    else
    {
        _editor_window->setWindowState(Qt::WindowActive);
        _editor_window->raise();
    }

}

//*****************************************************************************************************************

// opens advanced Dictionaryeditor
void MainWindow::on_actionErweiterter_W_rterbucheditor_triggered()
{
     if(_enhanced_editor_window == NULL)
     {
         _enhanced_editor_window = new Enhancededitorwindow();
         if(_editor_window == NULL) _editor_window = new EditorWindow();
         connect(_enhanced_editor_window, SIGNAL(dict_saved()), _editor_window, SLOT(on_save_dicts()));
     }
     if(!_enhanced_editor_window->isVisible()) _enhanced_editor_window->show();
     else
     {
         _enhanced_editor_window->setWindowState(Qt::WindowActive);
         _enhanced_editor_window->raise();
     }
}

//*****************************************************************************************************************


// opens formula editor
void MainWindow::on_actionAtomformeleditor_triggered()
{
    if(_formula_editor == NULL)
    {
        _formula_editor = new Formulaeditor();
        if(_enhanced_editor_window == NULL) _enhanced_editor_window = new Enhancededitorwindow();
        connect(_formula_editor, SIGNAL(formula_saved()), _enhanced_editor_window, SLOT(on_formula_saved()));
    }
    if(!_formula_editor->isVisible()) _formula_editor->show();
    else
    {
        _formula_editor->setWindowState(Qt::WindowActive);
        _formula_editor->raise();
    }
}

//*****************************************************************************************************************

// open treebase window
void MainWindow::on_actionCreate_treebased_dictionary_triggered()
{
    _treebased_dict_window = new TreebasedDictWindow();
    _treebased_dict_window->show();
}

//*****************************************************************************************************************

// open settings
void MainWindow::on_actionSettings_triggered()
{
    if(_setting_window == NULL) _setting_window = new settingwindow();
    if(_setting_window != NULL) _setting_window->set_values();
    if(!_setting_window->isVisible()) _setting_window->show();
    else
    {
        _setting_window->setWindowState(Qt::WindowActive);
        _setting_window->raise();
    }

    connect(_setting_window, SIGNAL(change_info_label()), this, SLOT(activate_info_label()));
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

void MainWindow::on_dsb_sample_rate_editingFinished()
{
    _sample_rate = ui->dsb_sample_rate->value();
    if(!read_fiff_changed)
    {
        read_fiff_changed = true;

        ui->dsb_from->setMaximum((_last_sample - 63) / _sample_rate);
        ui->dsb_to->setMinimum((_first_sample + 63) / _sample_rate);
        ui->lb_from->setToolTip(QString("minimum: %1 seconds").arg(_first_sample / _sample_rate));
        ui->lb_to->setToolTip(QString("maximum: %1 seconds").arg(_last_sample / _sample_rate));
        ui->sb_sample_count->setToolTip(QString("epoch: %1 sec").arg((_to - _from + 1) / _sample_rate));
        ui->lb_samples->setToolTip(QString("min: 64 (%1 sec)\nmax: 4096 (%2 sec)").arg(64 / _sample_rate).arg(4096 / _sample_rate));

        if(_from != 0)
            ui->dsb_from->setValue(_from / _sample_rate + _offset_time);
        ui->dsb_to->setValue(_to / _sample_rate + _offset_time);
        read_fiff_changed = false;
    }
    callXAxisWindow->update();
}

//*****************************************************************************************************************

void MainWindow::on_dsb_from_editingFinished()
{   
    if(read_fiff_changed || _from == last_from) return;
    if(ui->dsb_from->value() * _sample_rate < _first_sample)
        ui->dsb_from->setValue(_first_sample / _sample_rate + _offset_time);

    if(file_name.split('.').last() == "fif")
    {
        if(!_pick_evoked.isEmpty())
            read_fiff_ave_new();
        else
            read_fiff_file_new(file_name);
    }
    else read_matlab_file_new();
    last_from = _from;
}

//*****************************************************************************************************************

void MainWindow::on_dsb_to_editingFinished()
{
    if(read_fiff_changed || _to == last_to) return;
    if(ui->dsb_to->value() * _sample_rate > _last_sample)
        ui->dsb_to->setValue(_last_sample / _sample_rate);

    if(file_name.split('.').last() == "fif")
    {
        if(!_pick_evoked.isEmpty())
            read_fiff_ave_new();
        else
            read_fiff_file_new(file_name);
    }
    else read_matlab_file_new();
    last_to = _to;
}

//*****************************************************************************************************************

void MainWindow::on_sb_sample_count_editingFinished()
{
    if(read_fiff_changed || ui->sb_sample_count->value() == last_sample_count) return;

    if(file_name.split('.').last() == "fif")
    {
        if(!_pick_evoked.isEmpty())
            read_fiff_ave_new();
        else
            read_fiff_file_new(file_name);
    }
    else read_matlab_file_new();

    last_sample_count = ui->sb_sample_count->value();
}

//*****************************************************************************************************************

void MainWindow::on_dsb_from_valueChanged(double arg1)
{
    if(read_fiff_changed) return;

    read_fiff_changed = true;
    _from = floor((arg1 - _offset_time) * _sample_rate);
    if(_from < 0)//stay consistent despite negative time values
    {
        ui->dsb_from->setValue(_offset_time);
        _from = 0;
    }
    _to = _from + ui->sb_sample_count->value() - 1;

    if(_to >= _last_sample - _offset_time * _sample_rate)
    {
        _to = _last_sample - _offset_time * _sample_rate;
        _samplecount = _to - _from + 1;
        ui->sb_sample_count->setValue(_samplecount);
    }

    ui->dsb_to->setValue(_to / _sample_rate + _offset_time);
    read_fiff_changed = false;

    ui->dsb_from->setToolTip(QString("sample: %1").arg(_from));
}

//*****************************************************************************************************************


void MainWindow::on_dsb_to_valueChanged(double arg1)
{
    if(read_fiff_changed) return;

    read_fiff_changed = true;
    _to = floor((arg1 - _offset_time) * _sample_rate);
    if(_to > _last_sample - _offset_time * _sample_rate)
    {
        ui->dsb_to->setValue(_last_sample / _sample_rate);
        _to = _last_sample - _offset_time * _sample_rate;
    }
    _from = _to - ui->sb_sample_count->value() + 1;

    if(_from + _offset_time * _sample_rate <= _first_sample)
    {
        _from = _first_sample - _offset_time * _sample_rate;
        _samplecount = _to - _from + 1;
        ui->sb_sample_count->setValue(_samplecount);
    }

    ui->dsb_from->setValue(_from / _sample_rate + _offset_time);
    read_fiff_changed = false;

     ui->dsb_to->setToolTip(QString("sample: %1").arg(_to));
}

//*****************************************************************************************************************

void MainWindow::on_sb_sample_count_valueChanged(int arg1)
{
    ui->sb_sample_count->setToolTip(QString("epoch: %1 sec").arg((arg1) / _sample_rate));

    if(read_fiff_changed) return;

    read_fiff_changed = true;
    _to = _from + arg1 - 1;

    if(_to > _last_sample - _offset_time * _sample_rate)
    {
        _to = _last_sample - _offset_time * _sample_rate;
        _samplecount = _to - _from + 1;
        ui->sb_sample_count->setValue(_samplecount);
    }
    _samplecount = arg1;
    ui->dsb_to->setValue(_to / _sample_rate + _offset_time);
    read_fiff_changed = false;

}

//*****************************************************************************************************************

void MainWindow::on_cb_all_select_clicked()
{
    if(tbv_is_loading) return;

    if( ui->cb_all_select->checkState() == Qt::Unchecked && !was_partialchecked)
    {
        ui->cb_all_select->setCheckState(Qt::PartiallyChecked);
        was_partialchecked = true;
    }
    else if(ui->cb_all_select->checkState() == Qt::Checked && !was_partialchecked)
    {
        ui->cb_all_select->setCheckState(Qt::Unchecked);
        was_partialchecked = false;
    }

    auto_change = true;

    if(ui->cb_all_select->checkState() == Qt::Checked)
        for(qint32 i = 0; i < ui->tbv_Results->rowCount() - 1; i++)     // last item is residuum
            ui->tbv_Results->item(i, 0)->setCheckState(Qt::Checked);
    else if(ui->cb_all_select->checkState() == Qt::Unchecked)
        for(qint32 i = 0; i < ui->tbv_Results->rowCount() - 1; i++)     // last item is residuum
            ui->tbv_Results->item(i, 0)->setCheckState(Qt::Unchecked);
    else
    {
        for(qint32 i = 0; i < ui->tbv_Results->rowCount() - 1; i++)     // last item is residuum
            if(select_atoms_map[i] == true)
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
        was_partialchecked = true;
    }
    else ui->cb_all_select->setCheckState(Qt::PartiallyChecked);

    auto_change = false;
}

//*****************************************************************************************************************

void MainWindow::on_dicts_saved()
{
    fill_dict_combobox();
}

//*****************************************************************************************************************

void MainWindow::on_actionSpeicher_triggered()
{
    QString save_name = "";
    QStringList saveList = file_name.split('/').last().split('.').first().split('_');
    for(int i = 0; i < saveList.length(); i++)
    {
        if(i == saveList.length() - 1)
            save_name += "mp_" + saveList.at(i);
        else
            save_name += saveList.at(i) + "_";
    }

    if(file_name.split('.').last() != "fif")
    {
        if(save_path.isEmpty())
        {
            save_path = QFileDialog::getSaveFileName(this, "Save file as...", last_save_path + "/" + save_name,"(*.txt)");
            if(save_path.isEmpty()) return;
        }
        save_parameters();
        QMessageBox::information(this, tr("information"),
        tr("No fif file for save. Only parameter.txt saved."));
        return;
    }

    if(save_path.isEmpty())
    {
        save_path = QFileDialog::getSaveFileName(this, "Save file as...", last_save_path + "/" + save_name,"(*.fif)");
        if(save_path.isEmpty()) return;
    }

    QStringList string_list = save_path.split('/');
    last_save_path = "";
    for(qint32 i = 0; i < string_list.length() - 1; i++)
        last_save_path += string_list.at(i) + '/';

    save_fif_file();
}

//*****************************************************************************************************************

void MainWindow::on_actionSpeicher_unter_triggered()
{  
    QString save_name = "";
    QStringList saveList = file_name.split('/').last().split('.').first().split('_');
    for(int i = 0; i < saveList.length(); i++)
    {
        if(i == saveList.length() - 1) save_name += "mp_" + saveList.at(i);
        else save_name += saveList.at(i) + "_";
    }

    if(file_name.split('.').last() != "fif")
        save_path = QFileDialog::getSaveFileName(this, "Save file as...", last_save_path + "/" + save_name,"(*.txt)");
    else
        save_path = QFileDialog::getSaveFileName(this, "Save file as...", last_save_path + "/" + save_name,"(*.fif)");

    if(save_path.isEmpty()) return;

    // savepath without filename, memorise last savepath
    QStringList string_list = save_path.split('/');
    last_save_path = "";
    for(qint32 i = 0; i < string_list.length() - 1; i++)
        last_save_path += string_list.at(i) + '/';

    save_fif_file();
}

//*****************************************************************************************************************

void MainWindow::save_fif_file()
{
    save_parameters();

    SaveFifFile *save_Fif = new SaveFifFile();
    QThread *save_thread = new QThread();

    save_Fif->moveToThread(save_thread);

    connect(this, SIGNAL(to_save(QString, QString, fiff_int_t, fiff_int_t, MatrixXd, MatrixXd, select_map, RowVectorXi, source_file_type )),
            save_Fif, SLOT(save_fif_file(QString, QString, fiff_int_t, fiff_int_t, MatrixXd, MatrixXd, select_map, RowVectorXi, source_file_type )));
    connect(save_Fif, SIGNAL(save_progress(qint32, qint32)), this, SLOT(recieve_save_progress(qint32, qint32)));
    connect(this, SIGNAL(kill_save_thread()), save_thread, SLOT(quit()));
    connect(this, SIGNAL(kill_save_thread()), save_Fif, SLOT(deleteLater()));
    //connect(save_thread, SIGNAL(finished()), save_Fif, SLOT(deleteLater()));
    connect(save_thread, SIGNAL(finished()), save_thread, SLOT(deleteLater()));
    connect(save_Fif, SIGNAL(finished()), save_thread, SLOT(deleteLater()));

    is_saved = true;
    ui->lb_timer->setHidden(true);
    ui->cb_all_select->setHidden(true);
    ui->progress_bar_save->setHidden(false);
    ui->progress_bar_save->setFormat("save fif file:  %p%");
    ui->progress_bar_save->setValue(0);
    ui->progress_bar_save->setMinimum(0);
    ui->progress_bar_save->setMaximum(_last_sample);
    //reset progressbar text color to black
    pal.setColor(QPalette::Text, Qt::black);
    ui->progress_bar_save->setPalette(pal);
    //ui->progress_bar_save->repaint();
    is_save_white = false;

    ui->actionSpeicher->setEnabled(false);
    ui->actionSpeicher_unter->setEnabled(false);

    emit to_save(file_name, save_path, _from, _to, _atom_sum_matrix, reference_matrix, select_channel_map, picks, file_type);
    save_thread->start();
}

//*****************************************************************************************************************

void MainWindow::recieve_save_progress(qint32 current_progress, qint32 finished)
{
    if(finished == 0)   //save in progress
    {
        ui->progress_bar_save->setValue(current_progress);
        if(ui->progress_bar_save->value() > ui->progress_bar_save->maximum() / 2 && !is_save_white)
        {
            pal.setColor(QPalette::Text, Qt::white);
            ui->progress_bar_save->setPalette(pal);
            is_save_white = true;
        }
    }
    else if(finished == 2)  // error on save
    {
        QMessageBox::warning(this, "Error", "error: no success on save.");
        ui->progress_bar_save->setHidden(true);
        emit kill_save_thread();
    }
    else if(finished == 4) // save .txt instead of ave.fif as long as mne-cpp do not serve that function
        QMessageBox::warning(this, "Error", "error: unable to save -ave.fif files\nDecomposition data saved to:\n"  + save_path + ".txt");

    else    //save is successfully finished
    {
        ui->progress_bar_save->setHidden(true);        
        ui->actionSpeicher->setEnabled(true);
        ui->actionSpeicher_unter->setEnabled(true);

        emit kill_save_thread();
    }
}
//*****************************************************************************************************************

void SaveFifFile::save_fif_file(QString source_path, QString save_path, fiff_int_t start_change, fiff_int_t end_change, MatrixXd changes, MatrixXd original_signal,
                                select_map select_channel_map, RowVectorXi picks, source_file_type file_type)
{
    QFile t_fileIn(source_path);
    QFile t_fileOut(save_path);

    switch(file_type)
    {
        case RAW: // if(QString::compare(source_path.split('.').last(), "fif", Qt::CaseInsensitive) == 0)
        {
            //   Setup for reading the raw data
            FiffRawData raw(t_fileIn);

            RowVectorXd cals;
            FiffStream::SPtr outfid = FiffStream::start_writing_raw(t_fileOut, raw.info, cals, picks);

            //   Set up the reading parameters
            fiff_int_t from = raw.first_samp;
            fiff_int_t to = raw.last_samp;
            float quantum_sec = 10.0f;//read and write in 10 sec junks
            fiff_int_t quantum = ceil(quantum_sec*raw.info.sfreq);  //   To read the whole file at once set quantum     = to - from + 1;

            //   Read and write all the data
            //************************************************************************************
            bool first_buffer = true;
            fiff_int_t first;
            fiff_int_t last;
            MatrixXd data;
            MatrixXd times;

            // from 0 to start of change
            for(first = from; first < start_change; first += quantum)
            {
                last = first + quantum - 1;
                if (last > start_change)
                {
                    last = start_change - 1;
                }
                if (!raw.read_raw_segment(data ,times, first, last, picks))
                {
                    printf("error during read_raw_segment\n");
                    emit save_progress(first, 2);
                    return;
                }
                printf("Writing...");
                if (first_buffer)
                {
                    if (first > 0)
                        outfid->write_int(FIFF_FIRST_SAMPLE, &first);
                    first_buffer = false;
                }
                outfid->write_raw_buffer(data, cals);
                printf("[done]\n");

                emit save_progress(first, 0);
            }

            //************************************************************************************

            // from start of change to end of change
            if (!raw.read_raw_segment(data, times, start_change ,end_change,picks))
            {
                printf("error during read_raw_segment\n");
                emit save_progress(first, 2);
                return;
            }

            qint32 index = 0;
            for(qint32 channels = 0; channels < data.rows(); channels++)
            {
                if(select_channel_map[channels])
                {
                    data.row(channels) =  changes.col(index)  ;
                    index++;
                }
            }
            printf("Writing new data...");
            if (first_buffer)
            {
                if (start_change > 0)
                    outfid->write_int(FIFF_FIRST_SAMPLE, &start_change);
                first_buffer = false;
            }
            outfid->write_raw_buffer(data, cals);
            printf("[done]\n");


            //************************************************************************************

            // from end of change to end
            for(first = end_change + 1; first < to; first += quantum)
            {
                last = first + quantum - 1;
                if (last > to)
                {
                    last = to;
                }
                if (!raw.read_raw_segment(data, times, first, last, picks))
                {
                    printf("error during read_raw_segment\n");
                    emit save_progress(first, 2);
                    return;
                }
                printf("Writing...");
                outfid->write_raw_buffer(data, cals);
                printf("[done]\n");

                emit save_progress(first, false);
            }

            emit save_progress(to, true);

            outfid->finish_writing_raw();
            printf("Finished\n");
            break;
        }

        case AVE: // if(source_path.contains("-ave.", Qt::CaseInsensitive))
        {
        std::cout << "thread\n";
            //temporary ToDo: revert this temp if saving AVE is possible in MNE-CPP
            emit save_progress(0, 4);

            t_fileOut.setFileName(save_path.append(".txt"));

            if (t_fileOut.open(QFile::WriteOnly | QFile::Truncate))
            {
                QTextStream matlab_stream(&t_fileOut);
                qint32 channel_index = 0;
                for(qint32 channel = 0; channel < original_signal.cols(); channel++)
                {
                    qint32 sample_index = 0;
                    if(select_channel_map[channel])//changes in this channels
                    {
                        for(qint32 sample = 0; sample < original_signal.rows(); sample++)
                            if(sample >= start_change && sample < end_change)
                            {
                                matlab_stream << QString::number(changes(sample_index, channel_index)) << ",";
                                sample_index++;
                            }
                            else  matlab_stream << QString::number(original_signal(sample, channel)) << ",";
                        matlab_stream<< "\n";
                        channel_index++;
                    }                            
                    else //no changes in this channel, just save original channel
                    {
                        for(qint32 sample = 0; sample < original_signal.rows(); sample++)
                            matlab_stream << QString::number(original_signal(sample, channel)) << ",";
                        matlab_stream<< "\n";
                    }
                    emit save_progress((channel + 1) * (original_signal.rows() /original_signal.cols()), 0);
                }
            }
            emit save_progress(original_signal.rows(), true);
            //end temporary

            break;
        }

        case TXT: // if(QString::compare(source_path.split('.').last(), "txt", Qt::CaseInsensitive) == 0)
        {
            if (t_fileOut.open(QFile::WriteOnly | QFile::Truncate))
            {
                QTextStream matlab_stream(&t_fileOut);

                qint32 index = 0;
                for(qint32 channel = 0; channel < _matlab_channels.length(); channel++)
                {
                    if(select_channel_map[channel])//changes in this channels
                    {
                        QStringList signal_samples = _matlab_channels.at(channel).split(',', QString::SkipEmptyParts);
                        for(qint32 sample = start_change; sample <= end_change; sample++)
                        {
                            signal_samples.replace(sample, QString::number(changes(sample - start_change, index)));//ToDo: ready
                        }
                        for(qint32 sample = 0; sample < signal_samples.length() - 1; sample++)
                            matlab_stream << signal_samples.at(sample) << ",";
                        matlab_stream << signal_samples.last();
                        matlab_stream << "\n";   //next channel
                        index++;
                    }
                    else //no changes in this channel, just save original channel
                    {
                        matlab_stream << _matlab_channels.at(channel);
                        matlab_stream << "\n";   //next channel
                    }
                    emit save_progress((channel + 1) * (_matlab_channels.at(0).split(',', QString::SkipEmptyParts).length() /_matlab_channels.length()), 0);
                }
            }
            emit save_progress(_matlab_channels.at(0).split(',', QString::SkipEmptyParts).length(), true);
            break;
        }

        default:
        {
            emit save_progress(0, 2);
            return;
        }
    }
}

//*****************************************************************************************************************

void MainWindow::save_parameters()
{
    QString save_parameter_path = save_path.split(".").first() + "_params.txt";
    QString original_file_name = file_name.split("/").last().split(".").first();
    QFile xml_file(save_parameter_path);
    if(xml_file.open(QIODevice::WriteOnly))
    {
        QXmlStreamWriter xmlWriter(&xml_file);
        xmlWriter.setAutoFormatting(true);
        xmlWriter.writeStartDocument();
        xmlWriter.writeStartElement("MP_DECOMPOSITION");
        xmlWriter.writeAttribute("fiff_file_name", original_file_name);
        xmlWriter.writeAttribute("epoch_from", QString::number(ui->dsb_from->value()));
        xmlWriter.writeAttribute("epoch_to", QString::number(ui->dsb_to->value()));
        xmlWriter.writeAttribute("samples", QString::number(ui->sb_sample_count->value()));
        xmlWriter.writeAttribute("sample_rate", QString::number(ui->dsb_sample_rate->value()));

        for(qint32 i = 0; i < ui->tbv_Results->rowCount() - 1; i++)
        {
            if(ui->tbv_Results->columnCount() == 2 && ui->tbv_Results->item(i, 1)->text() != "residuum")
            {
                FixDictAtom fix_atom = _fix_dict_atom_list.at(i);

                xmlWriter.writeStartElement("ATOM");
                xmlWriter.writeAttribute("formula", fix_atom.atom_formula);
                xmlWriter.writeAttribute("sample_count", QString::number(fix_atom.sample_count));
                xmlWriter.writeAttribute("%energy_from_signal", ui->tbv_Results->item(i, 0)->text());
                xmlWriter.writeAttribute("dict_source", fix_atom.dict_source);
                xmlWriter.writeStartElement("PARAMETER");

                if(fix_atom.type == GABORATOM)
                {
                    xmlWriter.writeAttribute("scale", QString::number(fix_atom.gabor_atom.scale));
                    xmlWriter.writeAttribute("translation", QString::number(fix_atom.translation));
                    xmlWriter.writeAttribute("modulation", QString::number(fix_atom.gabor_atom.modulation));
                    xmlWriter.writeAttribute("phase", QString::number(fix_atom.gabor_atom.phase));
                }
                else if(fix_atom.type == CHIRPATOM)
                {                   
                    xmlWriter.writeAttribute("scale", QString::number(fix_atom.chirp_atom.scale));
                    xmlWriter.writeAttribute("translation", QString::number(fix_atom.translation));
                    xmlWriter.writeAttribute("modulation", QString::number(fix_atom.chirp_atom.modulation));
                    xmlWriter.writeAttribute("phase", QString::number(fix_atom.chirp_atom.phase));
                    xmlWriter.writeAttribute("chirp", QString::number(fix_atom.chirp_atom.chirp));
                }
                else if(fix_atom.type == FORMULAATOM)
                {                    
                    xmlWriter.writeAttribute("translation", QString::number(fix_atom.translation));
                    xmlWriter.writeAttribute("a", QString::number(fix_atom.formula_atom.a));
                    xmlWriter.writeAttribute("b", QString::number(fix_atom.formula_atom.b));
                    xmlWriter.writeAttribute("c", QString::number(fix_atom.formula_atom.c));
                    xmlWriter.writeAttribute("d", QString::number(fix_atom.formula_atom.d));
                    xmlWriter.writeAttribute("e", QString::number(fix_atom.formula_atom.e));
                    xmlWriter.writeAttribute("f", QString::number(fix_atom.formula_atom.f));
                    xmlWriter.writeAttribute("g", QString::number(fix_atom.formula_atom.g));
                    xmlWriter.writeAttribute("h", QString::number(fix_atom.formula_atom.h));
                }
                xmlWriter.writeEndElement();    //PARAMETER
                xmlWriter.writeEndElement();    //ATOM
            }
            else //ToDo: does not work for trial separation like this
            {
                GaborAtom gabor_atom = _adaptive_atom_list.at(i).last();
                xmlWriter.writeStartElement("ATOM");
                xmlWriter.writeAttribute("formula", "GABORATOM");
                xmlWriter.writeAttribute("sample_count", QString::number(gabor_atom.sample_count));
                xmlWriter.writeAttribute("%energy_from_signal", ui->tbv_Results->item(i, 0)->text());

                xmlWriter.writeStartElement("MATHEMATICAL_PARAMETERS");
                xmlWriter.writeAttribute("scale", QString::number(gabor_atom.scale));
                xmlWriter.writeAttribute("translation", QString::number(gabor_atom.translation));
                xmlWriter.writeAttribute("modulation", QString::number(gabor_atom.modulation));
                xmlWriter.writeAttribute("phase", QString::number(gabor_atom.phase));
                xmlWriter.writeEndElement();    //PARAMETER

                xmlWriter.writeStartElement("PHYSICAL_PARAMETERS");
                xmlWriter.writeAttribute("scale", QString::number(gabor_atom.scale / _sample_rate, 'g', 3));
                xmlWriter.writeAttribute("translation", QString::number(gabor_atom.translation / qreal(_sample_rate) + _from  / _sample_rate, 'g', 4));
                xmlWriter.writeAttribute("modulation", QString::number(gabor_atom.modulation * _sample_rate / gabor_atom.sample_count, 'g', 3));

                qreal phase = gabor_atom.phase;
                if(phase > 2*PI) phase -= 2*PI;
                xmlWriter.writeAttribute("phase", QString::number(phase, 'g', 3));
                xmlWriter.writeEndElement();    //PARAMETER
                xmlWriter.writeEndElement();    //ATOM
            }
        }
        xmlWriter.writeEndElement();    //MP_DECOMPOSITION
        xmlWriter.writeEndDocument();
    }
    xml_file.close();
}

//*****************************************************************************************************************

void MainWindow::on_actionExport_triggered()
{
    if(_adaptive_atom_list.length() == 0)
    {
        QMessageBox::warning(this, tr("Error"),
        tr("error: No adaptive MP results for save."));
        return;
    }

    QString save_path = QFileDialog::getSaveFileName(this, "Export results as dict file...", QDir::homePath() + "/" + "Matching-Pursuit-Toolbox" + "/" + "resultdict","(*.dict)");
    if(save_path.isEmpty()) return;

    QStringList string_list = save_path.split('/');
    last_save_path = "";
    for(qint32 i = 0; i < string_list.length() - 1; i++)
        last_save_path += string_list.at(i) + '/';

    QSettings settings;
    qint32 pdict_count = settings.value("pdict_count", 8).toInt();

    QFile xml_file(save_path);
    if(xml_file.open(QIODevice::WriteOnly))
    {
        QXmlStreamWriter xmlWriter(&xml_file);
        xmlWriter.setAutoFormatting(true);
        xmlWriter.writeStartDocument();

        xmlWriter.writeStartElement("COUNT");
        xmlWriter.writeAttribute("of_atoms", QString::number(_adaptive_atom_list.length() * _adaptive_atom_list.first().length()));

        qint16 built_count = 0;
        qint32 div = floor(_adaptive_atom_list.length() / (qreal)pdict_count);
        qint32 mod = _adaptive_atom_list.length() % pdict_count;
            if(div != 0)
            {
                for(qint32 j = 0; j < pdict_count; j++)
                {
                    xmlWriter.writeStartElement("built_Atoms");
                    xmlWriter.writeAttribute("formula", "Gaboratom");
                    xmlWriter.writeAttribute("sample_count", QString::number(_adaptive_atom_list.first().first().sample_count));
                    xmlWriter.writeAttribute("atom_count", QString::number(div * _adaptive_atom_list.first().length()));
                    xmlWriter.writeAttribute("source_dict", save_path.split('/').last().split('.').first() + "_" + QString::number(j));

                    for(qint32 i = 0; i < div; i++)
                    {
                        for(qint32 chn = 0; chn < _adaptive_atom_list.first().length(); chn++)
                        {
                            GaborAtom gabor_atom = _adaptive_atom_list.at(i + j * div).at(chn);
                            QStringList result_list = gabor_atom.create_string_values(gabor_atom.sample_count, gabor_atom.scale, gabor_atom.sample_count / 2, gabor_atom.modulation, gabor_atom.phase);

                            xmlWriter.writeStartElement("ATOM");
                            xmlWriter.writeAttribute("ID", QString::number(i * _adaptive_atom_list.first().length() + chn));
                            xmlWriter.writeAttribute("scale", QString::number(gabor_atom.scale));
                            xmlWriter.writeAttribute("modu", QString::number(gabor_atom.modulation));
                            xmlWriter.writeAttribute("phase", QString::number(gabor_atom.phase));

                            xmlWriter.writeStartElement("samples");
                            QString samples_to_xml;
                            for (qint32 it = 0; it < result_list.length(); it++)
                            {
                                samples_to_xml.append(result_list.at(it));
                                samples_to_xml.append(":");
                            }
                            xmlWriter.writeAttribute("samples", samples_to_xml);
                            xmlWriter.writeEndElement();    //samples

                            xmlWriter.writeEndElement();    //ATOM
                        }
                        built_count++;
                    }
                    xmlWriter.writeEndElement();    //built_atoms
                } //builds
            }

            if(mod != 0)
            {
                xmlWriter.writeStartElement("built_Atoms");
                xmlWriter.writeAttribute("formula", "Gaboratom");
                xmlWriter.writeAttribute("sample_count", QString::number(_adaptive_atom_list.first().first().sample_count));
                xmlWriter.writeAttribute("atom_count", QString::number(mod * _adaptive_atom_list.first().length()));
                xmlWriter.writeAttribute("source_dict", save_path.split('/').last().split('.').first()  + "_" + QString::number(built_count));

                for(qint32 i = 0; i < mod; i++)
                {
                    for(qint32 chn = 0; chn < _adaptive_atom_list.first().length(); chn++)
                    {
                        GaborAtom gabor_atom = _adaptive_atom_list.at(i + pdict_count * div).at(chn);
                        QStringList result_list = gabor_atom.create_string_values(gabor_atom.sample_count, gabor_atom.scale, gabor_atom.sample_count / 2, gabor_atom.modulation, gabor_atom.phase);

                        xmlWriter.writeStartElement("ATOM");
                        xmlWriter.writeAttribute("ID", QString::number(i * _adaptive_atom_list.first().length() + chn));
                        xmlWriter.writeAttribute("scale", QString::number(gabor_atom.scale));
                        xmlWriter.writeAttribute("modu", QString::number(gabor_atom.modulation));
                        xmlWriter.writeAttribute("phase", QString::number(gabor_atom.phase));

                        xmlWriter.writeStartElement("samples");
                        QString samples_to_xml;
                        for (qint32 it = 0; it < result_list.length(); it++)
                        {
                            samples_to_xml.append(result_list.at(it));
                            samples_to_xml.append(":");
                        }
                        xmlWriter.writeAttribute("samples", samples_to_xml);
                        xmlWriter.writeEndElement();    //samples

                        xmlWriter.writeEndElement();    //ATOM
                    }
                }
                xmlWriter.writeEndElement();    //built_atoms
        }
        xmlWriter.writeEndElement();    //COUNT
        xmlWriter.writeEndDocument();
    }
    xml_file.close();
    fill_dict_combobox();
}

//*****************************************************************************************************************

bool MainWindow::sort_energy_adaptive(const QList<GaborAtom> atom_1, const QList<GaborAtom> atom_2)
{
    qreal energy_1 = 0;
    qreal energy_2 = 0;

    for(qint32 i = 0; i < atom_1.length(); i++)
    {
        energy_1 += atom_1.at(i).energy;
        energy_2 += atom_2.at(i).energy;
    }
    return (energy_1 > energy_2);
}

//*****************************************************************************************************************

bool MainWindow::sort_energy_fix(const FixDictAtom &atom_1, const FixDictAtom &atom_2)
{
    return (atom_1.energy > atom_2.energy);
}

//*****************************************************************************************************************

void MainWindow::on_cb_Dicts_currentIndexChanged(const QString &arg1)
{
    Q_UNUSED(arg1);
    ui->lb_info_content->clear();
    //ui->lb_info_content->repaint();
    has_warning = false;
}

//*****************************************************************************************************************

void MainWindow::on_rb_adativMp_clicked()
{
    ui->cb_Dicts->setEnabled(false);
    ui->lb_info_content->clear();
    if(_has_file)
        ui->btt_Calc->setEnabled(true);
    //ui->lb_info_content->repaint();
    has_warning = false;
}

//*****************************************************************************************************************

void MainWindow::activate_info_label()
{
    QSettings settings;
    if(!settings.value("show_infos", true).toBool())
    {
        ui->lb_info_content->setHidden(true);
        ui->lb_info->setHidden(true);
    }
    else
    {
        ui->lb_info_content->setHidden(false);
        ui->lb_info->setHidden(false);
    }
}

//*****************************************************************************************************************

void MainWindow::on_dsb_energy_valueChanged(double arg1)
{
    if(arg1 > 99.9)
        ui->dsb_energy->setValue(99.9);
}

//*****************************************************************************************************************

void MainWindow::on_actionBeenden_triggered()
{
    close();
}

//*****************************************************************************************************************

void GraphWindow::mouseMoveEvent(QMouseEvent *event)
{
   if(_to - _from != 0)
   {
       qint32 temp_pos_x = mapFromGlobal(QCursor::pos()).x() - 55;
       qint32 temp_pos_y = this->height() - mapFromGlobal(QCursor::pos()).y();

       qreal stretch_factor_x = (this->width() - 55/*left_margin*/ - 15/*right_margin*/) / qreal(_samplecount - 1);
       qreal stretch_factor_y = (this->height() - _border_margin_height) / _signal_maximum;

       qreal time = _from / _sample_rate + _offset_time + temp_pos_x / stretch_factor_x / _sample_rate;
       qreal amplitude =  temp_pos_y / stretch_factor_y - (this->height() + _x_axis_height) / stretch_factor_y;

       if(mapFromGlobal(QCursor::pos()).x() >= 55 && mapFromGlobal(QCursor::pos()).x() <= (this->width() - 15))
           this->setToolTip(QString("time: %1 sec\namplitude: %2").arg(time).arg(QString::number(amplitude, 'g', 3)));
       else this->setToolTip(QString("amplitude: %1").arg(QString::number(amplitude, 'g', 3)));

       if(event->buttons() == Qt::LeftButton)
               setCursor(Qt::ClosedHandCursor);
       else
        setCursor(Qt::CrossCursor);
   }
}

//*****************************************************************************************************************

void AtomSumWindow::mouseMoveEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    if(_to - _from != 0)
    {
        qint32 temp_pos_x = mapFromGlobal(QCursor::pos()).x() - 55;
        qint32 temp_pos_y = this->height() - mapFromGlobal(QCursor::pos()).y();

        qreal stretch_factor_x = (this->width() - 55/*left_margin*/ - 15/*right_margin*/) / qreal(_samplecount - 1);
        qreal stretch_factor_y = (this->height() - _border_margin_height) / _signal_maximum;

        qreal time = _from / _sample_rate + _offset_time + temp_pos_x / stretch_factor_x / _sample_rate;
        qreal amplitude =  temp_pos_y / stretch_factor_y - (this->height() + _x_axis_height) / stretch_factor_y;

        if(mapFromGlobal(QCursor::pos()).x() >= 55 && mapFromGlobal(QCursor::pos()).x() <= (this->width() - 15))
            this->setToolTip(QString("time: %1 sec\namplitude: %2").arg(time).arg(QString::number(amplitude, 'g', 3)));
        else this->setToolTip(QString("amplitude: %1").arg(QString::number(amplitude, 'g', 3)));

       setCursor(Qt::CrossCursor);
   }
}

//*****************************************************************************************************************

void ResiduumWindow::mouseMoveEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    if(_to - _from != 0)
    {
        qint32 temp_pos_x = mapFromGlobal(QCursor::pos()).x() - 55;
        qint32 temp_pos_y = this->height() - mapFromGlobal(QCursor::pos()).y();

        qreal stretch_factor_x = (this->width() - 55/*left_margin*/ - 15/*right_margin*/) / qreal(_samplecount - 1);
        qreal stretch_factor_y = (this->height() - _border_margin_height) / _signal_maximum;

        qreal time = _from / _sample_rate + _offset_time + temp_pos_x / stretch_factor_x / _sample_rate;
        qreal amplitude =  temp_pos_y / stretch_factor_y - (this->height() + _x_axis_height) / stretch_factor_y;

        if(mapFromGlobal(QCursor::pos()).x() >= 55 && mapFromGlobal(QCursor::pos()).x() <= (this->width() - 15))
            this->setToolTip(QString("time: %1 sec\namplitude: %2").arg(time).arg(QString::number(amplitude, 'g', 3)));
        else this->setToolTip(QString("amplitude: %1").arg(QString::number(amplitude, 'g', 3)));

        setCursor(Qt::CrossCursor);
    }
}

//*****************************************************************************************************************

void GraphWindow::mousePressEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    if(_to - _from != 0)
    {
        _press_pos = mapFromGlobal(QCursor::pos()).x();
        setCursor(Qt::ClosedHandCursor);
    }
}

//*****************************************************************************************************************

void GraphWindow::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event);

    if(_to - _from != 0)
    {
        fiff_int_t release_pos = mapFromGlobal(QCursor::pos()).x();
        qreal stretch_factor = qreal(this->width() - 55 - 15) / (qreal)(_samplecount);
        qint32 old_from = _from;
        qint32 old_to = _to;
        _from += floor((_press_pos - release_pos) / stretch_factor);
        _to = _from + _samplecount - 1;
        if(_from < _first_sample - _offset_time * _sample_rate)
        {
            _from = _first_sample - _offset_time * _sample_rate;
             _to = _from + _samplecount - 1;
        }

        if(_to > _last_sample - _offset_time * _sample_rate)
        {
            _to = _last_sample - _offset_time * _sample_rate;
            _from = _to - _samplecount + 1;
        }

        setCursor(Qt::CrossCursor);

        // +/- 5 pixel dont read new if clicked by mistake
        if(std::abs(_press_pos - release_pos) < 5 || old_from == _from || old_to == _to)
            return;

        emit read_new();
    }

}

//*****************************************************************************************************************

void GraphWindow::wheelEvent(QWheelEvent *event)
{

    if(_to - _from != 0)
    {
        _samplecount -= event->angleDelta().y() / 1.875 *  _samplecount / 2048;

        if(_samplecount  > 4096)
            _samplecount = 4096;

        if(_samplecount < 64)
            _samplecount = 64;
        if(_samplecount == _to - _from + 1)
            return;

        qreal stretch_factor = qreal(this->width() - 55/*left_margin*/ - 15/*right_margin*/) / (qreal)(_to - _from);
        qint32 temp_pos = mapFromGlobal(QCursor::pos()).x() - 55;
        qint32 actual_sample = _from + floor((qreal)temp_pos / stretch_factor);
        qint32 delta_from = (qreal)_samplecount * ((qreal)temp_pos / (qreal)(this->width() - 70));

        _from = actual_sample - delta_from;

        _to = _from + _samplecount - 1;

        if(_from < _first_sample - _offset_time * _sample_rate)
        {
            _from = _first_sample  - _offset_time * _sample_rate;
            _samplecount = _to - _from + 1;
        }

        if(_to > _last_sample - _offset_time * _sample_rate)
        {
            _to = _last_sample - _offset_time * _sample_rate;
            _samplecount = _to - _from + 1;
        }
        emit read_new();
    }
}

//*****************************************************************************************************************

void MainWindow::on_mouse_button_release()
{
    if(QString::compare(ui->btt_Calc->text(), "cancel", Qt::CaseInsensitive) == 0 || QString::compare(ui->btt_Calc->text(), "wait...", Qt::CaseInsensitive) == 0)
        return;
    read_fiff_changed = true;

    ui->dsb_from->setValue(_from / _sample_rate + _offset_time);
    ui->dsb_to->setValue(_to / _sample_rate + _offset_time);
    ui->sb_sample_count->setValue(_samplecount);

    read_fiff_changed = false;

    if(file_name.split('.').last() == "fif")
    {
        if(!_pick_evoked.isEmpty())
            read_fiff_ave_new();
        else
            read_fiff_file_new(file_name);
    }
    else
        read_matlab_file_new();
}

//*****************************************************************************************************************

void MainWindow::on_rb_OwnDictionary_clicked()
{
    ui->cb_Dicts->setEnabled(true);
    if(ui->cb_Dicts->itemText(0) == "")
        ui->btt_Calc->setEnabled(false);
    else if(_has_file)
        ui->btt_Calc->setEnabled(true);
}

//*****************************************************************************************************************

void MainWindow::on_actionTFplot_triggered()
{    
    if(ui->tabWidget->count() == 1)
    {       
        MatrixXd tf_sum;
        /*
        tf_sum = MatrixXd::Zero(floor(_adaptive_atom_list.first().first().sample_count/2), _adaptive_atom_list.first().first().sample_count);

        for(qint32 i = 0; i < _adaptive_atom_list.first().length(); i++)//foreach channel
        {
            for(qint32 j = 0; j < _adaptive_atom_list.length(); j++) //foreach atom
            {
                GaborAtom atom  = _adaptive_atom_list.at(j).at(i);
                MatrixXd tf_matrix = atom.make_tf(atom.sample_count, atom.scale, atom.translation, atom.modulation);

                tf_matrix *= atom.max_scalar_list.at(i)*atom.max_scalar_list.at(i);
                tf_sum += tf_matrix;
            }
        }
        */
        tf_sum = Spectrogram::makeSpectrogram(_signal_matrix.col(0), 0);

        TFplot *tfplot = new TFplot(tf_sum, _sample_rate, 0, 600, Jet);
        ui->tabWidget->addTab(tfplot, "TF-Overview 0-500Hz");
        ui->tabWidget->setCurrentIndex(1);
        tfplot->resize(ui->tabWidget->size());

        TFplot *tfplot2 = new TFplot(tf_sum, _sample_rate, 0, 100, Jet);
        ui->tabWidget->addTab(tfplot2, "TF-Overview 0-100Hz");

        ui->tabWidget->setCurrentIndex(2);
        tfplot2->resize(ui->tabWidget->size());


        TFplot *tfplot3 = new TFplot(tf_sum, _sample_rate, 301, 480, Jet);
        ui->tabWidget->addTab(tfplot3, "TF-Overview 300-480Hz");

        ui->tabWidget->setCurrentIndex(3);
        tfplot3->resize(ui->tabWidget->size());

         ui->tabWidget->setCurrentIndex(1);
         tfplot->resize(ui->tabWidget->size());

        QPushButton *extendedButton = new QPushButton();
        extendedButton->setMaximumSize(20, 20);
        extendedButton->setStyleSheet("QPushButton {margin-right: 2px;  border-width: 1px; border-radius: 1px; border-color: grey;} QPushButton:pressed {background-color: grey; border-radius: 10px;}");
        extendedButton->setIcon(QIcon(":/images/icons/expand_512.png"));
        extendedButton->setIconSize(QSize(16, 16));

        ui->tabWidget->tabBar()->setTabButton(1, QTabBar::LeftSide, extendedButton);
        connect(extendedButton, SIGNAL (released()), this, SLOT (on_extend_tab_button()));
    }    
}

//*****************************************************************************************************************

void MainWindow::on_extend_tab_button()
{
    //plot_window->show();
    //plot_window->setWindowTitle("Time-Frequency-Overview");
  //  QWidget *tf_overview_w = new QWidget();
  //  tf_overview_w->setWindowTitle("Time-Frequency-Overview");
  //  tf_overview_w->show();
    ui->tabWidget->removeTab(1);
    /*QLayout layout;// = new QLayout;
    layout->addWidget(plot_window);
    tf_overview_w->setLayout(layout);

 */
}

//*****************************************************************************************************************

void MainWindow::on_close_tab_button(int index)
{
    ui->tabWidget->removeTab(index);
}
