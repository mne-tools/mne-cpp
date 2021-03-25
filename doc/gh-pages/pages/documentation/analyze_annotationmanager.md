---
title: Event Manager
has_children: false
parent: MNE Analyze
grand_parent: Documentation
nav_order: 4
---
## Event Manager

The event manager is used to create, display, and save events. Events in MNE Analyze are sorted into event groups, each with an associated name, type, and color. Events and event groups can either be manually added or detected from stim channels of the currently loaded file.

![](../../images/analyze/mne_an_annotationmanager_2.png)

### Adding Events Manually

To add an event group, select a name and press `Create`. Select a color when prompted.

![](../../images/analyze/mne_an_annotationmanager_4.png)

To add an event, right click on the data plot in the spot you wish to add, and select the `Add Event` option. The event will be added to the currently selected group.

Alternatively, hit the `E` key to add an event to where the signal viewer cursor is currently located.

![](../../images/analyze/mne_an_annotationmanager_3.png)

### Adding Events from Stim

Click the `Load from Stim Channels` button to bring up the Trigger Detection widget.

![](../../images/analyze/mne_an_annotationmanager_5.png)

Select the channel from which to load events, and a threshold for detecting triggers.

Press `Detect Triggers` to load in events. This may take a few seconds depending on how many events are being loaded.

### Managing and Viewing Events

Event groups will be added to the left side of the event table. The right side will display the events in the currently selected event group. Event groups can be recolored or deleted by right clicking them and selecting the corresponding action. To rename groups, double click them and input a new name.

The display of events can also be narrowed or broadened. Select multiple groups buy holding `Ctrl` when clicking through groups, or use `Shift` to select a range of groups. Checking `Show selected events only` will make only the currently selected events display in the Signal Viewer, use `Ctrl` and `Shift` to select multiple events as well.

To save the events currently showing on the right side of the table to an `.eve` file, press `Save Events`.
