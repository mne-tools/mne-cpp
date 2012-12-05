#include "mnebrowserawq.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MneBrowseRawQ w;
    w.show();
    
    return a.exec();
}
