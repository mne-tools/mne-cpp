#include "matrixview.h"
#include "ui_matrixview.h"

MatrixView::MatrixView(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MatrixView)
{
    ui->setupUi(this);
}

MatrixView::~MatrixView()
{
    delete ui;
}
