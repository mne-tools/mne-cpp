
#include <QtWidgets>
#include <QtOpenGL>
#include <QDebug>
#include <qmath.h>

#include "glwidget.h"

#define LINES_MORE   1
#define BOXES_MORE   2

GLWidget::GLWidget(QWidget *parent)
    : QGLWidget(QGLFormat(QGL::SampleBuffers), parent)
{
    qtGreen = QColor::fromCmykF(0.40, 0.0, 1.0, 0.0);
    qtPurple = QColor::fromCmykF(0.0, 0.0, 0.0, 0.0);

    NumLines = 0;
    NumBoxes = 0;
    Onset = 0;
    xCoordScale = 1;
    yCoordScale = 1;
    VScale = 10;
    xl = -1000.0;
    xr = 1000.0;
    yt = 1000.0;
    yd = -1000.0;

    NeedBox = false;
    NeedLabel = false;
    DragMode = false;
}

GLWidget::~GLWidget()
{
}

QSize GLWidget::minimumSizeHint() const
{
    return QSize(0, 0);
}

QSize GLWidget::sizeHint() const
{
    //return QSize(400, 400);
    //qDebug()<<"GL:Width="<<this->width();
    //qDebug()<<"GL:Height="<<this->height();
    return QSize(this->width(), this->height());

}

void GLWidget::initializeGL()
{
    //qglClearColor(qtPurple.dark());
    glClearColor(1.0f, 1.0f, 1.0f, 0.0);
    glLoadIdentity();
}

void GLWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT);

    glPushMatrix(); /* GL_MODELVIEW is default */
    glScalef(1.0, 1.0, 1.0);

    glColor3f(0.0f,0.0,0.0);
    glCallList(LINES_MORE);

    if (NeedBox){
    glColor3f(1.0f,0.0,0.0);
    glCallList(BOXES_MORE);
    }
    glPopMatrix();

    if (NeedLabel){
        DrawLabels();
    }
}

void GLWidget::DrawLabels()
{
    for (int i = 0; i< mlabels.size(); i++)
    {
        renderText(xCoordScale*mx[i],yCoordScale*my[i],0.0,mlabels[i]);
    }
}
/*
void GLWidget::paintEvent(QPaintEvent *)
{
    QPainter painter;
    painter.begin(this);
    painter.setRenderHint(QPainter::Antialiasing);
    helper->paint(&painter);
    painter.end();
}
*/
void GLWidget::SetBoxMode(bool boxmode)
{
    NeedBox = boxmode;
}

void GLWidget::SetLabelMode(bool labelmode)
{
    NeedLabel = labelmode;
}
void GLWidget::SetLabels(QList <QString> labels,QVector <float> x,QVector <float> y)
{
    mlabels = labels;
    mx = x;
    my = y;
}

void GLWidget::SetOnset(float ons)
{
    Onset = ons;
}

void GLWidget::SetXYScales(float xScale, float yScale)
{
    xCoordScale = xScale;
    yCoordScale = yScale;
}

void GLWidget::SetVScale(float v)
{
    VScale = v;
}

void GLWidget::drawBoxes(QVector <float> x, QVector <float> y, float w, float h)
{
    if(x.size()!= y.size()) {NumBoxes = 0; qDebug()<<"Error: box coordinates";}

    NumBoxes = x.size();
    qDebug()<<"NumBoxes"<<NumBoxes;
    if (NumBoxes > 0)
    {
        glNewList(BOXES_MORE, GL_COMPILE);
        for (int chanIndx=0; chanIndx < NumBoxes; chanIndx++)
        {
            //qDebug()<<"x,y"<<CoordScale*x[chanIndx]<<CoordScale*y[chanIndx];
            glBegin(GL_LINE_STRIP);
            glColor3f(1.0f,0.0,0.0);
            glVertex2f(xCoordScale*x[chanIndx],  yCoordScale*y[chanIndx]); //left top point
            glVertex2f(xCoordScale*x[chanIndx]+w,yCoordScale*y[chanIndx]); //right top
            glVertex2f(xCoordScale*x[chanIndx]+w,yCoordScale*y[chanIndx]-h); //right bottom
            glVertex2f(xCoordScale*x[chanIndx],  yCoordScale*y[chanIndx]-h); //left bottom
            glVertex2f(xCoordScale*x[chanIndx],  yCoordScale*y[chanIndx]);
            glEnd();

            // horizontal line
            glBegin(GL_LINE_STRIP);
            glColor3f(1.0f,0.0,0.0);
            glVertex2f(xCoordScale*x[chanIndx],  yCoordScale*y[chanIndx]-h/2); //left middle
            glVertex2f(xCoordScale*x[chanIndx]+w,yCoordScale*y[chanIndx]-h/2); //right middle
            glEnd();

            //Onset line
            glBegin(GL_LINE_STRIP);
            glColor3f(1.0f,0.0,0.0);
            glVertex2f(xCoordScale*x[chanIndx]+Onset,  yCoordScale*y[chanIndx]); //left middle
            glVertex2f(xCoordScale*x[chanIndx]+Onset,yCoordScale*y[chanIndx]-h); //right middle
            glEnd();
        }
        glLineWidth(4);
        glBegin(GL_LINE_STRIP);
        glColor3f(1.0f,0.0,0.0);//xl, xr, yd, yt,
        glVertex2f(xr,yt); //right top
        glVertex2f(xr,yt-h); //right bottom
        glEnd();
        glLineWidth(1);
        qDebug()<<"scale mark"<<xl<<w<<yd<<h;
        glEndList();
    }
}
void GLWidget::drawLines(float * samples, int row, int col, int wise_type,
                         QVector <float> x, QVector <float> y, float w, float h)
{
    NumLines = row;
    //drawOneLine(samples, row, col, wise_type, 0, 0.0, 0.0);
    //qDebug()<<"col"<<col;
    //qDebug()<<"row"<<row<<"x size"<<x.size()<<"y size"<<y.size();
    if((x.size()!=row) && (y.size()!=row)) NumLines = 0;
    if (NumLines>0){
        glNewList(LINES_MORE, GL_COMPILE);
        for (int chanIndx=0; chanIndx < row; chanIndx++)
            drawOneLine(samples, row, col, wise_type, chanIndx, xCoordScale*x[chanIndx], yCoordScale*y[chanIndx], w, h);// 0.0,chanIndx*10.0);//

        glEndList();
    }
}

void GLWidget::drawOneLine(float * samples, int row, int col, int wise_type, int chanIndx,
                           float x, float y, float w, float h)
{
    //    qDebug()<<"drawlines"<<samples[0*row+chanIndx]<<samples[1*row+chanIndx];
    //    qDebug()<<"drawlines1"<<samples[0+chanIndx*col]<<samples[1+chanIndx*col];

    float wscale = w/col;
    float hscale = h/VScale;
    glBegin(GL_LINE_STRIP);
    if(wise_type==0){ //0 --column wise
        for(int i=0;i<col;i++){
            glVertex2f(i*wscale+x,y-h/2+hscale*samples[i*row+chanIndx]);
        }
    }
    else
    { // 1 -- raw wise
        for(int i=0;i<col;i++){
            glVertex2f(i*wscale+x,y-h/2+hscale*samples[i+chanIndx*col]);
        }
    }
    glEnd();
}

void GLWidget::resizeGL(int width, int height)
{
    glViewport( 0, 0, (GLint)width, (GLint)height );
    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
    glOrtho( xl, xr, yd, yt, -1.0, 1.0 );
    glMatrixMode( GL_MODELVIEW );
}

void GLWidget::SetGLView(float xleft, float xright, float ydown, float ytop)
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

void GLWidget::mousePressEvent(QMouseEvent *event)
{
    startPos = event->pos();
    qDebug()<<startPos;
    DragMode = false;
}

void GLWidget::mouseMoveEvent(QMouseEvent *event)
{
    MovePos = event->pos();
    qDebug()<<MovePos;
    DragMode = true;
}
void GLWidget::mouseReleaseEvent(QMouseEvent *event)
{
    lastPos = event->pos();
    qDebug()<<lastPos;

    if (DragMode){
    // coordinates transform from 0,0 to center [Screen Coordinates]
    QPoint CenterPos(this->width()/2,this->height()/2);

    startPos = startPos - CenterPos;
    lastPos = lastPos - CenterPos;

    QPoint NewStartPos(((xr-xl)/this->width())*startPos.x()+(xr+xl)/2, (yt-yd)*startPos.y()/this->height()+(yt+yd)/2);

    QPoint NewLastPos(((xr-xl)/this->width())*lastPos.x()+(xr+xl)/2, (yt-yd)*lastPos.y()/this->height()+(yt+yd)/2);

    float xsl = NewStartPos.x();
    float xsr = NewLastPos.x();
    float yst = NewStartPos.y();
    float ysd = NewLastPos.y();

    if (xsl > xsr) {float t = xsr; xsr = xsl; xsl = t;}
    if (ysd > yst) {float t = yst; yst = ysd; ysd = t;}

    qDebug()<<"Width"<<this->width();
    qDebug()<<xsl<<xsr<<ysd<<yst;

    SetGLView(xsl, xsr, ysd, yst);
    updateGL();
    }
    DragMode = false;
}
