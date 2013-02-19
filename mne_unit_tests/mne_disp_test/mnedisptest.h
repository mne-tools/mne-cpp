#ifndef MNEDISPTEST_H
#define MNEDISPTEST_H

#include <QMainWindow>

#include <QTimer>
#include "3rdParty/QCustomPlot/qcustomplot.h" // the header file of QCustomPlot. Don't forget to add it to your project, if you use an IDE, so it gets compiled.



namespace Ui {
class MneDispTest;
}

class MneDispTest : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MneDispTest(QWidget *parent = 0);
    ~MneDispTest();

    void setupRealtimeDataDemo(QCustomPlot *customPlot);

    void setupPlayground(QCustomPlot *customPlot);

  private slots:
    void realtimeDataSlot();
    void bracketDataSlot();

private:
    Ui::MneDispTest *ui;

    QTimer dataTimer;
    QCPItemTracer *itemDemoPhaseTracer;
};

#endif // MNEDISPTEST_H
