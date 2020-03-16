#include "fiffrawevent.h"
#include "ui_fiffrawevent.h"

FiffRawEvent::FiffRawEvent()
: ui(new Ui::EventWindowDockWidget)
{
    ui->setupUi(this);
}
