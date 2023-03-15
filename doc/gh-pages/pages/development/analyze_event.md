---
title: Event System
parent: MNE Analyze
grand_parent: Development
nav_order: 2
---
# Event System

This guide covers the event system used in MNE Analyze, not to be confused with the [Event Manager plugin](../documentation/analyze_annotationmanager).

## Overview

In addition to the Qt Framerork's signal/slot system, MNE Analyze uses an event system for all communication between plugins. This event system is integrated in all plugins through the interface IPlugin, allowing all plugins to send and receive events. The event manager itself runs on a separate thread and runs on a cycle will check for buffered events and deliver them if available.

## Events

Events can be used to send data, trigger things, or both. As an example: `SELECTED_MODEL_CHANGED`, one of the most widely used event types, which is triggered by the selection of a new item in the Data Manager, contains a `QSharedPointer<ANSHAREDLIB::AbstractModel>>`, a pointer to the selected data item; while `TRIGGER_REDRAW`, used for making the Signal Viewer update, contains no data.

All event types are declared in `applications/mne_analyze/libs/anShared/Utils/types.h` in the enum `EVENT_TYPE`. Add a new event type by adding an entry to the list. Events ideally should only be used for a single purpose, and should always send and expect the same type of data, if any.

## Sending Events

Events are sent using the Communicator class (applications/mne_analyze/libs/anShared/Management/communicator.h), using `publishEvent()`. When sending an event we pass an event type, and can also pass data though a [QVariant](https://doc.qt.io/qt-5/qvariant.html). Below is a code snippet of the Filtering plugin broadcasting an event with data.

```
void Filtering::setFilterChannelType(const QString& sType)
{
    QVariant data;
    data.setValue(sType);
    m_pCommu->publishEvent(EVENT_TYPE::FILTER_CHANNEL_TYPE_CHANGED, data);
}
```
The string `sType` is added to QVariant `data` with `setValue()`

## Receiving Events

To receive an event or a certain type, a plugin has to subscribe to that event type. This can be done in a plugin's implementation of IPlugin's `getEventSubscriptions()` by returning a vector of all the vent types the plugin is subscribing to. Below is a code snippet of the Averaging plugin subscribing to events.

```
QVector<EVENT_TYPE> Averaging::getEventSubscriptions(void) const
{
    QVector<EVENT_TYPE> temp;
    temp.push_back(SELECTED_MODEL_CHANGED);
    temp.push_back(FILTER_ACTIVE_CHANGED);
    temp.push_back(FILTER_DESIGN_CHANGED);
    temp.push_back(EVENT_GROUPS_UPDATED);
    temp.push_back(CHANNEL_SELECTION_ITEMS);
    temp.push_back(SCALING_MAP_CHANGED);
    temp.push_back(VIEW_SETTINGS_CHANGED);

    return temp;
}
```

Once subscribed, plugins can handle incoming events with `handleEvent()`. Below is how the AnnotationManager (Events) plugin handles its events.

```
void AnnotationManager::handleEvent(QSharedPointer<Event> e)
{
    switch (e->getType()) {
        case EVENT_TYPE::NEW_ANNOTATION_ADDED:
            emit newAnnotationAvailable(e->getData().toInt());
            onTriggerRedraw();
            break;
        case EVENT_TYPE::SELECTED_MODEL_CHANGED:
            onModelChanged(e->getData().value<QSharedPointer<ANSHAREDLIB::AbstractModel> >());
            break;
        default:
            qWarning() << "[AnnotationManager::handleEvent] Received an Event that is not handled by switch cases.";
    }
}
```

## Common Practice

When receiving events, unless any conditional code is very short, we recommend outsourcing any functions or calculations to a separate function for the sake of neatness and code readability. See the function `onModelChanged` used in plugins that receive the `SELECTED_MODEL_CHANGED` event.
