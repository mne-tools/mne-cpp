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
#include <utils/mp/atom.h>
#include <utils/mp/adaptivemp.h>

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

//=============================================================================================================

namespace Ui
{
    class MainWindow;
}


enum TruncationCriterion
{
    Iterations,
    SignalEnergy,
    Both
};

class GraphWindow;
class ResiduumWindow;
class AtomSumWindow;
class Atom;
class YAxisWindow;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    // ToDo seb ctor
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    // ToDo seb
    void fill_dict_combobox();
    // end
    typedef QList<GaborAtom> adaptive_atom_list;
    typedef QList<FixDictAtom> fix_dict_atom_list;
    typedef Eigen::VectorXd VectorXd;    

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

    void on_btt_OpenSignal_clicked();

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

    void calc_thread_finished();
    void on_actionCreate_treebased_dictionary_triggered();
    void on_dsb_from_editingFinished();
    void on_dsb_to_editingFinished();
    void on_dsb_to_valueChanged(double arg1);
    void on_dsb_from_valueChanged(double arg1);
    void on_sb_sample_count_editingFinished();
    void on_sb_sample_count_valueChanged(int arg1);
    void on_cb_all_select_clicked();
    void on_time_out();
    void on_actionSettings_triggered();
    void on_dicts_saved();
    void on_actionSpeicher_unter_triggered();
    void on_actionSpeicher_triggered();
    void on_actionExport_triggered();
    void on_dsb_sample_rate_editingFinished();

signals:

    void send_input(MatrixXd send_signal, qint32 send_max_iterations, qreal send_epsilon, bool fix_phase, qint32 boost, qint32 simplex_it,
                    qreal simplex_reflection, qreal simplex_expansion, qreal simplex_contraction, qreal simplex_full_contraction);
    void send_input_fix_dict(MatrixXd send_signal, qint32 send_max_iterations, qreal send_epsilon, QString path);

private:
    Ui::MainWindow *ui;    
    GraphWindow *callGraphWindow;
    AtomSumWindow *callAtomSumWindow;
    ResiduumWindow *callResidumWindow;
    YAxisWindow *callYAxisWindow;    
    QStandardItem* cb_item;
    QStandardItem* tbv_item;
    QStandardItemModel* cb_model;
    QStandardItemModel* tbv_model;
    std::vector<QStandardItem*> cb_items;
    std::vector<QStandardItem*> tbv_items;

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
    void read_matlab_file(QString fileName);

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
    void calc_adaptiv_mp(MatrixXd signal, TruncationCriterion criterion);

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
    qint32 read_fiff_file(QString fileName);

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

    // ToDo seb
    void read_fiff_ave(QString file_name);
    void fill_channel_combobox();
    void save_fif_file();
    void save_parameters();
    void calc_fix_mp(QString path, MatrixXd signal, TruncationCriterion criterion);
    void closeEvent(QCloseEvent * event);
    static bool sort_Energie(const GaborAtom atom_1, const GaborAtom atom_2);
    //end
};

//*************************************************************************************************************
// Widget to paint inputsignal
class GraphWindow : public QWidget
{
    Q_OBJECT

protected:
   void paintEvent(QPaintEvent *event);

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
   //==========================================================================================================
};

//*************************************************************************************************************
// Widget to paint atoms
class AtomSumWindow : public QWidget
{
    Q_OBJECT

protected:
   void paintEvent(QPaintEvent *event);

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

// Widget to paint y-axis
class YAxisWindow : public QWidget
{
    Q_OBJECT

protected:
   void paintEvent(QPaintEvent *event);

public:
   //==========================================================================================================
   /**
   * YAxisWindow_paint_signal
   *
   * ### MP toolbox GUI function ###
   *
   * painting y-axis of chosen channels in butterfly plot
   *
   * @param[in] signalMatrix    matrix of input signal
   * @param[in] windowSize      size (height,width) of window
   *
   * @return void
   */
   void paint_axis(MatrixXd signalMatrix, QSize windowSize);
   //==========================================================================================================
};

//*************************************************************************************************************

#endif // MAINWINDOW_H
