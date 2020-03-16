#ifndef FIFFRAWEVENT_H
#define FIFFRAWEVENT_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fiff/fiff_ch_info.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QWidget>
#include <QMap>

namespace Ui {
    class EventWindowDockWidget;
}

class FiffRawEvent : public QWidget
{
public:
    FiffRawEvent();

    Ui::EventWindowDockWidget* ui;
};

#endif // FIFFRAWEVENT_H
