#ifndef MATRIXVIEW_H
#define MATRIXVIEW_H

#include <QWidget>

namespace Ui {
class MatrixView;
}

class MatrixView : public QWidget
{
    Q_OBJECT
    
public:
    explicit MatrixView(QWidget *parent = 0);
    ~MatrixView();
    
private:
    Ui::MatrixView *ui;
};

#endif // MATRIXVIEW_H
