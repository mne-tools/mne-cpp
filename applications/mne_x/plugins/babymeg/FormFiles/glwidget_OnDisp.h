#ifndef GLWIDGETONDISP_H
#define GLWIDGETONDISP_H

#include <QGLWidget>

//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================
//#include "include/3rdParty/Eigen/Core"
#include <Eigen/Core>

using namespace Eigen;


class GLWidget_OnDisp : public QGLWidget
{
    Q_OBJECT

public:
    GLWidget_OnDisp( QWidget *parent = 0);
    ~GLWidget_OnDisp();

    QSize minimumSizeHint() const;
    QSize sizeHint() const;

public slots:

signals:

protected:
    void initializeGL();
    void paintGL();
    void resizeGL(int width, int height);
    //void paintEvent(QPaintEvent *);

private:
    QColor qtGreen;
    QColor qtPurple;

public:
    void SetGLView(float xleft, float xright, float ydown, float ytop);
    void SetdrawLines(MatrixXf tmp,int wise_type, int chnind,
                                       float x, float y, float w, float h);
    void SetChnInd(int chnind);
    void drawAx(float xleft, float xright, float ytop, float ydown);
    void drawLines(float * samples, int row, int col, int wise_type,
                             float x, float y, float w, float h);
    void drawOneLine(float * samples, int row, int col, int wise_type, int chanIndx, float x, float y, float w, float h);
public:
    float xl,xr,yt,yd;
    int NumLines;
    float xCoordScale;
    float yCoordScale;

    MatrixXf m_tmp;
    float * m_samples;
    int m_row;
    int m_col;
    int m_wise_type;
    int m_chnind;
    int m_x;
    int m_y;
    int m_w;
    int m_h;
    bool linePlot;
    bool axPlot;
};


#endif // GLWIDGET_H
