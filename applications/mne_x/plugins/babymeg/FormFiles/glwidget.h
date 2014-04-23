#ifndef GLWIDGET_H
#define GLWIDGET_H

#include <QGLWidget>


class GLWidget : public QGLWidget
{
    Q_OBJECT

public:
    GLWidget( QWidget *parent = 0);
    ~GLWidget();

    QSize minimumSizeHint() const;
    QSize sizeHint() const;

public slots:

signals:

protected:
    void initializeGL();
    void paintGL();
    void resizeGL(int width, int height);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    //void paintEvent(QPaintEvent *);

private:
    QPoint startPos;
    QPoint lastPos;
    QPoint MovePos;
    QColor qtGreen;
    QColor qtPurple;

public:
    void drawLines(float * samples, int row, int col, int wise_type,QVector <float> x, QVector <float> y, float w, float h);
    void drawOneLine(float * samples, int row, int col, int wise_type, int chanIndx, float x, float y, float w, float h);
    void SetGLView(float xleft, float xright, float ydown, float ytop);
    void drawBoxes(QVector <float> x, QVector <float> y, float w, float h);
    void SetXYScales(float xCoordScale, float yCoordScale);
    void SetOnset(float ons);
    void SetVScale(float v);
    void SetBoxMode(bool boxmode);
    void SetLabelMode(bool labelmode);
    void SetLabels(QList <QString> labels,QVector <float> x,QVector <float> y);
    void DrawLabels();
public:
    int NumLines;
    int NumBoxes;
    float VScale;
    float Onset;
    bool NeedBox;
    bool NeedLabel;
    float xCoordScale, yCoordScale;
    float xl,xr,yt,yd;
    QList <QString> mlabels;
    QVector <float> mx;
    QVector <float> my;

    bool DragMode;
};


#endif // GLWIDGET_H
