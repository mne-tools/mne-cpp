#ifndef PLOTTER_H
#define PLOTTER_H

#include <QMap>
#include <QPixmap>
#include <QVector>
#include <QWidget>
#include <QToolButton>
#include <QStylePainter>
#include <QStyleOptionFocusRect>


//class QToolButton;
class PlotSettings;

class plotter : public QWidget
{
    Q_OBJECT
public:
    plotter(QWidget *parent=0);

    void setPlotSettings(const PlotSettings &settings);
    void setCurveData(int id, const QVector <QPointF>  &data);
    void clearCurve(int id);

    QSize minimumSizeHint() const;
    QSize sizeHint() const;

//public slots:
//    void zoomIn();
//    void zoomOut();

protected:
    void paintEvent(QPaintEvent *event);
    void resizeEvent(QResizeEvent *event);

private:
    void updateRubberBandRegion();
    void refreshPixmap();
    void drawGrid(QPainter *painter);
    void drawCurve(QPainter *painter);
    void drawRotatedText(QPainter *painter, int x, int y, const QString &text);


    enum {Margin = 30, xMargin = 80};

//    QToolButton *zoomInButton;
//    QToolButton *zoomOutButton;
    QMap<int, QVector<QPointF> > curveMap;
    QVector<PlotSettings> zoomStack;
    int curZoom;
    bool rubberBandIsShown;
    QRect rubberBandRect;
    QPixmap pixmap;



};

class PlotSettings
{
public:
    PlotSettings();

    void scroll(int dx, int dy);
    void adjust();
    double spanX() const { return maxX - minX; }
    double spanY() const { return maxY - minY; }

    double minX;
    double maxX;
    int numXTicks;
    double minY;
    double maxY;
    int numYTicks;
    QString xlabel;
    QString ylabel;

private:
    static void adjustAxis(double &min, double &max, int &numTicks);
};

#endif // PLOTTER_H
