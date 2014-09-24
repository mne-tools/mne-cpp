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
#include <disp/plot.h>

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

bool _new_paint;

fiff_int_t _from;
fiff_int_t _to;
fiff_int_t _first_sample;
fiff_int_t _last_sample;

qint32 _samplecount;
qreal _max_pos;
qreal _max_neg;
qreal _sample_rate;
qreal _signal_maximum;
qreal _signal_negative_scale;

QList<QColor> _colors;
MatrixXd _signal_matrix;
MatrixXd _atom_sum_matrix;
MatrixXd _residuum_matrix;

//QTimer *_counter_timer;
//QThread* mp_Thread;
//AdaptiveMp *adaptive_Mp;
//FixDictMp *fixDict_Mp ;
Formulaeditor *_formula_editor;
EditorWindow *_editor_window;
Enhancededitorwindow *_enhanced_editor_window;
settingwindow *_setting_window;
TreebasedDictWindow *_treebased_dict_window;

//*************************************************************************************************************
//=============================================================================================================
// MAIN
//=============================================================================================================
MainWindow::MainWindow(QWidget *parent) :    QMainWindow(parent),    ui(new Ui::MainWindow)
{
    ui->setupUi(this);    

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

    if(!settings.value("show_infos", true).toBool())
    {
        ui->lb_info_content->setHidden(true);
        ui->lb_info->setHidden(true);
    }
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

    /*
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

void MainWindow::fill_dict_combobox()
{    
    QDir dir(QDir::homePath() + "/" + "Matching-Pursuit-Toolbox");
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
    QString temp_file_name = QFileDialog::getOpenFileName(this, "Please select signal file.", last_open_path,"(*.fif *.txt)");
    if(temp_file_name.isNull()) return;

    QStringList string_list = temp_file_name.split('/');
    last_open_path = "";
    for(qint32 i = 0; i < string_list.length() - 1; i++)
        last_open_path += string_list.at(i) + '/';
    file_name = temp_file_name;
     this->cb_model->clear();
    this->cb_items.clear();

    ui->dsb_sample_rate->setEnabled(true);

    QFile file(file_name);
    if (!file.open(QIODevice::ReadOnly))
    {
        QMessageBox::warning(this, tr("Error"),
        tr("error: disable to open signal file."));
        return;
    }
    file.close();

    _colors.clear();
    _colors.append(QColor(0, 0, 0));

    _from = -1;
    if(file_name.endsWith(".fif", Qt::CaseInsensitive))
    {
        read_fiff_ave(file_name);
        //if(!read_fiff_file(file_name)) return;

        ui->dsb_sample_rate->setEnabled(false);
        original_signal_matrix = _signal_matrix;
        fill_channel_combobox();
    }
    else
    {

        if(!read_matlab_file(file_name)) return;
        // ToDo: change samplerate
        //ui->dsb_sample_rate->setEnabled(false);
        ui->dsb_sample_rate->setEnabled(true);
    }
    //initial
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

    ui->dsb_from->setValue(_from / _sample_rate);
    ui->dsb_to->setValue(_to / _sample_rate);
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
    callXAxisWindow->setMinimumHeight(22);
    callXAxisWindow->setMaximumHeight(22);


    _atom_sum_matrix = MatrixXd::Zero(_signal_matrix.rows(), _signal_matrix.cols()); //resize
    _residuum_matrix = MatrixXd::Zero(_signal_matrix.rows(), _signal_matrix.cols()); //resize

    ui->progressBarCalc->reset();
    ui->progressBarCalc->setVisible(false);
    ui->lb_info_content->setText("");
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

void MainWindow::read_fiff_ave(QString file_name)
{
    QFile t_fileEvoked(file_name);

    fiff_int_t setno = 0;
    QPair<QVariant, QVariant> baseline(QVariant(), 0);
    FiffEvoked evoked(t_fileEvoked, setno, baseline);    
    if(evoked.isEmpty())
        return;

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

    //   Set up pick list: STI 014
    QStringList include;
    include << "STI 014";
    bool want_meg   = chn_name_map["MEG"].toBool();
    bool want_eeg   = chn_name_map["EEG"].toBool();
    bool want_stim  = chn_name_map["STI"].toBool();

    picks = evoked.info.pick_types(want_meg, want_eeg, want_stim);
    pick_info = evoked.info.pick_info(picks);

    QStringList filter_list;
    for(qint32 i = 0; i < evoked.info.ch_names.length(); i++)
    {
        for(qint32 k = 0; k < pick_list.length(); k++)
            if(evoked.info.ch_names.at(i).contains(pick_list.at(k)))
                filter_list.append(evoked.info.ch_names.at(i));
    }

    FiffEvoked pick_evoked = evoked.pick_channels(filter_list);

    ui->dsb_sample_rate->setValue(pick_evoked.info.sfreq);
    _sample_rate = pick_evoked.info.sfreq;

    _signal_matrix = MatrixXd::Zero(pick_evoked.data.cols(), pick_evoked.data.rows());

    for(qint32 channels = 0; channels <  pick_evoked.data.rows(); channels++)
        _signal_matrix.col(channels) = pick_evoked.data.row(channels);

    _from = _first_sample = 0;
    _to = _last_sample = _signal_matrix.rows();
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

    chn_name_map = settings.value("channel_names", chn_name_map).toMap();//<QString, bool>();
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

    //   Set up pick list: STI 014
    QStringList include;
    include << "STI 014";
    bool want_meg   = chn_name_map["MEG"].toBool();
    bool want_eeg   = chn_name_map["EEG"].toBool();
    bool want_stim  = chn_name_map["STI"].toBool();

    picks = raw.info.pick_types(want_meg, want_eeg, want_stim);

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
    ui->lb_info_content->setText("");
    ui->cb_all_select->setHidden(true);    
    ui->lb_timer->setHidden(true);
    ui->progressBarCalc->setHidden(true);
    ui->actionExport->setEnabled(false);

    has_warning = false;
    _new_paint = true;
    update();
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



    cout << "chn_name_lenght: " << chn_names.length() << "\n";
    cout << "channelcount: " << _signal_matrix.cols() << "\n";
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

// ToDo: change samplerate --> update times (_from _to)
bool MainWindow::read_matlab_file(QString fileName)
{
    QFile file(fileName);
    file.open(QIODevice::ReadOnly);

    QTextStream stream(&file);
    bool isFloat;
    qint32 row_number = 0;

    matlab_signal = stream.readAll().split('\n', QString::SkipEmptyParts);
    _first_sample = 0;
    _last_sample = matlab_signal.length() - 1;

    _from = _first_sample;
    if(_from + 511 <= _last_sample)
        _to = _from + 511;
    else
        _to = _last_sample;

    _signal_matrix = MatrixXd::Zero(_to - _from + 1, 1);

    for(qint32 i = _from; i < _to; i++)
    {
        qreal value = matlab_signal.at(i).toFloat(&isFloat);
        if(!isFloat)
        {
             _signal_matrix = MatrixXd::Zero(0, 0);
            QMessageBox::critical(this, "error", QString("error reading matlab file. Could not read line %1 from file %2.").arg(i).arg(fileName));
            return false;
        }
        _signal_matrix(row_number, 0) = value;
        row_number++;
    }
    file.close();

    return true;
}

//*************************************************************************************************************************************

void MainWindow::read_matlab_file_new()
{

    bool isFloat;
    qint32 row_number = 0;

    _signal_matrix = MatrixXd::Zero(_to - _from, 1);

    for(qint32 i = _from; i < _to; i++)
    {
        qreal value = matlab_signal.at(i).toFloat(&isFloat);
        if(!isFloat)
        {
             _signal_matrix = MatrixXd::Zero(0, 0);
        }
        _signal_matrix(row_number, 0) = value;
        row_number++;
    }

    _atom_sum_matrix = MatrixXd::Zero(_signal_matrix.rows(), _signal_matrix.cols()); //resize
    _residuum_matrix = MatrixXd::Zero(_signal_matrix.rows(), _signal_matrix.cols()); //resize

    ui->tbv_Results->clearContents();
    ui->tbv_Results->setRowCount(0);
    ui->actionSpeicher->setEnabled(false);
    ui->actionSpeicher_unter->setEnabled(false);
    ui->lb_info_content->setText("");
    ui->cb_all_select->setHidden(true);
    ui->lb_timer->setHidden(true);
    ui->progressBarCalc->setHidden(true);
    ui->actionExport->setEnabled(false);

    has_warning = false;
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
        qint32 maxStrLenght = 55;           // max lenght in pixel of x-axis string
        qint32 borderMarginHeigth = 15;     // reduce paintspace in GraphWindow of borderMargin pixels
        qint32 borderMarginWidth = 15;      // reduce paintspace in GraphWindow of borderMargin pixels
        qreal maxPos = 0.0;                 // highest signalvalue
        qreal maxNeg = 0.0;                 // smalest signalvalue
        qreal maxmax = 0.0;                 // absolute difference maxpos - maxneg

        qreal scaleYText = 0.0;
        qint32 negScale  = 0;
        if(_new_paint)
        {
            maxPos = signalMatrix.maxCoeff();
            maxNeg = signalMatrix.minCoeff();

            // absolute signalheight to globe
            if(maxNeg <= 0) maxmax = maxPos - maxNeg;
            else  maxmax = maxPos + maxNeg;

            _max_pos = maxPos;          // to globe max_pos
            _max_neg = maxNeg;          // to globe min_pos
            _signal_maximum = maxmax;   // to globe abs_max
            _new_paint = false;
        }

        // scale axis title
        scaleYText = _signal_maximum / 10.0;
        if(_max_neg < 0)
            negScale = floor((_max_neg * 10 / _signal_maximum) + 0.5);
        if(_max_pos <= 0) negScale = -10;
        _signal_negative_scale = negScale;  // to globe _signal_negative_scale

        // scale signal
        qreal scaleX = ((qreal)(windowSize.width() - maxStrLenght - borderMarginWidth))/ (qreal)signalMatrix.rows();
        qreal scaleY = (qreal)(windowSize.height() - borderMarginHeigth) / _signal_maximum;

        //scale axis
        qreal scaleXAchse = (qreal)(windowSize.width() - maxStrLenght - borderMarginWidth) / (qreal)20;
        qreal scaleYAchse = (qreal)(windowSize.height() - borderMarginHeigth) / (qreal)10;

        for(qint32 i = 1; i <= 11; i++)
        {
            if(negScale == 0)                                                       // x-Axis reached (y-value = 0)
            {
                // append scaled signalpoints
                for(qint32 channel = 0; channel < signalMatrix.cols(); channel++)   // over all Channels
                {
                    QPolygonF poly;
                    for(qint32 h = 0; h < signalMatrix.rows(); h++)
                        poly.append(QPointF((h * scaleX) + maxStrLenght,  -((signalMatrix(h, channel) * scaleY + ((i - 1) * scaleYAchse) - (windowSize.height()) + borderMarginHeigth / 2))));
                    QPen pen(_colors.at(channel), 0.5, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);
                    painter.setPen(pen);
                    painter.drawPolyline(poly);
                }
                // paint x-axis
                for(qint32 j = 1; j < 21; j++)
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
                }
                painter.drawLine(maxStrLenght, -(((i - 1) * scaleYAchse)-(windowSize.height()) + borderMarginHeigth / 2), windowSize.width()-5, -(((i - 1) * scaleYAchse)-(windowSize.height()) + borderMarginHeigth / 2));
            }
            painter.drawText(3, -((i - 1) * scaleYAchse - windowSize.height()) - borderMarginHeigth/2 + 4, QString::number(negScale * scaleYText, 'g', 3));     // paint scalevalue y-axis
            painter.drawLine(maxStrLenght - 2, -(((i - 1) * scaleYAchse)-(windowSize.height()) + borderMarginHeigth / 2), maxStrLenght + 2, -(((i - 1) * scaleYAchse)-(windowSize.height()) + borderMarginHeigth / 2));  // scalelines y-axis
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
        qint32 maxStrLenght = 55;
        qint32 borderMarginHeigth = 15;                     // reduce paintspace in GraphWindow of borderMargin pixels
        qint32 borderMarginWidth = 15;                      // reduce paintspace in GraphWindow of borderMargin pixels

        // scale axis title
        qreal scaleYText = (qreal)signalMaximum / (qreal)10;

        // scale signal
        qreal scaleX = ((qreal)(windowSize.width() - maxStrLenght - borderMarginWidth)) / (qreal)atom_matrix.rows();
        qreal scaleY = (qreal)(windowSize.height() - borderMarginHeigth) / (qreal)signalMaximum;

        //scale axis
        qreal scaleXAchse = (qreal)(windowSize.width() - maxStrLenght - borderMarginWidth) / (qreal)20;
        qreal scaleYAchse = (qreal)(windowSize.height() - borderMarginHeigth) / (qreal)10;     

        for(qint32 i = 1; i <= 11; i++)
        {
            if(signalNegativeMaximum == 0)                                          // x-Axis reached (y-value = 0)
            {
                // append scaled signalpoints
                for(qint32 channel = 0; channel < atom_matrix.cols(); channel++)    // over all Channels
                {
                    QPolygonF poly;                   
                    for(qint32 h = 0; h < atom_matrix.rows(); h++)
                        poly.append(QPointF((h * scaleX) + maxStrLenght,  -((atom_matrix(h, channel) * scaleY + ((i - 1) * scaleYAchse)-(windowSize.height()) + borderMarginHeigth / 2))));
                    QPen pen(_colors.at(channel), 0.5, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);
                    painter.setPen(pen);
                    painter.drawPolyline(poly);
                }                
                // paint x-axis                
                for(qint32 j = 1; j < 21; j++)
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
                }
                painter.drawLine(maxStrLenght, -(((i - 1) * scaleYAchse)-(windowSize.height()) + borderMarginHeigth / 2), windowSize.width()-5, -(((i - 1) * scaleYAchse)-(windowSize.height()) + borderMarginHeigth / 2));
            }

            painter.drawText(3, -((i - 1) * scaleYAchse - windowSize.height()) - borderMarginHeigth/2 + 4, QString::number(signalNegativeMaximum * scaleYText, 'g', 3));     // paint scalvalue Y-axis
            painter.drawLine(maxStrLenght - 2, -(((i - 1) * scaleYAchse)-(windowSize.height()) + borderMarginHeigth / 2), maxStrLenght + 2, -(((i - 1) * scaleYAchse)-(windowSize.height()) + borderMarginHeigth / 2));  // scalelines y-axis
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
        qint32 maxStrLenght = 55;
        qint32 borderMarginHeigth = 15;                 // reduce paintspace in GraphWindow of borderMargin pixels
        qint32 borderMarginWidth = 15;                  // reduce paintspace in GraphWindow of borderMargin pixels

        // scale axis title
        qreal scaleYText = (qreal)signalMaximum / (qreal)10;

        // scale signal
        qreal scaleX = ((qreal)(windowSize.width() - maxStrLenght - borderMarginWidth))/ (qreal)residuum_matrix.rows();
        qreal scaleY = (qreal)(windowSize.height() - borderMarginHeigth) / (qreal)signalMaximum;

        //scale axis
        qreal scaleXAchse = (qreal)(windowSize.width() - maxStrLenght - borderMarginWidth) / (qreal)20;
        qreal scaleYAchse = (qreal)(windowSize.height() - borderMarginHeigth) / (qreal)10;

        for(qint32 i = 1; i <= 11; i++)
        {
            if(signalNegativeMaximum == 0)                                              // x-axis reached (y-value = 0)
            {
                // append scaled signalpoints
                for(qint32 channel = 0; channel < residuum_matrix.cols(); channel++)    // over all Channels
                {
                    QPolygonF poly;                    
                    for(qint32 h = 0; h < residuum_matrix.rows(); h++)
                        poly.append(QPointF((h * scaleX) + maxStrLenght,  -((residuum_matrix(h, channel) * scaleY + ((i - 1) * scaleYAchse)-(windowSize.height()) + borderMarginHeigth / 2))));
                    QPen pen(_colors.at(channel), 0.5, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);
                    painter.setPen(pen);
                    painter.drawPolyline(poly);
                }
                // paint x-axis
                for(qint32 j = 1; j < 21; j++)
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
                }
                painter.drawLine(maxStrLenght, -(((i - 1) * scaleYAchse)-(windowSize.height()) + borderMarginHeigth / 2), windowSize.width()-5, -(((i - 1) * scaleYAchse)-(windowSize.height()) + borderMarginHeigth / 2));
            }

            painter.drawText(3, -((i - 1) * scaleYAchse - windowSize.height()) - borderMarginHeigth/2 + 4, QString::number(signalNegativeMaximum * scaleYText, 'g', 3));     // paint scalevalue y-axis
            painter.drawLine(maxStrLenght - 2, -(((i - 1) * scaleYAchse)-(windowSize.height()) + borderMarginHeigth / 2), maxStrLenght + 2, -(((i - 1) * scaleYAchse)-(windowSize.height()) + borderMarginHeigth / 2));  // scalelines y-axis
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
        qint32 borderMarginWidth = 15;
        qreal scaleXText = (qreal)signalMatrix.rows() /  (qreal)_sample_rate / (qreal)20;                       // divide signallength
        qreal scaleXAchse = (qreal)(windowSize.width() - 55 - borderMarginWidth) / (qreal)20;

        for(qint32 j = 0; j < 21; j++)
        {
            if(j == 20)
            {
                painter.drawText(j * scaleXAchse + 37, 20, QString::number(j * scaleXText + _from / _sample_rate, 'f', 2));    // scalevalue as string
                painter.drawLine(j * scaleXAchse + 55, 5 + 2, j * scaleXAchse + 55 , 5 - 2);                    // scalelines
            }
            else
            {
                painter.drawText(j * scaleXAchse + 45, 20, QString::number(j * scaleXText + _from / _sample_rate, 'f', 2));    // scalevalue as string
                painter.drawLine(j * scaleXAchse + 55, 5 + 2, j * scaleXAchse + 55 , 5 - 2);                    // scalelines
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

        ui->frame->setEnabled(false);
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

        _adaptive_atom_list.clear();
        _fix_dict_atom_list.clear();

        //reset progressbar text color to black
        pal.setColor(QPalette::Text, Qt::black);
        ui->progressBarCalc->setPalette(pal);
        is_white = false;

        is_saved = false;
        has_warning = false;
        ui->lb_info_content->setText("");

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

    //current atoms list update
    if(fix_dict_atom_res_list.isEmpty())
    {
        GaborAtom temp_atom = adaptive_atom_res_list.last();

        qreal phase = temp_atom.phase;
        if(temp_atom.phase > 2*PI) phase -= 2*PI;

        QTableWidgetItem* atomEnergieItem = new QTableWidgetItem(QString::number(100 * temp_atom.energy / max_energy, 'f', 2));
        QTableWidgetItem* atomScaleItem = new QTableWidgetItem(QString::number(temp_atom.scale / _sample_rate, 'g', 3));
        QTableWidgetItem* atomTranslationItem = new QTableWidgetItem(QString::number(temp_atom.translation / qreal(_sample_rate) + _from  / _sample_rate, 'g', 4));
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
        if(settings.value("sort_results", true).toBool())
        {
            qSort(adaptive_atom_res_list.begin(), adaptive_atom_res_list.end(), sort_energie_adaptive);
            qSort(_adaptive_atom_list.begin(), _adaptive_atom_list.end(), sort_energie_adaptive);
        }
        // ToDo: index = _adaptive_atom_list.indexOf(temp_atom);
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

        //update residuum and atom sum for painting and later save to hdd
        for(qint32 i = 0; i < _signal_matrix.cols(); i++)
        {
            VectorXd atom_vec = temp_atom.max_scalar_list.at(i) * temp_atom.create_real(temp_atom.sample_count, temp_atom.scale, temp_atom.translation, temp_atom.modulation, temp_atom.phase_list.at(i));
            _residuum_matrix.col(i) -= atom_vec;
            _atom_sum_matrix.col(i) += atom_vec;
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
            qSort(fix_dict_atom_res_list.begin(),fix_dict_atom_res_list.end(), sort_energie_fix);
            qSort(_fix_dict_atom_list.begin(),_fix_dict_atom_list.end(), sort_energie_fix);
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
                _atom_sum_matrix.col(channels) += real_residuum_matrix.col(channels);
        }
        else
        {
            for(qint32 channels = 0; channels < _signal_matrix.cols(); channels++)            
                _atom_sum_matrix.col(channels) -= real_residuum_matrix.col(channels);
        }
    }
    else
    {
        if(ui->tbv_Results->columnCount() > 2)
        {
            GaborAtom  atom = _adaptive_atom_list.at(topLeft.row());
            if(!auto_change)
                select_atoms_map[topLeft.row()] = item->checkState();

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
            if(!auto_change)
                select_atoms_map[topLeft.row()] = item->checkState();

            if(item->checkState())
            {
                for(qint32 channels = 0; channels < _signal_matrix.cols(); channels++)
                {
                    _atom_sum_matrix.col(channels) += atom.max_scalar_list.at(channels) * atom.atom_samples;
                    _residuum_matrix.col(channels) -= atom.max_scalar_list.at(channels) * atom.atom_samples;
                }
            }
            else
            {
                for(qint32 channels = 0; channels < _signal_matrix.cols(); channels++)
                {
                    _atom_sum_matrix.col(channels) -= atom.max_scalar_list.at(channels) * atom.atom_samples;
                    _residuum_matrix.col(channels) += atom.max_scalar_list.at(channels) * atom.atom_samples;
                }
            }
        }
    }
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
    ui->frame->setEnabled(true);
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

    tbv_is_loading = false;

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

    connect(this, SIGNAL(send_input(MatrixXd, qint32, qreal, bool, qint32, qint32, qreal, qreal, qreal, qreal)),
            adaptive_Mp, SLOT(recieve_input(MatrixXd, qint32, qreal, bool, qint32, qint32, qreal, qreal, qreal, qreal)));
    connect(adaptive_Mp, SIGNAL(current_result(qint32, qint32, qreal, qreal, MatrixXd, adaptive_atom_list, fix_dict_atom_list)),
                 this, SLOT(recieve_result(qint32, qint32, qreal, qreal, MatrixXd, adaptive_atom_list, fix_dict_atom_list)));
    connect(adaptive_Mp, SIGNAL(finished_calc()), mp_Thread, SLOT(quit()));
    connect(adaptive_Mp, SIGNAL(finished_calc()), adaptive_Mp, SLOT(deleteLater()));
    connect(mp_Thread, SIGNAL(finished()), this, SLOT(calc_thread_finished()));
    connect(mp_Thread, SIGNAL(finished()), mp_Thread, SLOT(deleteLater()));

    connect(adaptive_Mp, SIGNAL(send_warning(qint32)), this, SLOT(recieve_warnings(qint32)));

    QSettings settings;
    bool fixphase = settings.value("fixPhase", false).toBool();
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
                            reflection, expansion, contraction, fullcontraction);
            mp_Thread->start();        
            break;

        case SignalEnergy:        
            emit send_input(signal, MAXINT32, res_energy, fixphase, boost, iterations,
                            reflection, expansion, contraction, fullcontraction);
            mp_Thread->start();        
            break;

        case Both:
            emit send_input(signal, ui->sb_Iterations->value(), res_energy, fixphase, boost, iterations,
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

QString MainWindow::create_display_text(FixDictAtom global_best_matching)
{
    QSettings settings;
    QString display_text;

    if(!settings.value("show_phys_params", false).toBool())
    {
        if(global_best_matching.type == AtomType::GABORATOM)
        {
            display_text = QString("Gaboratom: scale: %0, translation: %1, modulation: %2, phase: %3")
                    .arg(QString::number(global_best_matching.gabor_atom.scale, 'f', 2))
                    .arg(QString::number(global_best_matching.translation, 'f', 2))
                    .arg(QString::number(global_best_matching.gabor_atom.modulation, 'f', 2))
                    .arg(QString::number(global_best_matching.gabor_atom.phase, 'f', 2));
        }
        else if(global_best_matching.type == AtomType::CHIRPATOM)
        {
            display_text = QString("Chripatom: scale: %0, translation: %1, modulation: %2, phase: %3, chirp: %4")
                    .arg(QString::number(global_best_matching.chirp_atom.scale, 'f', 2))
                    .arg(QString::number(global_best_matching.translation, 'f', 2))
                    .arg(QString::number(global_best_matching.chirp_atom.modulation, 'f', 2))
                    .arg(QString::number(global_best_matching.chirp_atom.phase, 'f', 2))
                    .arg(QString::number(global_best_matching.chirp_atom.chirp, 'f', 2));
        }
        else if(global_best_matching.type == AtomType::FORMULAATOM)
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
        if(global_best_matching.type == AtomType::GABORATOM)
        {
            qreal phase = global_best_matching.gabor_atom.phase;
            if(global_best_matching.gabor_atom.phase > 2*PI) phase -= 2*PI;

            display_text = QString("Gaboratom: scale: %0 sec, translation: %1 sec, modulation: %2 Hz, phase: %3 rad")
                    .arg(QString::number(global_best_matching.gabor_atom.scale / _sample_rate, 'f', 2))
                    .arg(QString::number(global_best_matching.translation / qreal(_sample_rate) + _from  / _sample_rate, 'f', 2))
                    .arg(QString::number(global_best_matching.gabor_atom.modulation * _sample_rate / global_best_matching.sample_count, 'f', 2))
                    .arg(QString::number(phase, 'f', 2));
        }
        else if(global_best_matching.type == AtomType::CHIRPATOM)
        {
            qreal phase = global_best_matching.chirp_atom.phase;
            if(global_best_matching.chirp_atom.phase > 2*PI) phase -= 2*PI;

            display_text = QString("Chripatom: scale: %0 sec, translation: %1 sec, modulation: %2 Hz, phase: %3 rad, chirp: %4")
                    .arg(QString::number(global_best_matching.chirp_atom.scale  / _sample_rate, 'f', 2))
                    .arg(QString::number(global_best_matching.translation / qreal(_sample_rate) + _from  / _sample_rate, 'f', 2))
                    .arg(QString::number(global_best_matching.chirp_atom.modulation * _sample_rate / global_best_matching.sample_count, 'f', 2))
                    .arg(QString::number(phase, 'f', 2))
                    .arg(QString::number(global_best_matching.chirp_atom.chirp, 'f', 2));
        }
        else if(global_best_matching.type == AtomType::FORMULAATOM)
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
        if(_from != 0)
            ui->dsb_from->setValue(_from / _sample_rate);
        ui->dsb_to->setValue(_to / _sample_rate);
        read_fiff_changed = false;
    }
    callXAxisWindow->update();
}

//*****************************************************************************************************************

void MainWindow::on_dsb_from_editingFinished()
{   
    if(read_fiff_changed || _from == last_from) return;
    if(ui->dsb_from->value() * _sample_rate < _first_sample)
        ui->dsb_from->setValue(_first_sample / _sample_rate);

    if(file_name.split('.').last() == "fif")
        read_fiff_file_new(file_name);
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
        read_fiff_file_new(file_name);
    else read_matlab_file_new();
    last_to = _to;
}

//*****************************************************************************************************************

void MainWindow::on_sb_sample_count_editingFinished()
{
    if(read_fiff_changed || ui->sb_sample_count->value() == last_sample_count) return;

    if(file_name.split('.').last() == "fif")
        read_fiff_file_new(file_name);
    else read_matlab_file_new();

    last_sample_count = ui->sb_sample_count->value();
}

//*****************************************************************************************************************

void MainWindow::on_dsb_from_valueChanged(double arg1)
{
    if(read_fiff_changed) return;

    read_fiff_changed = true;
    _from = floor(arg1 * _sample_rate);
    _to = _from + ui->sb_sample_count->value() - 1;

    if(_to >= _last_sample)
    {
        _to = _last_sample;
        _samplecount = _to - _from + 1;
        ui->sb_sample_count->setValue(_samplecount);
    }

    ui->dsb_to->setValue(_to / _sample_rate);
    read_fiff_changed = false;

    ui->dsb_from->setToolTip(QString("sample: %1").arg(_from));
}

//*****************************************************************************************************************


void MainWindow::on_dsb_to_valueChanged(double arg1)
{
    if(read_fiff_changed) return;

    read_fiff_changed = true;
    _to = floor(arg1 * _sample_rate);
    _from = _to - ui->sb_sample_count->value() + 1;

    if(_from <= _first_sample)
    {
        _from = _first_sample;
        _samplecount = _to - _from + 1;
        ui->sb_sample_count->setValue(_samplecount);
    }

    ui->dsb_from->setValue(_from / _sample_rate);
    read_fiff_changed = false;

     ui->dsb_to->setToolTip(QString("sample: %1").arg(_to));
}

//*****************************************************************************************************************

void MainWindow::on_sb_sample_count_valueChanged(int arg1)
{
    if(read_fiff_changed) return;

    read_fiff_changed = true;
    _to = _from + arg1 - 1;

    if(_to > _last_sample)
    {
        _to = _last_sample;
        _samplecount = _to - _from + 1;
        ui->sb_sample_count->setValue(_samplecount);
    }
    _samplecount = arg1;
    ui->dsb_to->setValue(_to / _sample_rate);
    read_fiff_changed = false;

    ui->sb_sample_count->setToolTip(QString("epoch: %1 sec").arg((_samplecount) / _sample_rate));
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
    {
        save_path = QFileDialog::getSaveFileName(this, "Save file as...", last_save_path + "/" + save_name,"(*.txt)");
        save_parameters();
        QMessageBox::information(this, tr("information"),
        tr("No fif file for save. Only parameter.txt saved."));
        return;
    }

    save_path = QFileDialog::getSaveFileName(this, "Save file as...", last_save_path + "/" + save_name,"(*.fif)");
    if(save_path.isEmpty()) return;

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

    connect(this, SIGNAL(to_save(QString, QString, fiff_int_t, fiff_int_t, MatrixXd, select_map, RowVectorXi )),
            save_Fif, SLOT(save_fif_file(QString, QString, fiff_int_t, fiff_int_t, MatrixXd, select_map, RowVectorXi )));
    connect(save_Fif, SIGNAL(save_progress(qint32, qint32)), this, SLOT(recieve_save_progress(qint32, qint32)));
    connect(save_thread, SIGNAL(finished()), save_thread, SLOT(deleteLater()));
    connect(save_Fif, SIGNAL(finished()), save_Fif, SLOT(quit()));
    connect(save_Fif, SIGNAL(finished()), save_Fif, SLOT(deleteLater()));

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

    emit to_save(file_name, save_path, _from, _to, _atom_sum_matrix, select_channel_map, picks);
    save_thread->start();
}

//*****************************************************************************************************************

void MainWindow::recieve_save_progress(qint32 current_progress, qint32 finished)
{
    if(finished == 0)
    {
        ui->progress_bar_save->setValue(current_progress);
        if(ui->progress_bar_save->value() > ui->progress_bar_save->maximum() / 2 && !is_save_white)
        {
            pal.setColor(QPalette::Text, Qt::white);
            ui->progress_bar_save->setPalette(pal);
            is_save_white = true;
        }
    }
    else if(finished == 2)
    {
        QMessageBox::warning(this, "Error", "error: Save unsucessful.");
        ui->progress_bar_save->setHidden(true);
    }
    else
    {
        ui->progress_bar_save->setHidden(true);        
        ui->actionSpeicher->setEnabled(true);
        ui->actionSpeicher_unter->setEnabled(true);
    }
}

//*****************************************************************************************************************

void SaveFifFile::save_fif_file(QString source_path, QString save_path, fiff_int_t start_change, fiff_int_t end_change, MatrixXd changes, select_map select_channel_map, RowVectorXi picks)
{    
    QFile t_fileIn(source_path);
    QFile t_fileOut(save_path);

    //   Setup for reading the raw data   
    FiffRawData raw(t_fileIn);

    MatrixXd cals;
    FiffStream::SPtr outfid = Fiff::start_writing_raw(t_fileOut, raw.info, cals, picks);

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
                emit save_progress(first, 2);
                return;
        }
        printf("Writing...");
        if (first_buffer)
        {
           if (first > 0)
               outfid->write_int(FIFF_FIRST_SAMPLE,&first);
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
    if (first_buffer)
    {
       if (start_change > 0)
           outfid->write_int(FIFF_FIRST_SAMPLE,&start_change);
       first_buffer = false;
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
            emit save_progress(first, 2);
            return;
        }
        printf("Writing...");
        outfid->write_raw_buffer(data,cals);
        printf("[done]\n");

        emit save_progress(first, false);
    }

    emit save_progress(to, true);

    printf("Writing...");
    outfid->write_raw_buffer(data,cals);
    printf("[done]\n");

    outfid->finish_writing_raw();
    printf("Finished\n");    
}

//*****************************************************************************************************************

void MainWindow::save_parameters()
{
    QString save_parameter_path = save_path.split(".").first() + ".txt";
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

                if(fix_atom.type == AtomType::GABORATOM)
                {
                    xmlWriter.writeAttribute("scale", QString::number(fix_atom.gabor_atom.scale));
                    xmlWriter.writeAttribute("translation", QString::number(fix_atom.translation));
                    xmlWriter.writeAttribute("modulation", QString::number(fix_atom.gabor_atom.modulation));
                    xmlWriter.writeAttribute("phase", QString::number(fix_atom.gabor_atom.phase));
                }
                else if(fix_atom.type == AtomType::CHIRPATOM)
                {                   
                    xmlWriter.writeAttribute("scale", QString::number(fix_atom.chirp_atom.scale));
                    xmlWriter.writeAttribute("translation", QString::number(fix_atom.translation));
                    xmlWriter.writeAttribute("modulation", QString::number(fix_atom.chirp_atom.modulation));
                    xmlWriter.writeAttribute("phase", QString::number(fix_atom.chirp_atom.phase));
                    xmlWriter.writeAttribute("chirp", QString::number(fix_atom.chirp_atom.chirp));
                }
                else if(fix_atom.type == AtomType::FORMULAATOM)
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
            else
            {
                GaborAtom gabor_atom = _adaptive_atom_list.at(i);
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
        xmlWriter.writeAttribute("of_atoms", QString::number(_adaptive_atom_list.length()));

        qint32 div = floor(_adaptive_atom_list.length() / (qreal)pdict_count);
        qint32 mod = _adaptive_atom_list.length() % pdict_count;
        if(div != 0)
        {
            for(qint32 j = 0; j < pdict_count; j++)
            {
                xmlWriter.writeStartElement("built_Atoms");
                xmlWriter.writeAttribute("formula", "Gaboratom");
                xmlWriter.writeAttribute("sample_count", QString::number(_adaptive_atom_list.first().sample_count));
                xmlWriter.writeAttribute("atom_count", QString::number(div));
                xmlWriter.writeAttribute("source_dict", save_path.split('/').last().split('.').first() + "_" + QString::number(j));

                for(qint32 i = 0; i < div; i++)
                {
                    GaborAtom gabor_atom = _adaptive_atom_list.at(i + j * div);
                    QStringList result_list = gabor_atom.create_string_values(gabor_atom.sample_count, gabor_atom.scale, gabor_atom.sample_count / 2, gabor_atom.modulation, gabor_atom.phase);

                    xmlWriter.writeStartElement("ATOM");
                    xmlWriter.writeAttribute("ID", QString::number(i));
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
                xmlWriter.writeEndElement();    //built_atoms
            } //builds
        }

        if(mod != 0)
        {
            xmlWriter.writeStartElement("built_Atoms");
            xmlWriter.writeAttribute("formula", "Gaboratom");
            xmlWriter.writeAttribute("sample_count", QString::number(_adaptive_atom_list.first().sample_count));
            xmlWriter.writeAttribute("atom_count", QString::number(mod));
            xmlWriter.writeAttribute("source_dict", save_path.split('/').last().split('.').first()  + "_" + QString::number(8));

            for(qint32 i = 0; i < mod; i++)
            {
                GaborAtom gabor_atom = _adaptive_atom_list.at(i + pdict_count * div);
                QStringList result_list = gabor_atom.create_string_values(gabor_atom.sample_count, gabor_atom.scale, gabor_atom.sample_count / 2, gabor_atom.modulation, gabor_atom.phase);

                xmlWriter.writeStartElement("ATOM");
                xmlWriter.writeAttribute("ID", QString::number(i));
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
            xmlWriter.writeEndElement();    //built_atoms
        }
        xmlWriter.writeEndElement();    //COUNT
        xmlWriter.writeEndDocument();
    }
    xml_file.close();
    fill_dict_combobox();
}

//*****************************************************************************************************************

bool MainWindow::sort_energie_adaptive(const GaborAtom atom_1, const GaborAtom atom_2)
{
    return (atom_1.energy > atom_2.energy);
}

//*****************************************************************************************************************

bool MainWindow::sort_energie_fix(const FixDictAtom atom_1, const FixDictAtom atom_2)
{
    return (atom_1.energy > atom_2.energy);
}

//*****************************************************************************************************************

void MainWindow::on_cb_Dicts_currentIndexChanged(const QString &arg1)
{
    Q_UNUSED(arg1);
    ui->lb_info_content->setText("");
    ui->lb_info_content->repaint();
    has_warning = false;
}

//*****************************************************************************************************************

void MainWindow::on_rb_adativMp_clicked()
{
    ui->lb_info_content->setText("");
    ui->lb_info_content->repaint();
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
       qint32 temp_pos = mapFromGlobal(QCursor::pos()).x() - 55;
       qreal stretch_factor = qreal(this->width() - 55/*left_margin*/ - 15/*right_margin*/) / (qreal)(_to -_from);
       qreal time = (qreal)_from / _sample_rate + (qreal)temp_pos / stretch_factor / _sample_rate;
       if(mapFromGlobal(QCursor::pos()).x() >= 55 && mapFromGlobal(QCursor::pos()).x() <= (this->width() - 15))
           this->setToolTip(QString("time: %1 sec").arg(time));
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
        qint32 temp_pos = mapFromGlobal(QCursor::pos()).x() - 55;
        qreal stretch_factor = qreal(this->width() - 55/*left_margin*/ - 15/*right_margin*/) / (qreal)(_samplecount);
        qreal time = (qreal)_from / _sample_rate + (qreal)temp_pos / stretch_factor / _sample_rate;
        if(mapFromGlobal(QCursor::pos()).x() >= 55 && mapFromGlobal(QCursor::pos()).x() <= (this->width() - 15))
            this->setToolTip(QString("time: %1 sec").arg(time));
       setCursor(Qt::CrossCursor);
   }
}

//*****************************************************************************************************************

void ResiduumWindow::mouseMoveEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    if(_to - _from != 0)
    {
        qint32 temp_pos = mapFromGlobal(QCursor::pos()).x() - 55;
        qreal stretch_factor = qreal(this->width() - 55/*left_margin*/ - 15/*right_margin*/) / (qreal)(_samplecount);
        qreal time = (qreal)_from / _sample_rate + (qreal)temp_pos / stretch_factor / _sample_rate;
        if(mapFromGlobal(QCursor::pos()).x() >= 55 && mapFromGlobal(QCursor::pos()).x() <= (this->width() - 15))
            this->setToolTip(QString("time: %1 sec").arg(time));

        setCursor(Qt::CrossCursor);
    }
}

//*****************************************************************************************************************

void GraphWindow::mousePressEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    if(_to - _from != 0)
    {
        press_pos = mapFromGlobal(QCursor::pos()).x();
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
        qreal stretch_factor = qreal(this->width() - 55/*left_margin*/ - 15/*right_margin*/) / (qreal)(_samplecount);
        qint32 old_from = _from;
        qint32 old_to = _to;
        _from += floor((press_pos - release_pos) / stretch_factor);
        _to = _from + _samplecount - 1;
        if(_from < _first_sample)
        {
            _from = _first_sample;
             _to = _from + _samplecount - 1;
            //_samplecount = _to - _from + 1;
        }

        if(_to > _last_sample)
        {
            _to = _last_sample;
            _from = _to - _samplecount + 1;
            //_samplecount = _to - _from + 1;
        }

        setCursor(Qt::CrossCursor);

        // +/- 5 pixel dont read new if clicked by mistake
        if(abs(press_pos - release_pos) < 5 || old_from == _from || old_to == _to)
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

        if(_from < _first_sample)
        {
            _from = _first_sample;
            _samplecount = _to - _from + 1;
        }

        if(_to > _last_sample)
        {
            _to = _last_sample;
            _samplecount = _to - _from + 1;
        }
        emit read_new();
    }
}

//*****************************************************************************************************************

void MainWindow::on_mouse_button_release()
{

    read_fiff_changed = true;

    ui->dsb_from->setValue(_from / _sample_rate);
    ui->dsb_to->setValue(_to / _sample_rate);
    ui->sb_sample_count->setValue(_samplecount);

    read_fiff_changed = false;

    if(file_name.endsWith(".fif", Qt::CaseInsensitive))
        read_fiff_file_new(file_name);
    else
        read_matlab_file_new();
}
