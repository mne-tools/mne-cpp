#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <settingscontrollergui.h>

#include <QMessageBox>
#include <QCloseEvent>

using namespace MNEANONYMIZE;

MainWindow::MainWindow(MNEANONYMIZE::SettingsControllerGui *c)
: m_bDataModified(true)
, m_pUi(new Ui::MainWindow)
, m_pController(c)
{
    m_pUi->setupUi(this);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if(confirmClose())
    {
        event->accept();
    } else {
        event->ignore();
    }
}

bool MainWindow::confirmClose()
{
    if (!m_bDataModified)
    {
        return true;
    }
    const QMessageBox::StandardButton ret
            = QMessageBox::warning(this, tr("Application"),
                                   tr("The document has been modified.\n"
                                      "Do you want to save your changes?"),
                                   QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    switch (ret) {
    case QMessageBox::Save:
        return true; //save();
    case QMessageBox::Cancel:
        return false;
    case QMessageBox::Discard:
        return true;
    default:
        break;
    }
    return true;
}

void MainWindow::setController(MNEANONYMIZE::SettingsControllerGui *c)
{
    m_pController = c;
}

MNEANONYMIZE::SettingsControllerGui* MainWindow::getController() const
{
    return m_pController;
}


void MainWindow::setLineEditInFile(const QString &f)
{
    m_pUi->lineEditInFile->setText(f);
}



//void MainWindow::open()
//{
//    if(confirmOpen())
//    {
//        QString fileName = QFileDialog::getOpenFileName(this);
//        if(!fileName.isEmpty())
//        {
//            loadFile(filename);
//        }
//    }
//}

//void MainWindow::save()
//{
//    return saveFile(curFile);
//}

//void MainWindow::about()
//{
//    QMessageBox::about(this,"About Application",
//            "MNE Anonymize");
//}

//void MainWindow::doucmentWasModified()
//{
//    senWindowModified()
//}

//void MainWindow::createActions()
//{
//    QMenu *fileMenu = menuBar()->addMenu("&File");
//    QAction *openAct = new QAction("&Open",this);
//    openAct->setShortcut(QKeySequence::Open);
//    openAct->setStatusTip("Open a FIFF file");

//    //conect
//    fileMenu->addAction(openAct);

//    QAction * aboutAct = helpMenu->addAction("About MNE Anonymize"),qApp)
//    aboutAct->setStatusTip("Show information about MNE Anonymize");


//}
