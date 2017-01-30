#include <iostream>
#include "calcMetric.h"
#include "fuzzyMembership.h"
#include <QCoreApplication>

using namespace Eigen;
using namespace std;

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    calcMetric calculator;

    MatrixXd mat(3,7);
    mat.row(0) << 0.5,-0.3,0.2,0.1,0.05,-0.1,0.8;
    mat.row(1) << -0.9,-0.2,0.3,-0.1,-0.4,0.05,0.25;
    mat.row(2) << 0.08, 0.2, -0.05, -0.3, 0, 0.1, 0.6;

    calculator.calcAll(mat, 3, 0.2, 2);

    mat.row(0) << 0.08, 0.2, -0.05, -0.3, 0, 0.1, 0.6;
    mat.row(1) << 0.5,-0.3,0.2,0.1,0.05,-0.1,0.8;
    mat.row(2) << -0.9,-0.2,0.3,-0.1,-0.4,0.05,0.25;

    calculator.calcAll(mat, 3, 0.2, 2);

    mat.row(0) << -0.9,-0.2,0.3,-0.1,-0.4,0.05,0.25;
    mat.row(1) << 0.08, 0.2, -0.05, -0.3, 0, 0.1, 0.6;
    mat.row(2) << 0.5,-0.3,0.2,0.1,0.05,-0.1,0.8;

    calculator.calcAll(mat, 3, 0.2, 2);

    mat.row(0) << -0.1,-0.2,-0.3,-0.5,0.9,0.7,0.9;
    mat.row(1) << 0,0.2,0.2,-0.2,-0.4,-0.6,1;
    mat.row(2) << 0.1, 0.9, -0.1, -0.9, 0, 0, 0.1;

    calculator.calcAll(mat, 3, 0.2, 2);

    mat.row(0) << 0.1, 0.9, -0.1, -0.9, 0, 0, 0.1;
    mat.row(1) << -0.1,-0.2,-0.3,-0.5,0.9,0.7,0.9;
    mat.row(2) << 0,0.2,0.2,-0.2,-0.4,-0.6,1;

    calculator.calcAll(mat, 3, 0.2, 2);

    mat.row(0) << 0,0.2,0.2,-0.2,-0.4,-0.6,1;
    mat.row(1) << 0.1, 0.9, -0.1, -0.9, 0, 0, 0.1;
    mat.row(2) << -0.1,-0.2,-0.3,-0.5,0.9,0.7,0.9;

    calculator.calcAll(mat, 3, 0.2, 2);

    mat.row(0) << -0.7,-0.8,-0.9,0,0.9,0.8,0.7;
    mat.row(1) << 0.2, 0.3, 0.5,-0.5,-0.3,-0.8,0.2;
    mat.row(2) << 0.9, 0.9, 0.7, 0.8, 0.2, -0.1, 0.5;

    calculator.calcAll(mat, 3, 0.2, 2);

    mat.row(0) << 0.9, 0.9, 0.7, 0.8, 0.2, -0.1, 0.5;
    mat.row(1) << -0.7,-0.8,-0.9,0,0.9,0.8,0.7;
    mat.row(2) << 0.2, 0.3, 0.5,-0.5,-0.3,-0.8,0.2;

    calculator.calcAll(mat, 3, 0.2, 2);

    mat.row(0) << 0.2, 0.3, 0.5,-0.5,-0.3,-0.8,0.2;
    mat.row(1) << 0.9, 0.9, 0.7, 0.8, 0.2, -0.1, 0.5;
    mat.row(2) << -0.7,-0.8,-0.9,0,0.9,0.8,0.7;

    calculator.calcAll(mat, 3, 0.2, 2);

    mat.row(0) << 0.1,0.2,0.3,0.2,0,0.3,0;
    mat.row(1) << 1,-1,0.5,-0.7,0.9,0.2,-1;
    mat.row(2) << 0.2, 0.2, 0.5, -0.3, -0.1, 0.4, 0.5;

    calculator.calcAll(mat, 3, 0.2, 2);

    mat.row(0) << 1.24,0,0.3,0.2,0,0.3,0;
    mat.row(1) << 0.2, 0.2, 0.5, -0.8, -0.1, 0.4, 0.5;
    mat.row(2) << 1,-1,0.5,-0.7,0.9,0.2,-1;

    calculator.calcAll(mat, 3, 0.2, 2);

    cout << "mat:" << "\n" << mat <<"\n"<< "\n";


    VectorXd dummy;

    cout << "Value:" << "\n" << calculator.getP2P() << "\n" << "\n";

    cout << "History:" << "\n" << calculator.getP2PHistory() << "\n" << "\n";

    cout << "Mean:" << "\n" << (calculator.getP2PHistory()).rowwise().mean() << "\n" << "\n";

    fuzzyMembership kurt;
    cout << "µ: " << "\n" << kurt.getMembership(calculator.getP2PHistory(),calculator.getP2P(), dummy, 1.2, 'm') << "\n";


    return a.exec();
}
