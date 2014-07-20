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

private slots:

    void on_btt_Calc_clicked();
    void on_actionW_rterbucheditor_triggered();
    void on_actionAtomformeleditor_triggered();
    void on_actionErweiterter_W_rterbucheditor_triggered();
    void on_actionNeu_triggered();
    void on_btt_OpenSignal_clicked();
    void on_tbv_Results_cellClicked(int row, int column);
    void recieve_result(qint32 current_iteration, qint32 max_iterations, qreal current_energy, qreal max_energy, QList<GaborAtom> atom_res_list);
    void slot_changed(const QModelIndex&, const QModelIndex&);

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
    QList<qreal> NormSignal(QList<qreal> signalSamples);
    //QStringList correlation(VectorXd signalSamples, QList<qreal> atomSamples, QString atomName);
    //VectorXd mpCalc(QFile& dictionary, VectorXd signalSamples, qint32 iterationsCount);
    qint32 ReadFiffFile(QString fileName);
    void ReadMatlabFile(QString fileName);
    void CalcAdaptivMP(MatrixXd signal, TruncationCriterion criterion);
    MatrixXd remove_column(MatrixXd& matrix, qint32 colToRemove);
    MatrixXd remove_row(MatrixXd& matrix, qint32 rowToRemove);
    MatrixXd add_row_at(MatrixXd& matrix, VectorXd &rowData, qint32 rowNumber);
    MatrixXd add_column_at(MatrixXd& matrix, VectorXd& rowData, qint32 colNumber);

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
class ResiduumWindow : public QWidget //, QTableWidgetItem
{
    Q_OBJECT

  protected:
    void paintEvent(QPaintEvent *event);
public:
   void PaintResiduum(VectorXd signalSamples, QSize windowSize, qreal maxPos, qreal maxNeg);


};

//*************************************************************************************************************
//=============================================================================================================



#endif // MAINWINDOW_H
