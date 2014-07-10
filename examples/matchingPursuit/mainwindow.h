#ifndef MAINWINDOW_H
#define MAINWINDOW_H
//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/mne.h>

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

private:

    Ui::MainWindow *ui;    
    GraphWindow *callGraphWindow;
    AtomSumWindow *callAtomSumWindow;
    ResiduumWindow *callResidumWindow;
    void open_file();
    QList<qreal> NormSignal(QList<qreal> signalSamples);
    //QStringList correlation(VectorXd signalSamples, QList<qreal> atomSamples, QString atomName);
    //VectorXd mpCalc(QFile& dictionary, VectorXd signalSamples, qint32 iterationsCount);
    qint32 ReadFiffFile(QString fileName);
    void ReadMatlabFile(QString fileName);
    void CalcAdaptivMP(int iterations, TruncationCriterion criterion);
};

//*************************************************************************************************************
// Widget to paint inputsignal
class GraphWindow : public QWidget
{
    Q_OBJECT

protected:
   void paintEvent(QPaintEvent *event);
public:
   void PaintSignal(VectorXd signalSamples, VectorXd residuumSamples, QColor color, QSize windowSize);

};

//*************************************************************************************************************
// Widget to paint Atoms
class AtomSumWindow : public QWidget
{
    Q_OBJECT

protected:
   void paintEvent(QPaintEvent *event);
public:
   void PaintAtomSum(VectorXd signalSamples, QSize windowSize);

};

//*************************************************************************************************************
// Widget to paint residuum
class ResiduumWindow : public QWidget //, QTableWidgetItem
{
    Q_OBJECT

  protected:
    void paintEvent(QPaintEvent *event);
public:
   void PaintResiduum(VectorXd signalSamples, QSize windowSize);


};

//*************************************************************************************************************
//=============================================================================================================



#endif // MAINWINDOW_H
