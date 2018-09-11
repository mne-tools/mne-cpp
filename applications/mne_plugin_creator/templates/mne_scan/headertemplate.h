//=============================================================================================================
/**
* @file     {{header_filename}}
* @author   {{author}} <{{author_email}}>
*           Erik Hornberger <erik.hornberger@shi-g.com>
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     {{month}}, {{year}}
*
* @section  LICENSE
*
* Copyright (C) {{year}}, {{author}}, Sumitomo Heavy Industries, Ltd., Christoph Dinh, and Matti Hamalainen.
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that
* the following conditions are met:
*     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
*       following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
*       the following disclaimer in the documentation and/or other materials provided with the distribution.
*     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
*       to endorse or promote products derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*
* @brief    Contains the declaration of the {{name}} class.
*
*/

#ifndef {{header_define}}
#define {{header_define}}


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "{{global_header_filename}}"

#include <scShared/Interfaces/{{superclass}}.h>
#include <utils/generics/circularmatrixbuffer.h>
#include <scMeas/realtimemultisamplearray.h>
#include "FormFiles/{{setup_widget_header_filename}}"
#include "FormFiles/{{widget_header_filename}}"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>
#include <QtCore/QtPlugin>
#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE {{name}}
//=============================================================================================================

namespace {{namespace}}
{


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace SCSHAREDLIB;

//=============================================================================================================
/**
* DECLARE CLASS {{name}}
*
* @brief The {{name}} class does...
*/
class {{export_define}} {{name}} : public {{superclass}}
{
    Q_OBJECT                                                            // All subclasses of QObject must have this macro
    Q_PLUGIN_METADATA(IID "scsharedlib/1.0" FILE "{{json_filename}}")   // New Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    Q_INTERFACES(SCSHAREDLIB::{{superclass}})                           // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces

public:
    
    /**
     * @brief Creates a new instance of `{{name}}`
     * 
     * Constructs a `{{name}}` object. The `ISensor` interface requires
     * and uses a constructor that takes no arguments, so we cannot perform
     * dependency injection. Nothing should be initialized in the constructor
     * because it gets called by MNE to generate a list of plugins. The `init()`
     * method on `IPlugin` is the proper place to perform initialization.
     */
    {{name}}();

    /**
     * @brief Destroy an instance of `{{name}}`
     *
     * Destroy an instance of `{{name}}` and cleans up memory.
     * Like the constructor above, `unload()` should be used in place of
     * the desctructor.
     */
    ~{{name}}();

// QThread interface
protected:

    /**
     * @brief Processing takes place in this loop. Do not return while plugin is still running.
     * @see QThread::execute() and QThread::quit()
     * 
     * This is the most central function. When `start` is called, we enter the run loop.
     * Do not return from the run loop until `stop` is called. All processing will take place
     * within run.
     *
     * During the run loop, we check to see if there is any data in the buffer. If there is,
     * process it and then append the result to the output of plugin.
     *
     * The speed the run loop runs at is dictated by `m_pBuffer`. It's pop method blocks until
     * data becomes available.
     */
    void run();

// IPlugin interface
public:

    /**
     * @brief Create a copy of the plugin
     */
    QSharedPointer<IPlugin> clone() const;

    /**
     * @brief instantiates a new instance when a block is placed on the stage
     * 
     * This is where everything should be setup. Instantiate all of the properties and connect their
     * signals and slots. This is the function that is triggered when the function block is drag-and-
     * dropped onto the stage.
     */
    void init();

    /**
     * @brief Stop anything that is running and delete all of the properties here.
     *
     *  This is called when the plugin is deleted from the stage.
     */
    void unload();

    /**
     * @brief Start receiving and processing data.
     * 
     * Check if the plugin is ready to start, and return true if it is. Returning false will
     * prevent MNE from starting other plugins as well, so only return false if there is a
     * non-recoverable error.
     *
     * `start()` is triggered when the user clicks the play button in the MNE GUI. The `run()`
     * function will automatically be called just after `start()` completes if no other plugins
     * returned false.
     */
    bool start();

    /**
     * @brief Force the run loop to exit and end this thread execution
     * 
     * Force the run loop to exit and end this thread execution. This is called when the user
     * presses the stop button in the GUI. Return true once the thread has been stopped,
     * or false to indicate that there is a non-recoverable error preventing the thread from
     * being stopped.
     */
    bool stop();

    /**
     * @brief Returns the type of the plugin. For an MEG, the type is ISensor.
     */
    PluginType getType() const;

    /**
     * This function determines display name of the plugin in the
     * MNE GUI.
     *
     * @brief Returns the name of the plugin as a `QString`.
     */
    QString getName() const;

    /**
     * @brief Creates the `{{setup_widget_name}}`
     * 
     * Initialize and setup the screen shown when this plugin is selected.
     * The widget is created and destroyed every time that the run button
     * is pressed, so some loading the state from memory is necessary.
     *
     * The plugin scene manager is responsible for the deletion the newly
     * created pointer. It does not need to be deleted manually within this class.
     */
    QWidget* setupWidget();

    /**
     * @brief shows the `{{widget_name}}`
     * 
     * This is called when the toolbar action is called and will display the `{{widget_name}}`.
     */
    void showWidget();

    /**
     * Updates the plugin with new (incoming) data.
     *
     * @param pMeasurement    The incoming data in form of a generalized Measurement.
     */
    void update(SCMEASLIB::Measurement::SPtr pMeasurement);

signals:
    /**
    * @brief Emitted when fiffInfo becomes available
    */
    void fiffInfoAvailable();

private:

    /**
     * `m_isRunning` is set to true in `start()` and, and once `run()` has been called,
     * the thread will continue to execute until `m_isRunning` is manually set back to false
     * inside the `stop()` method.
     *
     * @brief m_bIsRunning indicates if the thread is currently running
     */
    bool m_bIsRunning;

    /**
     * Contains the most recent set of channel and system parameters that have
     * been received.
     * 
     * @brief m_pFiffInfo the most recently received MEG device parameters
     */
    FIFFLIB::FiffInfo::SPtr m_pFiffInfo;

    /**
     * A pointer to the the primary widget. The MNE Scan `DisplayManger` is responsible for
     * destroying the widget when it is no longer needed.
     * 
     * @brief pointer to the primary widget.
     * @note pointer may be a null pointer if the widget is not being shown.
     */
    QSharedPointer<{{widget_name}}> m_pWidget;

    /**
     * @brief a action that is shown in the toolbar and triggers the display of the settings widget.
     */
    QAction* m_pActionShowWidget;    

    /**
     * A thread safe circular buffer. The buffer has a fixed size, a read head, and write head. When the 
     * write head reaches the end of the buffer, it loops back and begins overwriting from the beginning.
     * If the read head doesn't keep up with the write head, and the write head loops back and gets to
     * where it would overlap the read head, writes are ignored until data in the buffer is consumed by
     * the read head. Likewise, if the read head catches up to the write head, reads return a matrix of
     * zeros instead of data in the buffer. Reads and writes are controlled with a semaphore lock that
     * can block execution indefinitely if not used carefully. For more information, see the following links.
     *
     * https://en.wikipedia.org/wiki/Circular_buffer
     * http://doc.qt.io/qt-5/qtcore-threads-semaphores-example.html
     *
     * The `pop` method on CircularMatrixBuffer blocks the thread until data becomes available. This effectively
     * controls the speed of the thread's `run` loop.
     *
     * @brief m_pBuffer thread safe circular buffer
     */
    IOBUFFER::CircularMatrixBuffer<double>::SPtr    m_pBuffer;

    /**
     * Data received from upstream plugins is converted converted into a `SCMEASLIB::NewRealTimeMultiSampleArray`
     * and then passed into our plugin here. It emits a `notify` signal each time that
     * new data becomes available. The `notify` event is connected to our `update` method.
     *
     * @brief m_poutput Output from the SHI MEG plugin
     * @note Delete this variable if creating a sensor plugin that does not be receiving input from other plugins
     */
    PluginInputData<SCMEASLIB::RealTimeMultiSampleArray>::SPtr m_pInput;

    /**
     * Data processed during the `run` loop is converted into a `SCMEASLIB::NewRealTimeMultiSampleArray`
     * and then passed on to subsequent plugins through this output variable. The plugin manager will
     * handle propagating the new values to downstream plugins.
     *
     * @brief m_poutput Output from the SHI MEG plugin
     */
    PluginOutputData<SCMEASLIB::RealTimeMultiSampleArray>::SPtr m_pOutput;
};

} // NAMESPACE

#endif // {{header_define}}
