//=============================================================================================================
/**
* @file     mainwindow.cpp
* @author   Florian Schlembach <florian.schlembach@tu-ilmenau.de>;
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
*           Jens Haueisen <jens.haueisen@tu-ilmenau.de>
* @version  1.0
* @date     January, 2014
*
* @section  LICENSE
*
* Copyright (C) 2014, Florian Schlembach, Christoph Dinh, Matti Hamalainen and Jens Haueisen. All rights reserved.
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
* @brief    mne_browse_raw_qt is the QT equivalent of the already existing C-version of mne_browse_raw. It is pursued
*           to reimplement the full feature set of mne_browse_raw and even extend these.
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mainwindow.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEBrowseRawQt;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MainWindow::MainWindow(QWidget *parent)
: QMainWindow(parent)
, m_qFileRaw("./MNE-sample-data/MEG/sample/sample_audvis_raw.fif")
//, m_qEventFile("./MNE-sample-data/MEG/sample/sample_audvis_raw-eve.fif")
, m_qSettings()
, m_rawSettings()
, ui(new Ui::MainWindowWidget)
{
    ui->setupUi(this);

    //The following functions and their point of calling should NOT be changed
    //Setup the windows first - this NEEDS to be done here because important pointers (window pointers) which are needed for further processing are generated in this function
    setupWindowWidgets();
    setupMainWindow();

    // Setup rest of the GUI
    connectMenus();
    setWindowStatus();

    //Set standard LogLevel
    setLogLevel(_LogLvMax);
}


//*************************************************************************************************************

MainWindow::~MainWindow()
{
}


//*************************************************************************************************************

void MainWindow::setupWindowWidgets()
{
    //Create data window
    m_pDataWindow = new DataWindow(this);

    //Create dockble event window - QTDesigner used - see /FormFiles
    m_pEventWindow = new EventWindow(this);
    addDockWidget(Qt::RightDockWidgetArea, m_pEventWindow);
    m_pEventWindow->hide();

    //Create filter window - QTDesigner used - see /FormFiles
    m_pFilterWindow = new FilterWindow(this);
    m_pFilterWindow->hide();

    //Create dockable information window - QTDesigner used - see /FormFiles
    m_pInformationWindow = new InformationWindow(this);
    addDockWidget(Qt::BottomDockWidgetArea, m_pInformationWindow);
    m_pInformationWindow->hide();

    //Create about window - QTDesigner used - see /FormFiles
    m_pAboutWindow = new AboutWindow(this);
    m_pAboutWindow->hide();

    //Create selection manager window - QTDesigner used - see /FormFiles
    m_pSelectionManagerWindow = new SelectionManagerWindow(this);
    addDockWidget(Qt::BottomDockWidgetArea, m_pSelectionManagerWindow);
    m_pSelectionManagerWindow->setFloating(true);
    m_pSelectionManagerWindow->hide();

    //Create selection manager window - QTDesigner used - see /FormFiles
    m_pAverageWindow = new AverageWindow(this);
    addDockWidget(Qt::BottomDockWidgetArea, m_pAverageWindow);
    m_pAverageWindow->hide();

    //Init windows
    m_pDataWindow->init();
    m_pEventWindow->init();
    m_pFilterWindow->init();
}


//*************************************************************************************************************

void MainWindow::connectMenus()
{
    //File
    connect(ui->m_openAction, SIGNAL(triggered()), this, SLOT(openFile()));
    connect(ui->m_writeAction, SIGNAL(triggered()), this, SLOT(writeFile()));
    connect(ui->m_loadEvents, SIGNAL(triggered()), this, SLOT(loadEvents()));
    connect(ui->m_saveEvents, SIGNAL(triggered()), this, SLOT(saveEvents()));
    connect(ui->m_quitAction, SIGNAL(triggered()), qApp, SLOT(quit()));

    //Adjust
    connect(ui->m_filterAction, SIGNAL(triggered()), this, SLOT(showFilterWindow()));

    //Windows
    connect(ui->m_eventAction, SIGNAL(triggered()), this, SLOT(showEventWindow()));
    connect(ui->m_informationAction, SIGNAL(triggered()), this, SLOT(showInformationWindow()));
    connect(ui->m_channelSelectionManagerAction, SIGNAL(triggered()), this, SLOT(showSelectionManagerWindow()));
    connect(ui->m_averageWindow_Action, SIGNAL(triggered()), this, SLOT(showAverageWindow()));

    //Help
    connect(ui->m_aboutAction, SIGNAL(triggered()), this, SLOT(showAboutWindow()));
}


//*************************************************************************************************************

void MainWindow::setupMainWindow()
{
    //set Window functions
    resize(m_qSettings.value("MainWindow/size").toSize()); //Resize to predefined default size
    move(m_qSettings.value("MainWindow/position").toPoint()); // Move this main window to position 50/50 on the screen

    //Set data window as central widget - This is needed because we are using QDockWidgets
    setCentralWidget(m_pDataWindow);

    //Set window icon for MNE browse Raw Qt
    setWindowIcon(QIcon(":/Resources/Images/application_icon_mne_browse_raw_qt.png"));
}


//*************************************************************************************************************

void MainWindow::setWindowStatus()
{
    //Set window title
    QString title;
    title = QString("%1").arg(CInfo::AppNameShort());
    setWindowTitle(title);

    //Set status bar
    if(m_pDataWindow->getDataModel()->m_bFileloaded) {
        int idx = m_qFileRaw.fileName().lastIndexOf("/");
        QString filename = m_qFileRaw.fileName().remove(0,idx+1);
        title = QString("Data file: %1  /  First sample: %2  /  Sample frequency: %3Hz").arg(filename).arg(m_pDataWindow->getDataModel()->firstSample()).arg(m_pDataWindow->getDataModel()->m_fiffInfo.sfreq);
    }
    else
        title = QString("No data file");

    if(m_pEventWindow->getEventModel()->m_bFileloaded) {
        int idx = m_qEventFile.fileName().lastIndexOf("/");
        QString filename = m_qEventFile.fileName().remove(0,idx+1);

        title.append(QString("  -  Event file: %1").arg(filename));
    }
    else
        title.append("  -  No event file");

    //Add permanent widget to status bar after deleting old one
    QObjectList cildrenList = statusBar()->children();

    for(int i = 0; i< cildrenList.size(); i++)
        statusBar()->removeWidget((QWidget*)cildrenList.at(i));

    QLabel* label = new QLabel(title);
    statusBar()->addWidget(label);
}


//*************************************************************************************************************
//Log
void MainWindow::writeToLog(const QString& logMsg, LogKind lgknd, LogLevel lglvl)
{
    if(lglvl<=m_eLogLevelCurrent) {
        if(lgknd == _LogKndError)
            m_pTextBrowser_Log->insertHtml("<font color=red><b>Error:</b> "+logMsg+"</font>");
        else if(lgknd == _LogKndWarning)
            m_pTextBrowser_Log->insertHtml("<font color=blue><b>Warning:</b> "+logMsg+"</font>");
        else
            m_pTextBrowser_Log->insertHtml(logMsg);
        m_pTextBrowser_Log->insertPlainText("\n"); // new line
        //scroll down to the latest entry
        QTextCursor c = m_pTextBrowser_Log->textCursor();
        c.movePosition(QTextCursor::End);
        m_pTextBrowser_Log->setTextCursor(c);

        m_pTextBrowser_Log->verticalScrollBar()->setValue(m_pTextBrowser_Log->verticalScrollBar()->maximum());
    }
}


//*************************************************************************************************************

void MainWindow::setLogLevel(LogLevel lvl)
{
    switch(lvl) {
    case _LogLvMin:
        writeToLog(tr("minimal log level set"), _LogKndMessage, _LogLvMin);
        break;
    case _LogLvNormal:
        writeToLog(tr("normal log level set"), _LogKndMessage, _LogLvMin);
        break;
    case _LogLvMax:
        writeToLog(tr("maximum log level set"), _LogKndMessage, _LogLvMin);
        break;
    }

    m_eLogLevelCurrent = lvl;
}


//*************************************************************************************************************
// SLOTS
void MainWindow::openFile()
{
    QString filename = QFileDialog::getOpenFileName(this,QString("Open fiff data file"),QString("./MNE-sample-data/MEG/sample/"),tr("fif data files (*.fif)"));

    if(filename.isEmpty())
    {
        qDebug("User aborted opening of fiff data file");
        return;
    }

    if(m_qFileRaw.isOpen())
        m_qFileRaw.close();
    m_qFileRaw.setFileName(filename);

    if(m_pDataWindow->getDataModel()->loadFiffData(m_qFileRaw))
        qDebug() << "Fiff data file" << filename << "loaded.";
    else
        qDebug("ERROR loading fiff data file %s",filename.toLatin1().data());

    //Clear event model
    m_pEventWindow->getEventModel()->clearModel();

    //set position of QScrollArea
    m_pDataWindow->getDataTableView()->horizontalScrollBar()->setValue(0);

    //Set fiffInfo in event model
    m_pEventWindow->getEventModel()->setFiffInfo(m_pDataWindow->getDataModel()->m_fiffInfo);
    m_pEventWindow->getEventModel()->setFirstLastSample(m_pDataWindow->getDataModel()->firstSample(),
                                                        m_pDataWindow->getDataModel()->lastSample());

    //resize columns to contents - needs to be done because the new data file can be shorter than the old one
    m_pDataWindow->getDataTableView()->resizeColumnsToContents();

    //Update status bar
    setWindowStatus();
}


//*************************************************************************************************************

void MainWindow::writeFile()
{
    QString filename = QFileDialog::getSaveFileName(this,QString("Write fiff data file"),QString("./MNE-sample-data/MEG/sample/"),tr("fif data files (*.fif)"));

    if(filename.isEmpty())
    {
        qDebug("User aborted saving to fiff data file");
        return;
    }

    QFile t_fileRaw(filename);

    if(!m_pDataWindow->getDataModel()->writeFiffData(t_fileRaw))
        qDebug() << "MainWindow: ERROR writing fiff data file" << t_fileRaw.fileName() << "!";
}


//*************************************************************************************************************

void MainWindow::loadEvents()
{
    QString filename = QFileDialog::getOpenFileName(this,QString("Open fiff event data file"),QString("./MNE-sample-data/MEG/sample/"),tr("fif event data files (*-eve.fif);;fif data files (*.fif)"));

    if(filename.isEmpty())
    {
        qDebug("User aborted loading fiff event file");
        return;
    }

    if(m_qEventFile.isOpen())
        m_qEventFile.close();

    m_qEventFile.setFileName(filename);

    if(m_pEventWindow->getEventModel()->loadEventData(m_qEventFile))
        qDebug() << "Fiff event data file" << filename << "loaded.";
    else
        qDebug("ERROR loading fiff event data file %s",filename.toLatin1().data());

    //Update status bar
    setWindowStatus();

    //Show event window
    showEventWindow();
}


//*************************************************************************************************************

void MainWindow::saveEvents()
{
    QString filename = QFileDialog::getSaveFileName(this,
                                                    QString("Save fiff event data file"),
                                                    QString("./MNE-sample-data/MEG/sample/"),
                                                    tr("fif event data files (*-eve.fif);;fif data files (*.fif)"));
    if(filename.isEmpty())
    {
        qDebug("User aborted saving to fiff event data file");
        return;
    }

    if(m_qEventFile.isOpen())
        m_qEventFile.close();
    m_qEventFile.setFileName(filename);

    if(m_pEventWindow->getEventModel()->saveEventData(m_qEventFile)) {
        qDebug() << "Fiff event data file" << filename << "saved.";
    }
    else
        qDebug("ERROR saving fiff event data file %s",filename.toLatin1().data());
}


//*************************************************************************************************************

void MainWindow::showAboutWindow()
{
    //Note: A widget that happens to be obscured by other windows on the screen is considered to be visible.
    if(!m_pAboutWindow->isVisible())
    {
        m_pAboutWindow->show();
        m_pAboutWindow->raise();
    }
    else // if visible raise the widget to be sure that it is not obscured by other windows
        m_pAboutWindow->raise();
}


//*************************************************************************************************************

void MainWindow::showFilterWindow()
{
    //Note: A widget that happens to be obscured by other windows on the screen is considered to be visible.
    if(!m_pFilterWindow->isVisible())
    {
        m_pFilterWindow->show();
        m_pFilterWindow->raise();
    }
    else // if visible raise the widget to be sure that it is not obscured by other windows
        m_pFilterWindow->raise();
}


//*************************************************************************************************************

void MainWindow::showEventWindow()
{
    //Note: A widget that happens to be obscured by other windows on the screen is considered to be visible.
    if(!m_pEventWindow->isVisible())
    {
        m_pEventWindow->show();
        m_pEventWindow->raise();
    }
    else // if visible raise the widget to be sure that it is not obscured by other windows
        m_pEventWindow->raise();
}


//*************************************************************************************************************

void MainWindow::showInformationWindow()
{
    //Note: A widget that happens to be obscured by other windows on the screen is considered to be visible.
    if(!m_pInformationWindow->isVisible())
    {
        m_pInformationWindow->show();
        m_pInformationWindow->raise();
    }
    else // if visible raise the widget to be sure that it is not obscured by other windows
        m_pInformationWindow->raise();
}


//*************************************************************************************************************

void MainWindow::showSelectionManagerWindow()
{
    //Note: A widget that happens to be obscured by other windows on the screen is considered to be visible.
    if(!m_pSelectionManagerWindow->isVisible())
    {
        m_pSelectionManagerWindow->show();
        m_pSelectionManagerWindow->raise();
    }
    else // if visible raise the widget to be sure that it is not obscured by other windows
        m_pSelectionManagerWindow->raise();
}


//*************************************************************************************************************

void MainWindow::showAverageWindow()
{
    //Note: A widget that happens to be obscured by other windows on the screen is considered to be visible.
    if(!m_pAverageWindow->isVisible())
    {
        m_pAverageWindow->show();
        m_pAverageWindow->raise();
    }
    else // if visible raise the widget to be sure that it is not obscured by other windows
        m_pAverageWindow->raise();
}

