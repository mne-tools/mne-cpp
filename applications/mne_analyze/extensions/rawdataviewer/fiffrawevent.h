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

protected slots:

    //=========================================================================================================
    /**
     * jumpToEvent jumps to a event specified in the event table view
     *
     * @param [in] current model item focused in the view
     * @param [in] previous model item focused in the view
     */
    void jumpToEvent(const QModelIndex &current, const QModelIndex &previous);

    //=========================================================================================================
    /**
     * jumpToEvent jumps to a event specified in the event table view
     */
    void removeEventfromEventModel();

    //=========================================================================================================
    /**
     * Adds an event to the event model and its QTableView
     */
    void addEventToEventModel();

    //=========================================================================================================
    /**
     * call this function whenever a new event type is to be added
     */
    void addNewEventType();


    Ui::EventWindowDockWidget* ui;
};

#endif // FIFFRAWEVENT_H
