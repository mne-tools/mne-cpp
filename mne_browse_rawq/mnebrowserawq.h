#ifndef MNEBROWSERAWQ_H
#define MNEBROWSERAWQ_H

#include <QMainWindow>

#include <QTimer>
#include "3rdParty/QCustomPlot/qcustomplot.h" // the header file of QCustomPlot. Don't forget to add it to your project, if you use an IDE, so it gets compiled.



namespace Ui {
class MneBrowseRawQ;
}

class MneBrowseRawQ : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MneBrowseRawQ(QWidget *parent = 0);
    ~MneBrowseRawQ();

    void setupRealtimeDataDemo(QCustomPlot *customPlot);

    void setupPlayground(QCustomPlot *customPlot);

  private slots:
    void realtimeDataSlot();
    void bracketDataSlot();

private:
    Ui::MneBrowseRawQ *ui;

    QTimer dataTimer;
    QCPItemTracer *itemDemoPhaseTracer;
};

#endif // MNEBROWSERAWQ_H
