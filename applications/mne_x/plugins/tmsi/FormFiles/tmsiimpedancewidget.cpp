#include "tmsiimpedancewidget.h"
#include "ui_tmsiimpedancewidget.h"

#include "../tmsi.h"

#include <QFileDialog>
#include <QString>

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace TMSIPlugin;

//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

TmsiImpedanceWidget::TmsiImpedanceWidget(TMSI* p_pTMSI, QWidget *parent)
: m_pTMSI(p_pTMSI)
, QWidget(parent)
, ui(new Ui::TmsiImpedanceWidget)
{
    ui->setupUi(this);

    ui->m_graphicsView_impedanceView->setScene(&m_scene);

    ui->m_graphicsView_impedanceView->show();

    // Connect buttons
    connect(ui->m_pushButton_stop, &QPushButton::released, this, &TmsiImpedanceWidget::stopImpedanceMeasurement);
    connect(ui->m_pushButton_start, &QPushButton::released, this, &TmsiImpedanceWidget::startImpedanceMeasurement);
    connect(ui->m_pushButton_takeScreenshot, &QPushButton::released, this, &TmsiImpedanceWidget::takeScreenshot);
    connect(ui->m_pushButton_loadLayout, &QPushButton::released, this, &TmsiImpedanceWidget::loadLayout);
}

//*************************************************************************************************************

TmsiImpedanceWidget::~TmsiImpedanceWidget()
{
    delete ui;
}

//*************************************************************************************************************

void TmsiImpedanceWidget::updateGraphicScene(MatrixXf &matValue)
{
    for(int i = 0; i<m_qmElectrodeIndex.size(); i++)
    {
        // Find item in scene
        QVector2D position = m_qmElectrodePositions[m_qmElectrodeIndex[i]];
        //QList<QGraphicsItem *> itemList = m_scene.itemAt(QPointF(position.x(), position.y()));

        cout<<position.x()<<" "<<position.y()<<endl;

        // Repaint item depending on the current impedance value
//        if(!itemList.isEmpty())
//        {
            QGraphicsEllipseItem *item = (QGraphicsEllipseItem *)m_scene.itemAt(QPointF(position.x(), position.y()), QTransform());
            item->setBrush(QBrush(Qt::cyan));
//        }
    }
}

//*************************************************************************************************************

void TmsiImpedanceWidget::initGraphicScene()
{
    //m_scene.addText("Hello, world!");

    // Load standard layout file
    AsAElc *asaObject = new AsAElc();
    QVector< QVector<double> > elcLocation3D;
    QVector< QVector<double> > elcLocation2D;
    QString unit;
    QStringList elcChannelNames;
    QString sElcFilePath = QString("./mne_x_plugins/resources/tmsi/loc_files/standard_waveguard128.elc");

    if(!asaObject->readElcFile(sElcFilePath, elcChannelNames, elcLocation3D, elcLocation2D, unit))
        qDebug() << "Error: Reading elc file.";

    // Transform to QMap
    for(int i = 0; i<elcLocation2D.size(); i++)
    {
        m_qmElectrodePositions.insert(elcChannelNames.at(i), QVector2D(elcLocation2D[i][0], elcLocation2D[i][1]));
        m_qmElectrodeIndex.insert(i, elcChannelNames.at(i));
    }

    // Add electrodes to scene
    for(int i = 0; i<elcLocation2D.size(); i++)
    {
        QVector2D position(elcLocation2D[i][0],elcLocation2D[i][1]);
        addElectrodeItem("Test", position);
    }
}

//*************************************************************************************************************

void TmsiImpedanceWidget::addElectrodeItem(QString electrodeName, QVector2D position)
{
    m_scene.addEllipse(position.x(), position.y(), 10, 10, QPen(), QBrush(Qt::cyan));
}

//*************************************************************************************************************

void TmsiImpedanceWidget::startImpedanceMeasurement()
{
    m_pTMSI->m_bCheckImpedances = true;

    m_pTMSI->start();
}

//*************************************************************************************************************

void TmsiImpedanceWidget::stopImpedanceMeasurement()
{
    m_pTMSI->m_bCheckImpedances = false;

    m_pTMSI->stop();
}

//*************************************************************************************************************

void TmsiImpedanceWidget::takeScreenshot()
{

}

//*************************************************************************************************************

void TmsiImpedanceWidget::loadLayout()
{

}

