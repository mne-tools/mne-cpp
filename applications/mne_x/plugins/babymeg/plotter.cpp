#include "plotter.h"

plotter::plotter(QWidget *parent)
    : QWidget(parent)
{
    setBackgroundRole(QPalette::Dark);
    setAutoFillBackground(true);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    setFocusPolicy(Qt::StrongFocus);
    rubberBandIsShown = false;

    zoomInButton = new QToolButton(this);
    zoomInButton->adjustSize();
    connect(zoomInButton,SIGNAL(clicked()), this, SLOT(zoomIn()));

    zoomOutButton = new QToolButton(this);
    zoomOutButton->adjustSize();
    connect(zoomOutButton,SIGNAL(clicked()), this, SLOT(zoomOut()));

    setPlotSettings(PlotSettings());

}

void plotter::setPlotSettings(const PlotSettings &settings)
{
    zoomStack.clear();
    zoomStack.append(settings);
    curZoom = 0;
    zoomInButton->hide();
    zoomOutButton->hide();
    refreshPixmap();
}


void plotter::zoomOut()
{
    if(curZoom > 0){
        --curZoom;
        zoomOutButton->setEnabled(curZoom > 0);
        zoomInButton->setEnabled(true);
        zoomInButton->show();
        refreshPixmap();
    }
}

void plotter::zoomIn()
{
    if(curZoom < zoomStack.count() - 1){
        ++curZoom;
        zoomInButton->setEnabled(curZoom < zoomStack.count() - 1);
        zoomOutButton->setEnabled(true);
        zoomOutButton->show();
        refreshPixmap();
    }
}

void plotter::setCurveData(int id, const QVector<QPointF> &data)
{
    curveMap[id] = data;
    refreshPixmap();
}

void plotter::clearCurve(int id)
{
    curveMap.remove(id);
    refreshPixmap();
}

QSize plotter::minimumSizeHint() const
{
    return QSize(6*Margin,4*Margin);
}

QSize plotter::sizeHint() const
{
    return QSize(12*Margin, 8*Margin);
}

void plotter::paintEvent(QPaintEvent * /*event*/)
{
    QStylePainter painter(this);
    painter.drawPixmap(0,0,pixmap);

    if(rubberBandIsShown){
        painter.setPen(palette().light().color());
        painter.drawRect(rubberBandRect.normalized().adjusted(0,0,-1,-1));
    }

    if(hasFocus())
    {
        QStyleOptionFocusRect option;
        option.initFrom(this);
        option.backgroundColor = palette().dark().color();
        painter.drawPrimitive(QStyle::PE_FrameFocusRect, option);
    }
}

void plotter::resizeEvent(QResizeEvent * /*event*/)
{
    int x = width() - (zoomInButton->width() + zoomOutButton->width() + 10);

    zoomInButton->move(x,5);
    zoomOutButton->move(x + zoomInButton->width() +5, 5);
    refreshPixmap();
}

void plotter::refreshPixmap()
{
    pixmap = QPixmap(size());
    pixmap.fill(this, 0, 0);

    QPainter painter(&pixmap);
    painter.initFrom(this);
    drawGrid(&painter);
    drawCurve(&painter);
    update();
}

void plotter::drawGrid(QPainter *painter)
{
    QRect rect(xMargin, Margin, width()-2*xMargin, height() -2*Margin);
    if(!rect.isValid()) return;

    PlotSettings settings = zoomStack[curZoom];
    QPen quiteDark = palette().dark().color().light();
    QPen light = palette().light().color();

    for(int i=0; i<=settings.numXTicks;++i)
    {
        int x = rect.left() + (i*(rect.width()-1)/settings.numXTicks);
        double label = settings.minX + (i*settings.spanX()/settings.numXTicks);
        painter->setPen(quiteDark);
        painter->drawLine(x,rect.top(),x,rect.bottom());
        painter->setPen(light);
        painter->drawLine(x,rect.bottom(),x,rect.bottom()-5);
        painter->drawText(x,rect.bottom()+5,100,20,Qt::AlignLeft, QString::number(label));
    }

    for(int j=0; j<=settings.numYTicks;++j)
    {
        int y = rect.bottom() - (j*(rect.height()-1)/settings.numYTicks);
        double label = settings.minY + (j*settings.spanY()/settings.numYTicks);
        painter->setPen(quiteDark);
        painter->drawLine(rect.left(),y,rect.right(),y);
        painter->setPen(light);
        painter->drawLine(rect.left()+5 ,y, rect.left(),y);
        painter->drawText(rect.left()- xMargin -20, y-10, 100,20,
                          Qt::AlignVCenter|Qt::AlignRight, QString::number(label));
    }
    painter->drawRect(rect.adjusted(0,0,-1,-1));

}

void plotter::drawCurve(QPainter *painter)
{
    static const QColor colorForIds[6] = {Qt::red,Qt::green,Qt::blue,Qt::cyan,Qt::magenta,Qt::yellow};

    PlotSettings settings = zoomStack[curZoom];
    QRect rect(xMargin, Margin, width()-2*xMargin, height()-2*Margin);
    if(!rect.isValid()) return;

    painter->setClipRect(rect.adjusted(+1,+1,-1,-1));
    QMapIterator <int, QVector<QPointF>> i(curveMap);
    while (i.hasNext()) {
       i.next();
       int id = i.key();
       QVector<QPointF> data = i.value();
       QPolygonF polyline(data.count());
       for(int j=0; j<data.count(); ++j){
           double dx = data[j].x() - settings.minX;
           double dy = data[j].y() - settings.minY;
           double x = rect.left() + (dx*(rect.width()-1)/settings.spanX());
           double y = rect.bottom() - (dy *(rect.height()-1)/settings.spanY());
           polyline[j] = QPointF(x,y);
       }
       painter->setPen(colorForIds[uint(id) % 6]);
       painter->drawPolyline(polyline);
    }
}

PlotSettings::PlotSettings()
{
    minX = 0.0;
    maxX = 10.0;
    numXTicks = 5;

    minY = -10.0;
    maxY = 10.0;
    numYTicks = 5;
}

void PlotSettings::adjustAxis(double &min, double &max, int &numTicks)
{
    const int MinTicks = 4;
    double grossStep = (max-min)/ MinTicks;
    double step = std::pow(10.0, std::floor(std::log10(grossStep)));

    if(5*step<grossStep){
        step *= 5;
    }
    else if(2*step < grossStep){
        step *= 2;
    }

    numTicks = int(std::ceil(max/step) - std::floor(min/step));
    if(numTicks < MinTicks)
        numTicks = MinTicks;
    min = std::floor(min/step)*step;
    max = std::ceil(max/step)*step;
}

