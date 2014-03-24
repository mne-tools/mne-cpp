#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtGui>

namespace Ui
{
    class MainWindow;
}

class GraphWindow;
class Atom;

// Haupfenster
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
    Atom *atom;
    void OpenFile();
    QList<qreal> NormSignal(QList<qreal> signalSamples);
    QStringList correlation(QList<qreal> signalSamples, QList<qreal> atomSamples, QString atomName);
    QList<qreal> mpCalc(QFile& dictionary, QList<qreal> signalSamples, qint32 iterationsCount);
};

// Widget in dem das Eingangssignal gezeichnet wird
class GraphWindow : public QWidget
{
    Q_OBJECT

protected:
   void paintEvent(QPaintEvent *event);
public:
   void PaintSignal(QList<qreal> valueList, QList<qreal> residuumValues, QColor color, QSize windowSize);

};

// Widget in dem das Atom gezeichnet wird
class AtomWindow : public QWidget//, QTableWidgetItem
{
    Q_OBJECT

  protected:
     void paintEvent(QPaintEvent *event);

};

// Widget in dem das Residuum gezeichnet wird
class ResiduumWindow : public QWidget //, QTableWidgetItem
{
   // Q_OBJECT

//protected:
   //void paintEvent(QPaintEvent *event);

};

// Atomklasse zum Erstellen und Abrufen von Atomen und deren Parameter
class Atom : public QObject
{
    Q_OBJECT



public:    
    enum AtomType
    {
             Gauss,
             Chirp
    };

    qreal Samples;
    qreal Scale;
    qreal Modulation;
    qreal Phase;
    qreal ChirpValue;
    Atom::AtomType AType;

    QList<qreal> Create(qint32 samples, qreal scale, qreal modulation, qreal phase, qreal chirp = 0, Atom::AtomType atomType = Gauss);
    QStringList  CreateStringValues(qint32 samples, qreal scale, qreal modulation, qreal phase, qreal chirp, Atom::AtomType atomType);
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
