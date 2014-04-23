
#include <QtWidgets>
#include <QtOpenGL>
#include <QDebug>
#include <qmath.h>

#include "glwidget_OnDisp.h"

#define LINES_BUTT   3


GLWidget_OnDisp::GLWidget_OnDisp(QWidget *parent)
    : QGLWidget(QGLFormat(QGL::SampleBuffers), parent)
{
    qtGreen = QColor::fromCmykF(0.40, 0.0, 1.0, 0.0);
    qtPurple = QColor::fromCmykF(0.0, 0.0, 0.0, 0.0);
    xl = -200.0;
    xr = 200.0;
    yt = 200.0;
    yd = -200.0;
    xCoordScale = 1.0;
    yCoordScale = 1.0;
    m_row = 0;
    m_col = 0;
    linePlot = false;
    axPlot = false;
}

GLWidget_OnDisp::~GLWidget_OnDisp()
{
}

QSize GLWidget_OnDisp::minimumSizeHint() const
{
    return QSize(0, 0);
}

QSize GLWidget_OnDisp::sizeHint() const
{
    //return QSize(400, 400);
    qDebug()<<"GL:Width="<<this->width();
    qDebug()<<"GL:Height="<<this->height();
    return QSize(this->width(), this->height());

}

void GLWidget_OnDisp::initializeGL()
{
    //qglClearColor(qtPurple.dark());
    glClearColor(1.0f, 1.0f,1.0f, 0.0);
    glLoadIdentity();
}

void GLWidget_OnDisp::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT);

    glPushMatrix(); /* GL_MODELVIEW is default */
    glScalef(1.0, 1.0, 1.0);

    /*
    // horizontal line
    glBegin(GL_LINE_STRIP);
    glColor3f(0.0f,0.0,0.0);
    glVertex2f(100, 100); //left middle
    glVertex2f(-100,-100); //right middle
    glEnd();
    */
    if (axPlot)
    {
        glColor3f(0.0f,0.0f,0.0f);
        drawAx(m_x, m_x+m_w, m_y, m_y-m_h);
    }
    if (linePlot)
    {
        glColor3f(0.0f,0.0f,1.0f);
        //glCallList(LINES_BUTT);
        drawLines(m_samples, m_row, m_col, m_wise_type, m_x, m_y, m_w, m_h);
    }
    glPopMatrix();
}

void GLWidget_OnDisp::resizeGL(int width, int height)
{
    glViewport( 0, 0, (GLint)width, (GLint)height );
    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
    glOrtho( xl, xr, yd, yt, -1.0, 1.0 );
    glMatrixMode( GL_MODELVIEW );
}

void GLWidget_OnDisp::SetGLView(float xleft, float xright, float ydown, float ytop)
{
    xl = xleft;
    xr = xright;
    yd = ydown;
    yt = ytop;

    // reset port view
    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
    glOrtho( xl, xr, yd, yt, -1.0, 1.0 );
    glMatrixMode( GL_MODELVIEW );
}
void GLWidget_OnDisp::SetdrawLines(MatrixXf tmp,int wise_type, int chnind, float x, float y, float w, float h)
{
    m_tmp = tmp;
    m_row = m_tmp.rows();
    m_col = m_tmp.cols();

    m_chnind = chnind;

    //set the opengl window according to the current data
    float xleft = -200;
    float xright = 200;
    float ydown = -150;//m_tmp(chnind,0) - 2*m_tmp(chnind,0);
    float ytop  = 150;//m_tmp(chnind,0) + 2*m_tmp(chnind,0);

    SetGLView(xleft, xright, ydown, ytop);

    m_wise_type = wise_type;
    m_x = -150;//x;
    m_y = 100;//ytop-m_tmp(chnind,0);//y;
    m_w = 300;
    m_h = 200;//2*m_tmp(chnind,0);

    for(int i = 0; i<m_col;i++)
        m_tmp(chnind,i) -= m_tmp(chnind,0);

    m_samples = m_tmp.data();


    linePlot = true;
    axPlot = true;
    updateGL();
}
void GLWidget_OnDisp::SetChnInd(int chnind)
{
    m_chnind = chnind;
    qDebug()<<"GLWidget_OnDisp:chnind : "<<chnind;
    if (m_row == 0)
        linePlot = false;
    else
    {
        linePlot = true;
        updateGL();
    }
}
void GLWidget_OnDisp::drawAx(float xleft, float xright, float ytop, float ydown)
{

    glLineWidth(2.0f);
    glBegin(GL_LINE_STRIP);
    glVertex2f(xleft, ytop);
    glVertex2f(xright, ytop);
    glVertex2f(xright, ydown);
    glVertex2f(xleft, ydown);
    glVertex2f(xleft, ytop);
    glEnd();
    glLineWidth(1.0f);

    glBegin(GL_LINES);

    glVertex2f(xleft, ytop-(ytop-ydown)/3);
    glVertex2f(xleft+(xright-xleft)/20, ytop-(ytop-ydown)/3);

    glVertex2f(xleft, ytop-2*(ytop-ydown)/3);
    glVertex2f(xleft+(xright-xleft)/20, ytop-2*(ytop-ydown)/3);

    glVertex2f(xleft+(xright-xleft)/3, ydown);
    glVertex2f(xleft+(xright-xleft)/3, ydown+(ytop-ydown)/20);

    glVertex2f(xleft+2*(xright-xleft)/3, ydown);
    glVertex2f(xleft+2*(xright-xleft)/3, ydown+(ytop-ydown)/20);

    glEnd();

}

void GLWidget_OnDisp::drawLines(float * samples, int row, int col, int wise_type,
                         float x, float y, float w, float h)
{
    NumLines = row;
    if (NumLines>0){
        //glNewList(LINES_BUTT, GL_COMPILE);
        int chanIndx = m_chnind;
        //for (int chanIndx=0; chanIndx < row; chanIndx++)
        //qDebug()<<"GLWidget_OnDisp:drawLines (chnind) : "<<m_chnind;

            drawOneLine(samples, row, col, wise_type, chanIndx, xCoordScale*x, yCoordScale*y, w, h);
        //glEndList();
    }
}
void GLWidget_OnDisp::drawOneLine(float * samples, int row, int col, int wise_type, int chanIndx,
                           float x, float y, float w, float h)
{

    float wscale = w/col;
    float hscale = 2;//h/10;

    glBegin(GL_LINE_STRIP);
    if(wise_type==0){ //0 --column wise
        for(int i=0;i<col;i++){
            glVertex2f(i*wscale+x,hscale*(y-h/2+samples[i*row+chanIndx]));
        }
    }
    else
    { // 1 -- raw wise
        for(int i=0;i<col;i++){
            glVertex2f(i*wscale+x,hscale*(y-h/2+samples[i+chanIndx*col]));
        }
    }
    glEnd();
}
