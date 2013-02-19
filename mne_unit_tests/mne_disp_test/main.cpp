#include "MneDispTest.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MneDispTest w;
    w.show();
    
    return a.exec();
}
