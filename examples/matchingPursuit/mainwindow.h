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
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MASSACHUSETTS GENERAL HOSPITAL BE LIABLE FOR ANY DIRECT,
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

//=============================================================================================================
// USED NAMESPACES

using namespace MNELIB;

//=============================================================================================================


namespace Ui
{
    class MainWindow;  
}

enum TruncationCriterion;
class GraphWindow;
class ResiduumWindow;
class AtomSumWindow;
class Atom;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    typedef QList<GaborAtom> gabor_atom_list;

private slots:

    void on_btt_Calc_clicked();
    void on_actionW_rterbucheditor_triggered();
    void on_actionAtomformeleditor_triggered();
    void on_actionErweiterter_W_rterbucheditor_triggered();
    void on_actionNeu_triggered();
    void on_btt_OpenSignal_clicked();
    void on_tbv_Results_cellClicked(int row, int column);
    void cb_selection_changed(const QModelIndex&, const QModelIndex&);
    void tbv_selection_changed(const QModelIndex& topLeft, const QModelIndex& bottomRight);
    void recieve_result(qint32 current_iteration, qint32 max_iterations, qreal current_energy, qreal max_energy, gabor_atom_list atom_res_list);
    void calc_thread_finished();

    void on_actionCreate_treebased_dictionary_triggered();

signals:

    void send_input(MatrixXd send_signal, qint32 send_max_iterations, qreal send_epsilon);

private:

    Ui::MainWindow *ui;    
    GraphWindow *callGraphWindow;
    AtomSumWindow *callAtomSumWindow;
    ResiduumWindow *callResidumWindow;
    QStandardItemModel* cb_model;
    QStandardItem* cb_item;
    std::vector<QStandardItem*> cb_items;

    QStandardItemModel* tbv_model;
    QStandardItem* tbv_item;
    std::vector<QStandardItem*> tbv_items;

    void open_file();
    void read_matlab_file(QString fileName);
    void calc_adaptiv_mp(MatrixXd signal, TruncationCriterion criterion);
    qint32 read_fiff_file(QString fileName);
    QList<qreal> norm_signal(QList<qreal> signalSamples);
     //QStringList correlation(VectorXd signalSamples, QList<qreal> atomSamples, QString atomName);
    //VectorXd mpCalc(QFile& dictionary, VectorXd signalSamples, qint32 iterationsCount);

};

//*************************************************************************************************************
// Widget to paint inputsignal
class GraphWindow : public QWidget
{
    Q_OBJECT

protected:
   void paintEvent(QPaintEvent *event);
public:
   void paint_signal(MatrixXd signalMatrix, QSize windowSize);

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
   * AtomSumWIndow_paint_atom_sum
   *
   * ### MP toolbox GUI function ###
   *
   * painting sum of found atoms in butterfly plot
   *
   * @param[in] atom_matrix    matrix of found atoms for each channel
   * @param[in] windowSize
   * @param[in] signalMaximum
   * @param[in] signalNegativeMaximum
   *
   *
   * @return void
   */
   void paint_atom_sum(MatrixXd atom_matrix, QSize windowSize, qreal signalMaximum, qreal signalNegativeMaximum);

};

//*************************************************************************************************************
// Widget to paint residuum
class ResiduumWindow : public QWidget
{
    Q_OBJECT

  protected:
    void paintEvent(QPaintEvent *event);
public:
   void PaintResiduum(MatrixXd residuum_matrix, QSize windowSize, qreal maxPos, qreal maxNeg);


};

//*************************************************************************************************************
/*
class MatrixXdS : public MatrixXd
{
    Q_OBJECT


public:
   void set_selected(int index);
   bool is_selected(int index);


};
*/
//=============================================================================================================



#endif // MAINWINDOW_H
