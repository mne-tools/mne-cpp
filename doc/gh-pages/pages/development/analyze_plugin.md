---
title: Creating a new plugin
parent: MNE Analyze
grand_parent: Development
nav_order: 1
---
# Creating a New Plugin

This guide covers the creation of a new plugin.

## Overview

All MNE Analyze plugins are subclassed from the same base, AbstractPlugin (`/applications/mne_analyze/libs/anShared/Plugins/abstractplugin.h`). Unlike MNE Scan, MNE Analyze does not differentiate between types of plugins. MNE Analyze plugins are loaded into MNE Analyze at run time from `bin/mne_analyze_plugins`. Once loaded, MNE Analyze gets the menus, controls, and views from each of the plugins and displays them to the user. Currently the plugins are limited to returning one of each of these. For details on how these steps are done, see `/applications/mne_analyze/mne_analyze/analyzecore.cpp` function `initPluginManager()` for how plugins are loaded, and the constructor for `MainWindow` in `/applications/mne_analyze/mne_analyze/mainwindow.cpp` for how the GUI elements are loaded.

## Sample Plugin

A sample plugin with no functionality is included in `/applications/mne_analyze/plugins/sampleplugin`. A good way to start implementing a plugin is to duplicate that folder and its contents and use an IDE to replace all instances of 'SamplePlugin' with the name of the new plugin. See the end of the guide for some easy to miss details on getting new plugins to work correctly.

## Deriving from AbstractPlugin

AbstractPlugin has a number of pure virtual functions that need to be defined by any new plugin. Among them are functions for getting the view, control, and menu GUI items for the plugin, as well subscribing to and receiving events from the event manager.

### clone()

Returns an instance of the plugin. This is not a copy. Most of the existing plugins do something like this:

```
    return QSharedPointer<AbstractPlugin> (new Averaging);
```

### init()

Initializes the plugin. This gets called after all the plugins get loaded into the Plugin Manager. Any startup actions that need to be done before the user begins interacting with the plugin, such as allocating memory or initializing parameters, can be done here.

### unload()

Closes the plugin. Gets called when the plugins are being shutdown when the main window is closed. Any shutdown actions that need to be performed before the program shuts down can be done here.

### getName()

Returns the plugin name, this will be used to set the plugin title on any views, controls, and menu items the plugin has.

### getMenu()

Returns menu items to be added to the top dropdown menu bar. If the plugin does not need a menu, return a `Q_NULLPTR` (null pointer) instead.

Create a menu object to return, and populate it with relevant actions. Below is a section of the data loader plugin as an example.

```
QMenu* pMenuFile = new QMenu(tr("File"));

QAction* pActionLoad = new QAction(tr("Open"));
pActionLoad->setStatusTip(tr("Load a data file"));
connect(pActionLoad, &QAction::triggered,
        this, &DataLoader::onLoadFilePressed);
```

If the name of the menu matches an existing one, the new actions will be added to that existing menu, otherwise a new one will be created.

### getControl()

Returns a QDockWidget* containing the control GUI elements of the plugin. If the plugin does not need controls, return a `Q_NULLPTR` (null pointer) instead. Here we also set the allowed docking areas and the tab name that appears when the plugin is docked.

The controls are set by adding any QWidget to the QDockWidget that needs to be returned. Below is an example of a QDockWidget with a single widget added, with the tab name set to the plugin name, that can be docked on the left or right sides of the window.

```
QDockWidget* pControlDock = new QDockWidget(getName());
pControlDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
pControlDock->setObjectName(getName());

QPushButton* pSingleButton = new QPushButton("Compute");
pControlDock->setWidget(pSingleButton);
```

This is useful if the control widget is already created and all that needs to be done is add it to the dock widget. If instead the control widget needs to be created or multiple control widgets need to be added, a layout can be used to add multiple elements to the Widget being set in the QDockWidget.

```
QDockWidget* pControlDock = new QDockWidget(getName());
pControlDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
pControlDock->setObjectName(getName());

QScrollArea* wrappedScrollArea = new QScrollArea(pControlDock);
QVBoxLayout* pLayout = new QVBoxLayout;
pControlDock->setWidget(wrappedScrollArea);

QPushButton* pFirstButton = new QPushButton("Compute");
QPushButton* pSecondButton = new QPushButton("Clear");

pLayout->addWidget(pFirstButton);
pLayout->addWidget(pSecondButton);
```

A lot of control widgets are already implemented in the disp library, they can be used here.

Many different plugins will be loaded to MNE_Analyze, to influence how high or low the controls of a particular plugin will appear in the drop-down menu "Control" tab, you can modify the variable ```m_iOrder```. By default it is set to 0, the higher you set that value to, the higher your control will appear. To force the controls to appear lower you can also use negative values.

### getView()

Returns a QWidget* with the view GUI element of the plugin. If the plugin does not need a view, return a `Q_NULLPTR` (null pointer) instead.

In all cases in MNE Analyze the view is created elsewhere and we simply return an instance of it here.

A lot of view widgets are already implemented in the disp library, they can be used here.

### getEventSubscriptions()

Returns a QVector with the events the plugin is subscribed to. If the plugin does not care about incoming events, return an empty QVector. See example below:

```
QVector<EVENT_TYPE> temp;
temp.push_back(SELECTED_MODEL_CHANGED);
temp.push_back(FILTER_ACTIVE_CHANGED);
temp.push_back(FILTER_DESIGN_CHANGED);
```

### handleEvent()

Receives event `e` that the plugin is subscribed to. Typically this is handled by a switch case for each of the subscribed events:

```
switch (e->getType()) {
    case EVENT_TYPE::SELECTED_MODEL_CHANGED:
        // do something
        break;
    case FILTER_ACTIVE_CHANGED:
        // do something
        break;
    case FILTER_DESIGN_CHANGED:
        // do something
        break;
    default:
        qWarning() << "[Averaging::handleEvent] Received an Event that is not handled by switch cases.";
```

## Views and Controls

Existing views and controls are in the library layer, in the disp library. These can be used when making you plugin's view or controls. Otherwise, custom ones can be made with QtCreator using `.ui` files. All of the views and controls used subclass from `AbstractView`.

### Using the event system

Events are how the plugins in MNE Analyze communicate with each other. If your plugin needs to know about any new data that was loaded, any new item being selected, or wants to make use of any of the scaling or channel selection controls already provided by other plugins, you can subscribe to the relevant events. If you plugin needs to instead send out data, you can send the relevant events or create new ones to suit you needs. See the page on the [event system](analyze_event.md).

### Setting/Clearing Data

Data is displayed to the user using views, which display the data contained in models, in a [Model/View](https://doc.qt.io/qt-5/model-view-programming.html) setup.

## Static Building

When building statically, MNE Analyze needs to be told about the plugins at build time. To do this, the plugins are included with `Q_IMPORT_PLUGIN` in `applications/mne_analyze/mne_analyze/main.cpp` under the `#ifdef STATICBUILD` macro, as well as in `applications/mne_analyze/mne_analyze/mne_analyze.pro` under the section that checks for static builds, `contains(MNECPP_CONFIG, static)`.

## Getting the New Plugin Running

 - Make sure the new plugin is included in `plugins.pro` so that it is included in the build.

 - Make sure that the `.pro` file for your plugin has the correct file names, target, and included Libraries

 - Make sure your plugin has a `*_global.h` file.

## Useful References

 - [Guide on Layout Management in Qt](https://doc.qt.io/qt-5/layout.html)

 - [Guide on Basics of User Interface Design](https://www.usability.gov/what-and-why/user-interface-design.html)
