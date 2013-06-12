#ifndef MATRIXVIEW_H
#define MATRIXVIEW_H

#include <QWidget>
#include <QImage>
#include <QString>
#include <QPen>

#include <Eigen/Core>


using namespace Eigen;

class MatrixView : public QWidget
{
    Q_OBJECT
    
public:
    explicit MatrixView(QWidget *parent = 0);
    ~MatrixView();

    void updateMatrix(MatrixXd &data);

protected:

    int R(double v);
    int G(double v);
    int B(double v);
    double slopeMRaising(double x, double n);
    double slopeMFalling(double x, double n);



    void paintEvent(QPaintEvent*);
    void resizeEvent(QResizeEvent*);
    
private:
//    MatrixXd
    QPixmap* pixmap;
    QSize widgetSize;

    qint32 m_iBorderTopBottom;
    qint32 m_iBorderLeftRight;

    double m_qMinValue;
    double m_qMaxValue;

    QString m_sTitle;
    QFont m_qFontTitle;
    QPen m_qPenTitle;

    QString m_sXLabel;
    QString m_sYLabel;
    QFont m_qFontAxes;
    QPen m_qPenAxes;


};

#endif // MATRIXVIEW_H
