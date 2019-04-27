//=============================================================================================================
/**
* @file     mainwindow.h
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
*     * Neither the name of the Massachusetts General Hospital nor the names of its contributors may be used
*       to endorse or promote products derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*
* @brief    MainWindow class declaration shows the main window of the matching pursuit toolbox which includes
*           all main functions. It is composed of three widgets (input signal, approximation and residuum). One
*           can choose the truncation criterions and the typ of MP-Algorithm. Also the channels (if multichannel
*           data) and the calculated atoms could be selected. With the help of the toolbar above, signals can be
*           loaded and approximations can be saved. Furthermore one can open dictionary editor, advanced
*           dictionary editor und atom formula editor.
*/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/mne.h>
#include <disp/plots/plot.h>
#include <disp/plots/helpers/colormap.h>
#include <utils/spectrogram.h>
#include <utils/mp/atom.h>
#include <utils/mp/adaptivemp.h>
#include <utils/mp/fixdictmp.h>
#include <disp/plots/tfplot.h>

#include "editorwindow.h"
#include "ui_editorwindow.h"
#include "formulaeditor.h"
#include "ui_formulaeditor.h"
#include "enhancededitorwindow.h"
#include "ui_enhancededitorwindow.h"
#include "deletemessagebox.h"
#include "ui_deletemessagebox.h"
#include "processdurationmessagebox.h"
#include "ui_processdurationmessagebox.h"
#include "treebaseddictwindow.h"
#include "ui_treebaseddictwindow.h"
#include "settingwindow.h"
#include "ui_settingwindow.h"

//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QMainWindow>
#include <QtGui>
#include <QAbstractTableModel>
#include <QImage>
#include <QAbstractItemDelegate>
#include <QFontMetrics>
#include <QModelIndex>
#include <QSize>

#ifndef UINT32
typedef unsigned int        UINT32, *PUINT32;
#endif
#ifndef INT32
typedef signed int          INT32, *PINT32;
#endif
#ifndef MAXUINT32
#define MAXUINT32   ((UINT32)~((UINT32)0))
#endif
#ifndef MAXINT32
#define MAXINT32    ((INT32)(MAXUINT32 >> 1))
#endif
#ifndef MININT32
#define MININT32    ((INT32)~MAXINT32)
#endif


//=============================================================================================================
// USED NAMESPACES

using namespace MNELIB;
using namespace DISPLIB;

//=============================================================================================================

namespace Ui
{
    class MainWindow;
}


enum truncation_criterion
{
    Iterations,
    SignalEnergy,
    Both
};

enum source_file_type
{
    AVE,
    RAW,
    TXT
};

class GraphWindow;
class ResiduumWindow;
class AtomSumWindow;
class XAxisWindow;
class tfplotwidget;

class MainWindow : public QMainWindow, ColorMap
{
    Q_OBJECT

public:
    //**********************************************************************************************************
    //constructor
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    //**********************************************************************************************************

    typedef QList<QList<GaborAtom> > adaptive_atom_list;
    typedef QList<FixDictAtom> fix_dict_atom_list;
    typedef QMap<qint32, bool> select_map;
    typedef Eigen::VectorXd VectorXd;
    typedef Eigen::RowVectorXi RowVectorXi;    


private slots:
    //==========================================================================================================
    /**
    * MainWindow_on_btt_Calc_clicked
    *
    * ### MP toolbox main window slots ###
    *
    * starts calculation
    *
    * @return void
    */
    void on_btt_Calc_clicked();

    //==========================================================================================================
    /**
    * MainWindow_on_actionW_rterbucheditor_triggered
    *
    * ### MP toolbox main window slots ###
    *
    * opens dictionary editor
    *
    * @return void
    */
    void on_actionW_rterbucheditor_triggered();

    //==========================================================================================================
    /**
    * MainWindow_on_actionAtomformeleditor_triggered
    *
    * ### MP toolbox main window slots ###
    *
    * opens atom formular editor
    *
    * @return void
    */
    void on_actionAtomformeleditor_triggered();

    //==========================================================================================================
    /**
    * MainWindow_on_actionErweiterter_W_rterbucheditor_triggered
    *
    * ### MP toolbox main window slots ###
    *
    * opens advanced dictionary editor
    *
    * @return void
    */
    void on_actionErweiterter_W_rterbucheditor_triggered();

    //==========================================================================================================
    /**
    * MainWindow_on_actionNeu_triggered
    *
    * ### MP toolbox main window slots ###
    *
    * opens interface to select signal file
    *
    * @return void
    */
    void on_actionNeu_triggered();

    //==========================================================================================================
    /**
    * MainWindow_on_btt_OpenSignal_clicked
    *
    * ### MP toolbox main window slots ###
    *
    * opens interface to select signal file
    *
    * @return void
    */
    void on_btt_OpenSignal_clicked();

    //==========================================================================================================
    /**
    * MainWindow_cb_selection_changed
    *
    * ### MP toolbox main window slots ###
    *
    * changes channel selection
    *
    * @param[in] QModelIndex    channel number
    *
    * @return void
    */
    void cb_selection_changed(const QModelIndex&, const QModelIndex&);

    //==========================================================================================================
    /**
    * MainWindow_tbv_selection_changed
    *
    * ### MP toolbox main window slots ###
    *
    * selects/deselects all calculated atoms
    *
    * @return void
    */
    void tbv_selection_changed(const QModelIndex& topLeft, const QModelIndex& bottomRight);

    //==========================================================================================================
    /**
    * MainWindow_recieve_result
    *
    * ### MP toolbox main window slots ###
    *
    * receives calculated atoms and updates remaining energy, iterations and progressbar
    *
    * @param[in]    current_iteration   number of current iteration
    * @param[in]    max_iterations      number of all iterations for current calculation
    * @param[in]    current_energy      energy of decomposition
    * @param[in]    max_energy          whole energy of given signal
    * @param[in]    atom_res_list       list of calculated atoms
    *
    * @return void
    */
    void recieve_result(qint32 current_iteration, qint32 max_iterations, qreal current_energy, qreal max_energy, MatrixXd residuum,
                        adaptive_atom_list adaptive_atom_res_list, fix_dict_atom_list fix_dict_atom_res_list);

    //==========================================================================================================
    /**
    * MainWindow_recieve_warnings
    *
    * ### MP toolbox main window slots ###
    *
    * receives warnings and infos from fixDict and adaptive MP Algorithm to show in GUI
    *
    * @param[in]    warning_number     number of corresponding warning
    *
    * @return void
    *
    * warning_number and meaning:
    *
    * "1"   fixDict MP: dict excludes atoms to approximate the signal more closely. Calculation terminated.
    * "2"   fixDict MP: No matching sample count between atoms and signal. This may lead to discontinuities.
    * "10"  fixDict, adaptive MP: Algorithm canceled by user interaction. (click on btt cancel)
    * "11"  adaptive MP: Max. Simplex Iteration limit achieved, result may not be optimal.   *
    *
    */
    void recieve_warnings(qint32 warning_number);

    //==========================================================================================================
    /**
    * MainWindow_calc_thread_finished
    *
    * ### MP toolbox main window slots ###
    *
    * updates main window when calculation is finished
    *
    * @return void
    */
    void calc_thread_finished();

    //==========================================================================================================
    /**
    * MainWindow_on_actionCreate_treebased-dictionary_triggered
    *
    * ### MP toolbox main window slots ###
    *
    * opens editor for creating treebased dictionary
    *
    * @return void
    */
    void on_actionCreate_treebased_dictionary_triggered();

    //==========================================================================================================
    /**
    * MainWindow_on_dsb_from_editingFinished
    *
    * ### MP toolbox main window slots ###
    *
    * reads start value out of "from" double spin box
    *
    * @return void
    */
    void on_dsb_from_editingFinished();

    //==========================================================================================================
    /**
    * MainWindow_on_dsb_to_editingFinished
    *
    * ### MP toolbox main window slots ###
    *
    * reads end value out of "to" double spin box
    *
    * @return void
    */
    void on_dsb_to_editingFinished();

    //==========================================================================================================
    /**
    * MainWindow_on_dsb_to_valueChanged
    *
    * ### MP toolbox main window slots ###
    *
    * reads end value
    *
    * @param    arg1    value from "to" double spin box
    *
    * @return void
    */
    void on_dsb_to_valueChanged(double arg1);

    //==========================================================================================================
    /**
    * MainWindow_on_dsb_from_valueChanged
    *
    * ### MP toolbox main window slots ###
    *
    * reads start value
    *
    * @param    arg1    value from "from" double spin box
    *
    * @return void
    */
    void on_dsb_from_valueChanged(double arg1);

    //==========================================================================================================
    /**
    * MainWindow_on_sb_sample_count_editingFinished
    *
    * ### MP toolbox main window slots ###
    *
    * reads in fiff file after editing sample lenght
    *
    * @return void
    */
    void on_sb_sample_count_editingFinished();

    //==========================================================================================================
    /**
    * MainWindow_on_sb_sample_count_valueChanged
    *
    * ### MP toolbox main window slots ###
    *
    * calculates new end value when sample length is changed
    *
    * @param    arg1    sample lenght (64 to 4097)
    *
    * @return void
    */
    void on_sb_sample_count_valueChanged(int arg1);

    //==========================================================================================================
    /**
    * MainWindow_on_cb_all_select_clicked
    *
    * ### MP toolbox main window slots ###
    *
    * check button selects or deselects all founded atoms
    *
    * @return void
    */
    void on_cb_all_select_clicked();

    //==========================================================================================================
    /**
    * MainWindow_on_time_out
    *
    * ### MP toolbox main window slots ###
    *
    * stops calculation
    *
    * @return void
    */
    void on_time_out();

    //==========================================================================================================
    /**
    * MainWindow_on_actionSettings_triggered
    *
    * ### MP toolbox main window slots ###
    *
    * opens setting window
    *
    * @return void
    */
    void on_actionSettings_triggered();

    //==========================================================================================================
    /**
    * MainWindow_on_dicts_saved
    *
    * ### MP toolbox main window slots ###
    *
    * fills combobox with available dictionaries
    *
    * @return void
    */
    void on_dicts_saved();

    //==========================================================================================================
    /**
    * MainWindow_on_actionSpeicher_unter_triggered
    *
    * ### MP toolbox main window slots ###
    *
    * opens dialog window for saving approximation
    *
    * @return void
    */
    void on_actionSpeicher_unter_triggered();

    //==========================================================================================================
    /**
    * MainWindow_on_actionSpeicher_triggered
    *
    * ### MP toolbox main window slots ###
    *
    * saves approximation
    *
    * @return void
    */
    void on_actionSpeicher_triggered();

    //==========================================================================================================
    /**
    * MainWindow_on_actionExport_triggered
    *
    * ### MP toolbox main window slots ###
    *
    * saves calculated atoms as new dictionary
    *
    * @return void
    */
    void on_actionExport_triggered();

    //==========================================================================================================
    /**
    * MainWindow_on_dsb_sample_rate_editingFinished
    *
    * ### MP toolbox main window slots ###
    *
    * sends
    *
    * @return void
    */
    void on_dsb_sample_rate_editingFinished();

    //==========================================================================================================
    /**
    * MainWindow_on_cb_Dicts_currentIndexChanged
    *
    * ### MP toolbox main window slots ###
    *
    * ???
    *
    * @param
    *
    * @return void
    */
    void on_cb_Dicts_currentIndexChanged(const QString &arg1);

    //==========================================================================================================
    /**
    * MainWindow_on_rb_adaptivMP_clicked
    *
    * ### MP toolbox main window slots ###
    *
    * ???
    *
    * @return void
    */
    void on_rb_adativMp_clicked();

    //==========================================================================================================
    /**
    * MainWindow_activate_info_label
    *
    * ### MP toolbox main window slots ###
    *
    *
    *
    * @return void
    */
    void activate_info_label();

    //==========================================================================================================
    /**
    * MainWindow_recieve_save_progress
    *
    * ### MP toolbox main window slots ###
    *
    *
    *
    * @return void
    */
    void recieve_save_progress(qint32 current_progress, qint32 finished);

    //==========================================================================================================

    void on_dsb_energy_valueChanged(double arg1);
    void on_actionBeenden_triggered();
    void on_mouse_button_release();
    void on_rb_OwnDictionary_clicked();
    void on_extend_tab_button();
    void on_close_tab_button(int index);
    void on_actionTFplot_triggered();

signals:

    void send_input(MatrixXd send_signal, qint32 send_max_iterations, qreal send_epsilon, bool fix_phase, qint32 boost, qint32 simplex_it,
                    qreal simplex_reflection, qreal simplex_expansion, qreal simplex_contraction, qreal simplex_full_contraction, bool trial_separation);
    void send_input_fix_dict(MatrixXd send_signal, qint32 send_max_iterations, qreal send_epsilon, qint32 boost, QString path, qreal delta);
    void to_save(QString source_path, QString save_path, fiff_int_t start_change, fiff_int_t end_change, MatrixXd changes, MatrixXd original_signal, select_map select_channel_map, RowVectorXi picks, source_file_type file_type);
    void kill_save_thread();

private:

    bool is_saved;
    bool has_warning;
    bool is_save_white;
    bool tbv_is_loading;
    bool auto_change;
    bool was_partialchecked;
    bool read_fiff_changed;
    bool is_white;
    bool is_calulating;
    fiff_int_t last_to;
    fiff_int_t last_from;
    qint32 last_sample_count;
    qreal residuum_energy;
    qreal signal_energy;
    qreal composed_energy;
    qint32 recieved_result_counter;
    qint32 max_tbv_header_width;
    QString save_path;
    QString file_name;
    source_file_type file_type;
    QString last_open_path;
    QString last_save_path;
    QMap<qint32, bool> select_channel_map;
    QMap<qint32, bool> select_atoms_map;
    QList<QColor> original_colors;
    QList<QList<GaborAtom> > _adaptive_atom_list;
    QList<FixDictAtom> _fix_dict_atom_list;
    MatrixXd datas;
    RowVectorXf times_vec;
    MatrixXd times;
    MatrixXd original_signal_matrix;
    MatrixXd reference_matrix;
    MatrixXd real_residuum_matrix;
    QTime counter_time;
    Ui::MainWindow *ui;    
    GraphWindow *callGraphWindow;
    AtomSumWindow *callAtomSumWindow;
    ResiduumWindow *callResidumWindow;
    XAxisWindow *callXAxisWindow;
    QStandardItem* cb_item;
    QStandardItemModel* cb_model;
    std::vector<QStandardItem*> cb_items;
    RowVectorXi picks;
    FiffInfo pick_info;
    QPalette pal;

    QTimer *_counter_timer;
    QThread* mp_Thread;
    AdaptiveMp *adaptive_Mp;
    FixDictMp *fixDict_Mp ;

    //==========================================================================================================
    /**
    * MainWindow_fill_dict_combobox
    *
    * ### MP toolbox main window function ###
    *
    * fills combobox with available dictionaries
    *
    * @return void
    */
    void fill_dict_combobox();

    //==========================================================================================================
    /**
    * MainWindow_fill_dict_combobox
    *
    * ### MP toolbox main window function ###
    *
    * save files
    *
    * @return void
    */
    void save_fif_file();

    //==========================================================================================================
    /**
    * MainWindow_open_file
    *
    * ### MP toolbox main function ###
    *
    * opens files for read signals
    *
    * @return void
    */
    void open_file();

    //==========================================================================================================
    /**
    * MainWindow_read_matlab_file
    *
    * ### MP toolbox main function ###
    *
    * reads matlab files
    *
    * @param[in] fileName   name of matlab file
    *
    * @return void
    */
    bool read_matlab_file(QString fileName);

    //==========================================================================================================
    /**
    * MainWindow_read_matlab_file_new
    *
    * ### MP toolbox main function ###
    *
    * reads matlab files new
    *
    * @param[in] fileName   name of matlab file
    *
    * @return void
    */
    void read_matlab_file_new();

    //==========================================================================================================
    /**
    * MainWindow_calc_adaptive_mp
    *
    * ### MP toolbox main function ###
    *
    * calculates atoms with adaptive matching pursuit algorithm
    *
    * @param[in] signal         input signal
    * @param[in] criterion      truncation criterion to end approximation
    *
    * @return void
    */
    void calc_adaptiv_mp(MatrixXd signal, truncation_criterion criterion);

    //==========================================================================================================
    /**
    * MainWindow_read_fiff_file
    *
    * ### MP toolbox main function ###
    *
    * reads data from fiff files
    *
    * @param[in] fileName   name of fiff file
    *
    * @return gibt 0 zurück wenn erflogreich sonst ungleich 0
    */
    bool read_fiff_file(QString fileName);

    //==========================================================================================================
    /**
    * MainWindow_read_fiff_file
    *
    * ### MP toolbox main function ###
    *
    * reads data from fiff files
    *
    * @param[in] file_name   name of fiff file
    */
    void read_fiff_file_new(QString file_name);

    //==========================================================================================================
    /**
    * MainWindow_read_fiff_ave
    *
    * ### MP toolbox main function ###
    *
    * reads data from ave files
    *
    * @param[in] QString file name  name of average-file
    *
    * @return   bool
    */
    bool read_fiff_ave(QString file_name);

    //==========================================================================================================
    /**
    * MainWindow_read_fiff_ave
    *
    * ### MP toolbox main function ###
    *
    * reads data from ave files
    *
    * @param[in] QString file name  name of average-file
    *
    * @return   bool
    */
    void read_fiff_ave_new();

    //==========================================================================================================
    /**
    * MainWindow_fill channel_combobox
    *
    * ### MP toolbox main function ###
    *
    * fills combobox with channel indicies
    *
    * @return   void
    */
    void fill_channel_combobox();


    //==========================================================================================================
    /**
    * MainWindow_save_paramters
    *
    * ### MP toolbox main function ###
    *
    * saves parameters
    *
    * @return   void
    */
    void save_parameters();

    //==========================================================================================================
    /**
    * MainWindow_calc_fix_mp
    *
    * ### MP toolbox main function ###
    *
    * calculates MP-Algorithm with fix dictionaries
    *
    * @param    path
    * @param    signal      Matrix of input signal
    * @param    criterion
    *
    * @return   void
    */
    void calc_fix_mp(QString path, MatrixXd signal, truncation_criterion criterion);

    //==========================================================================================================
    /**
    * MainWindow_create_display_text
    *
    * ### MP toolbox main function ###
    *
    * creates the display texts of found atoms to show in the result list
    *
    * @param[in]    global_best_matching    best matching atom, which text should be created
    *
    * @return       QString                 display text
    */
    QString create_display_text(FixDictAtom global_best_matching);

    //==========================================================================================================
    /**
    * MainWindow_closeEvent
    *
    * ### MP toolbox main function ###
    *
    * Qt close event
    *
    * @param    QCloseEvent
    *
    * @return   void
    */
    void closeEvent(QCloseEvent * event);

    //==========================================================================================================
    /**
    * MainWindow_sort_energie_adaptive
    *
    * ### MP toolbox main function ###
    *
    * sorts atoms from adaptive MP regarding to their energy
    *
    * @param    atom_1  first atom
    * @param    atom_2  second atom
    *
    * @return   static bool     sort_energie_adaptive
    */
    static bool sort_energy_adaptive(const QList<GaborAtom> atom_1, const QList<GaborAtom> atom_2);

    //==========================================================================================================
    /**
    * MainWindow_sort_energie_fix
    *
    * ### MP toolbox main function ###
    *
    * sorts atoms from fix dictionaries regarding to their energy
    *
    * @param    atom_1  first atom
    * @param    atom_2  second atom
    *
    * @return   sort_energie_fix    ???
    */
    static bool sort_energy_fix(const FixDictAtom atom_1, const FixDictAtom atom_2);

    //==========================================================================================================
};

//*************************************************************************************************************
// Widget to paint inputsignal
class GraphWindow : public QWidget
{
    Q_OBJECT
private:
    fiff_int_t press_pos;

protected:
   void paintEvent(QPaintEvent *event);
   void mouseMoveEvent(QMouseEvent *event);
   void mousePressEvent(QMouseEvent *event);
   void mouseReleaseEvent(QMouseEvent *event);
   void wheelEvent (QWheelEvent *event);

public:
   //==========================================================================================================
   /**
   * GraphWindow_paint_signal
   *
   * ### MP toolbox GUI function ###
   *
   * painting input signal of chosen channels in butterfly plot
   *
   * @param[in] signalMatrix    matrix of input signal
   * @param[in] windowSize      size (height,width) of window
   *
   * @return void
   */
   void paint_signal(MatrixXd signalMatrix, QSize windowSize);

signals:
    void read_new();

   //==========================================================================================================
};

//*************************************************************************************************************
// Widget to paint atoms
class AtomSumWindow : public QWidget
{
    Q_OBJECT

protected:
   void paintEvent(QPaintEvent *event);
   void mouseMoveEvent(QMouseEvent *event);

public:
   //=========================================================================================================
   /**
   * AtomSumWindow_paint_atom_sum
   *
   * ### MP toolbox GUI function ###
   *
   * painting sum of found atoms in butterfly plot
   *
   * @param[in] atom_matrix             matrix of found atoms for each channel
   * @param[in] windowSize              size (height,width) of window
   * @param[in] signalMaximum           maximum value of atom signal
   * @param[in] signalNegativeMaximum   minimum value of atom signal
   *
   * @return void
   */
   void paint_atom_sum(MatrixXd atom_matrix, QSize windowSize, qreal signalMaximum, qreal signalNegativeMaximum);

   //==========================================================================================================
};

//*************************************************************************************************************
// Widget to paint residuum
class ResiduumWindow : public QWidget
{
    Q_OBJECT

  protected:
    void paintEvent(QPaintEvent *event);
    void mouseMoveEvent(QMouseEvent *event);

public:
    //=========================================================================================================
    /**    
    * ResiduumWindow_paint_residuum
    *
    * ### MP toolbox GUI function ###
    *
    * painting residuum after each iteration in butterfly plot
    *
    * @param[in] residuum_matrix    matrix of found residuums for each channel
    * @param[in] windowSize         size (height,width) of window
    * @param[in] maxPos             maximum value of residuum signal
    * @param[in] maxNeg             minimum value of residuum signal
    *
    * @return void
    */
    void paint_residuum(MatrixXd residuum_matrix, QSize windowSize, qreal maxPos, qreal maxNeg);

    //=========================================================================================================
};

// Widget to paint x-axis
class XAxisWindow : public QWidget
{
    Q_OBJECT

protected:
   void paintEvent(QPaintEvent *event);

public:
   //==========================================================================================================
   /**
   * XAxisWindow_paint_signal
   *
   * ### MP toolbox GUI function ###
   *
   * painting x-axis of chosen channels in butterfly plot
   *
   * @param[in] signalMatrix    matrix of input signal
   * @param[in] windowSize      size (height,width) of window
   *
   * @return void
   */
   void paint_axis(MatrixXd signalMatrix, QSize windowSize);

   //==========================================================================================================
};

//save fif file class
class SaveFifFile : public QThread
{
    Q_OBJECT

    typedef QMap<qint32, bool> select_map;
    typedef Eigen::MatrixXd MatrixXd;
    typedef FIFFLIB::fiff_int_t fiff_int_t;
    typedef Eigen::RowVectorXi RowVectorXi;

public:
    SaveFifFile();

    ~SaveFifFile();

private slots:
    //==========================================================================================================
    /**
    * MainWindow_save_fif_file
    *
    * ### MP toolbox main function ###
    *
    * saves fiff-files
    *
    * @return   void
    */
    void save_fif_file(QString source_path, QString save_path, fiff_int_t start_change, fiff_int_t end_change, MatrixXd changes, MatrixXd original_signal, select_map select_channel_map, RowVectorXi picks, source_file_type file_type);

signals:
    void save_progress(qint32 current_progress, qint32 finished);

};

//*************************************************************************************************************

#endif // MAINWINDOW_H
