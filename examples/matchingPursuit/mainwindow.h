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

class GraphWindow;
class Atom;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:

    void on_btt_Calc_clicked();
    void on_btt_Close_clicked();
    void on_actionW_rterbucheditor_triggered();
    void on_actionAtomformeleditor_triggered();
    void on_actionErweiterter_W_rterbucheditor_triggered();
    void on_actionNeu_triggered();
    void on_btt_OpenSignal_clicked();

private:
    Ui::MainWindow *ui;    
    GraphWindow *callGraphWindow;
    //AtomWindow *callAtomWindow;
    //ResiduumWindow *callResiduumWindow;    
    void OpenFile();
    QList<qreal> NormSignal(QList<qreal> signalSamples);
    //QStringList correlation(VectorXd signalSamples, QList<qreal> atomSamples, QString atomName);
    //VectorXd mpCalc(QFile& dictionary, VectorXd signalSamples, qint32 iterationsCount);
    qint32 ReadFiffFile(QString fileName);
    void ReadMatlabFile(QString fileName);
};

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
//=============================================================================================================

 class SignalModel : public QAbstractTableModel
 {
     Q_OBJECT

 public:
     SignalModel(QObject *parent = 0);

     void setSignal(const MatrixXd &signal);

     virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
     virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;

     virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
     virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

 private:
     MatrixXd modelSignal;
 };

//***************************************************************************************************************

class QAbstractItemModel;
class QObject;
class QPainter;

static const int ItemSize = 256;

class SignalDelegate : public QAbstractItemDelegate
{
    Q_OBJECT

public:
    SignalDelegate(QObject *parent = 0);
    virtual void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    virtual QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index ) const;

public slots:

private:

};

//*************************************************************************************************************
//=============================================================================================================

// Widget to paint atom
class AtomWindow : public QWidget//, QTableWidgetItem
{
    Q_OBJECT

  protected:
     void paintEvent(QPaintEvent *event);

};

// Widget to paint residuum
class ResiduumWindow : public QWidget //, QTableWidgetItem
{
   // Q_OBJECT

//protected:
   //void paintEvent(QPaintEvent *event);

};

/*template <class type>
class Q2DVector : public QVector< QVector<type> >
{
        public:
                Q2DVector() : QVector< QVector<type> >(){};
                Q2DVector(int rows, int columns) : QVector< QVector<type> >(rows) {
                        for(int r=0; r<rows; r++) {
                                this[r].resize(columns);
                        }
                };
                virtual ~Q2DVector() {};
};

typedef Q2DVector<bool> Q2DBoolVector;*/

#endif // MAINWINDOW_H
