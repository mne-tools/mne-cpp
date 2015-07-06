#include "mriviewer.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QGuiApplication::setApplicationDisplayName(MriViewer::tr("MGH Test Viewer"));

    MriViewer widget;
    widget.show();

    return app.exec();
}
