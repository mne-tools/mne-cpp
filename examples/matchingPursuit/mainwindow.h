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
    void recieve_result(qint32 current_iteration, qint32 max_iterations, qreal current_energy, qreal max_energy, gabor_atom_list atom_res_list);
    void calc_thread_finished();

signals:

    void send_input(MatrixXd send_signal, qint32 send_max_iterations, qreal send_epsilon);

private:

    Ui::MainWindow *ui;    
    GraphWindow *callGraphWindow;
    AtomSumWindow *callAtomSumWindow;
    ResiduumWindow *callResidumWindow;
    QStandardItemModel* model;
    QStandardItem* item;
    std::vector<QStandardItem*> items;

    void open_file();
    void ReadMatlabFile(QString fileName);
    void CalcAdaptivMP(MatrixXd signal, TruncationCriterion criterion);
    qint32 ReadFiffFile(QString fileName);
    QList<qreal> NormSignal(QList<qreal> signalSamples);
    MatrixXd remove_column(MatrixXd& matrix, qint32 colToRemove);
    MatrixXd remove_row(MatrixXd& matrix, qint32 rowToRemove);
    MatrixXd add_row_at(MatrixXd& matrix, VectorXd &rowData, qint32 rowNumber);
    MatrixXd add_column_at(MatrixXd& matrix, VectorXd& rowData, qint32 colNumber);
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
   void PaintSignal(MatrixXd signalMatrix, VectorXd residuumSamples, QList<QColor> colors, QSize windowSize);

};

//*************************************************************************************************************
// Widget to paint Atoms
class AtomSumWindow : public QWidget
{
    Q_OBJECT

protected:
   void paintEvent(QPaintEvent *event);
public:
   void PaintAtomSum(VectorXd signalSamples, QSize windowSize, qreal signalMaximum, qreal signalNegativeMaximum);

};

//*************************************************************************************************************
// Widget to paint residuum
class ResiduumWindow : public QWidget
{
    Q_OBJECT

  protected:
    void paintEvent(QPaintEvent *event);
public:
   void PaintResiduum(VectorXd signalSamples, QSize windowSize, qreal maxPos, qreal maxNeg);


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
