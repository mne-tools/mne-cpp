//=============================================================================================================
/**
 * @file     averagewindow.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  dev
 * @date     October, 2014
 *
 * @section  LICENSE
 *
 * Copyright (C) 2014, Christoph Dinh, Lorenz Esch. All rights reserved.
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
 * @brief    Definition of the AverageWindow class.
 *
 */

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "averagewindow.h"

#include <disp/viewers/helpers/averagesceneitem.h>

#include <cmath>

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDate>
#include <QRandomGenerator>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEBROWSE;

namespace
{

constexpr double kDefaultWhitenRegMag = 0.1;
constexpr double kDefaultWhitenRegGrad = 0.1;
constexpr double kDefaultWhitenRegEeg = 0.1;

bool matchesButterflySelection(const FIFFLIB::FiffChInfo& channelInfo,
                               const QString& selectionText)
{
    if(selectionText == QLatin1String("EEG")) {
        return channelInfo.kind == FIFFV_EEG_CH;
    }

    if(channelInfo.kind != FIFFV_MEG_CH) {
        return false;
    }

    if(selectionText == QLatin1String("MEG_mag")) {
        return channelInfo.unit == FIFF_UNIT_T;
    }

    return channelInfo.unit == FIFF_UNIT_T_M;
}

QStringList selectedButterflyChannels(const FIFFLIB::FiffInfo& info,
                                      const QString& selectionText)
{
    QStringList channelNames;

    for(const auto& channelInfo : info.chs) {
        if(info.bads.contains(channelInfo.ch_name)) {
            continue;
        }

        if(matchesButterflySelection(channelInfo, selectionText)) {
            channelNames << channelInfo.ch_name;
        }
    }

    return channelNames;
}

Eigen::MatrixXd createWhitener(const FIFFLIB::FiffCov& covariance)
{
    if(covariance.dim <= 0) {
        return Eigen::MatrixXd();
    }

    if(covariance.diag) {
        Eigen::MatrixXd whitener = Eigen::MatrixXd::Zero(covariance.dim, covariance.dim);
        for(int i = 0; i < covariance.dim; ++i) {
            const double variance = covariance.data(i, 0);
            if(variance > 1e-30) {
                whitener(i, i) = 1.0 / std::sqrt(variance);
            }
        }

        return whitener;
    }

    if(covariance.eig.size() == 0 || covariance.eigvec.size() == 0) {
        return Eigen::MatrixXd();
    }

    Eigen::MatrixXd whitener = Eigen::MatrixXd::Zero(covariance.dim, covariance.dim);
    for(int i = 0; i < covariance.eig.size(); ++i) {
        if(covariance.eig(i) > 1e-30) {
            whitener(i, i) = 1.0 / std::sqrt(covariance.eig(i));
        }
    }

    whitener *= covariance.eigvec;
    return whitener;
}

bool whitenButterflyEvoked(FIFFLIB::FiffEvoked& evoked,
                           const FIFFLIB::FiffCov& covariance)
{
    if(covariance.isEmpty() || evoked.isEmpty() || evoked.data.rows() == 0) {
        return false;
    }

    const FIFFLIB::FiffCov regularizedCov =
        covariance.regularize(evoked.info,
                              kDefaultWhitenRegMag,
                              kDefaultWhitenRegGrad,
                              kDefaultWhitenRegEeg,
                              true);

    const FIFFLIB::FiffCov preparedCov =
        regularizedCov.prepare_noise_cov(evoked.info, evoked.info.ch_names);

    if(preparedCov.isEmpty() || preparedCov.dim != evoked.data.rows()) {
        return false;
    }

    const Eigen::MatrixXd whitener = createWhitener(preparedCov);
    if(whitener.rows() != evoked.data.rows() || whitener.cols() != evoked.data.rows()) {
        return false;
    }

    evoked.data = whitener * evoked.data;
    return true;
}

FIFFLIB::FiffEvoked buildButterflyEvoked(const FIFFLIB::FiffEvoked& sourceEvoked,
                                         const QString& selectionText,
                                         const FIFFLIB::FiffCov& covariance,
                                         bool whiten,
                                         bool* whiteningApplied)
{
    const QStringList includeChannels = selectedButterflyChannels(sourceEvoked.info, selectionText);

    FIFFLIB::FiffEvoked displayEvoked =
        sourceEvoked.pick_channels(includeChannels, sourceEvoked.info.bads);

    bool didApplyWhitening = false;
    if(whiten) {
        didApplyWhitening = whitenButterflyEvoked(displayEvoked, covariance);
    }

    if(whiteningApplied) {
        *whiteningApplied = didApplyWhitening;
    }

    return displayEvoked;
}

} // namespace


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

AverageWindow::AverageWindow(QWidget *parent, QFile &file)
: QDockWidget(parent)
, ui(new Ui::AverageWindow)
{
    ui->setupUi(this);
    initMVC(file);
    init();
}


//*************************************************************************************************************

AverageWindow::AverageWindow(QWidget *parent)
: QDockWidget(parent)
, ui(new Ui::AverageWindow)
{
    ui->setupUi(this);
    initMVC();
    init();
}


//*************************************************************************************************************

//AverageWindow::AverageWindow()
//: AverageWindow(0)
//{
//}


//*************************************************************************************************************

AverageWindow::~AverageWindow()
{
}


//*************************************************************************************************************

AverageModel* AverageWindow::getAverageModel()
{
    return m_pAverageModel;
}


//*************************************************************************************************************

void AverageWindow::channelSelectionManagerChanged(const QList<QGraphicsItem*> &selectedChannelItems)
{
    //Repaint the average items in the average scene based on the input parameter
    m_pAverageScene->repaintItems(selectedChannelItems);

    //call the onSelection function manually to replot the data for the givven average items
    onSelectionChanged(ui->m_tableView_loadedSets->selectionModel()->selection(), QItemSelection());

    //fit everything in the view and update the scene
    ui->m_graphicsView_layout->fitInView(m_pAverageScene->sceneRect(), Qt::KeepAspectRatio);
    m_pAverageScene->update(m_pAverageScene->sceneRect());
}


//*************************************************************************************************************

void AverageWindow::scaleAveragedData(const QMap<QString,double> &scaleMap)
{
    //Set the scale map received from the scale window
    QMap<qint32,float> newScaleMapIdx;

    newScaleMapIdx[FIFF_UNIT_T_M] = scaleMap["MEG_grad"];
    newScaleMapIdx[FIFF_UNIT_T] = scaleMap["MEG_mag"];
    newScaleMapIdx[FIFFV_REF_MEG_CH] = scaleMap["MEG_mag"];
    newScaleMapIdx[FIFFV_EEG_CH] = scaleMap["MEG_EEG"];
    newScaleMapIdx[FIFFV_EOG_CH] = scaleMap["MEG_EOG"];
    newScaleMapIdx[FIFFV_EMG_CH] = scaleMap["MEG_EMG"];
    newScaleMapIdx[FIFFV_STIM_CH] = scaleMap["MEG_STIM"];
    newScaleMapIdx[FIFFV_MISC_CH] = scaleMap["MEG_MISC"];

    m_pAverageScene->setScaleMap(newScaleMapIdx);
    m_pButterflyScene->setScaleMap(scaleMap);
}


//*************************************************************************************************************

void AverageWindow::setMappedChannelNames(QStringList mappedChannelNames)
{
    m_mappedChannelNames = mappedChannelNames;
}


//*************************************************************************************************************

void AverageWindow::setNoiseCovariance(const FIFFLIB::FiffCov& covariance)
{
    m_noiseCovariance = covariance;
    refreshPlots();
}


//*************************************************************************************************************

void AverageWindow::clearNoiseCovariance()
{
    m_noiseCovariance.clear();
    m_bWhitenButterfly = false;
    refreshPlots();
}


//*************************************************************************************************************

void AverageWindow::setButterflyWhiteningEnabled(bool enabled)
{
    if(enabled && m_noiseCovariance.isEmpty()) {
        return;
    }

    m_bWhitenButterfly = enabled;
    refreshPlots();
}


//*************************************************************************************************************

bool AverageWindow::isButterflyWhiteningEnabled() const
{
    return m_bWhitenButterfly;
}


//*************************************************************************************************************

void AverageWindow::init()
{
    initAverageSceneView();
    initTableViewWidgets();
    initButtons();
    initComboBoxes();
}


//*************************************************************************************************************

void AverageWindow::initMVC(QFile &file)
{
    //Setup average model
    if(file.exists())
        m_pAverageModel = new AverageModel(file, this);
    else
        m_pAverageModel = new AverageModel(this);

    //Setup average delegate
    m_pAverageDelegate = new AverageDelegate(this);
}


//*************************************************************************************************************

void AverageWindow::initMVC()
{
    m_pAverageModel = new AverageModel(this);

    //Setup average delegate
    m_pAverageDelegate = new AverageDelegate(this);
}


//*************************************************************************************************************

void AverageWindow::initTableViewWidgets()
{
    //Set average model to list widget
    ui->m_tableView_loadedSets->setModel(m_pAverageModel);
    ui->m_tableView_loadedSets->setSelectionMode(QAbstractItemView::ExtendedSelection);
    ui->m_tableView_loadedSets->setColumnHidden(1,true); //hide second column because the average model holds the aspect kind for this column
    ui->m_tableView_loadedSets->setColumnHidden(4,true); //hide last column because the average model holds the data types for this column
    ui->m_tableView_loadedSets->resizeColumnsToContents();

    ui->m_tableView_loadedSets->adjustSize();

    //Connect selection of the loaded evoked files
    connect(ui->m_tableView_loadedSets->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &AverageWindow::onSelectionChanged);

    connect(m_pAverageModel, &AverageModel::fileLoaded,
            this, [this](bool loaded) {
        if(loaded) {
            selectLoadedSets();
        } else {
            m_pAverageScene->update();
            m_pButterflyScene->clear();
            m_pButterflyScene->update();
        }
    });

    selectLoadedSets();
}


//*************************************************************************************************************

void AverageWindow::initAverageSceneView()
{
    //Create average scene and set view
    m_pAverageScene = new AverageScene(ui->m_graphicsView_layout, this);
    ui->m_graphicsView_layout->setScene(m_pAverageScene);

    //Create butterfly average scene and set view
    m_pButterflyScene = new ButterflyScene(ui->m_graphicsView_butterflyPlot, this);
    ui->m_graphicsView_butterflyPlot->setScene(m_pButterflyScene);

    //Generate random colors for plotting
    for(int i = 0; i<500; i++)
        m_lButterflyColors.append(QColor(QRandomGenerator::global()->generate()%256, QRandomGenerator::global()->generate()%256, QRandomGenerator::global()->generate()%256));
}


//*************************************************************************************************************

void AverageWindow::initButtons()
{
    ui->m_toolbutton_addAverage->setText("+");
    ui->m_toolbutton_addAverage->setToolTip("Compute evoked responses from the current raw file");
    connect(ui->m_toolbutton_addAverage, &QToolButton::released,
            this, &AverageWindow::addAverageRequested);

    connect(ui->m_pushButton_exportLayoutPlot, &QPushButton::released,
            this, &AverageWindow::exportAverageLayoutPlot);

    connect(ui->m_pushButton_exportButterflyPlot, &QPushButton::released,
            this, &AverageWindow::exportAverageButterflyPlot);
}


//*************************************************************************************************************

void AverageWindow::initComboBoxes()
{
    connect(ui->m_comboBox_channelKind, &QComboBox::currentTextChanged,
            this, [this](){
        refreshPlots();
    });
}


//*************************************************************************************************************

void AverageWindow::onSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    Q_UNUSED(deselected);
    Q_UNUSED(selected);

    refreshPlots();
}


//*************************************************************************************************************

void AverageWindow::refreshPlots()
{
    QModelIndexList selectedRows;
    if(ui->m_tableView_loadedSets->selectionModel()) {
        selectedRows = ui->m_tableView_loadedSets->selectionModel()->selectedRows(0);
    }

    //Get current items from the average scene
    QList<QGraphicsItem *> currentAverageSceneItems = m_pAverageScene->items();

    //Set new data for all averageSceneItems
    for(int i = 0; i<currentAverageSceneItems.size(); i++) {
        AverageSceneItem* averageSceneItemTemp = static_cast<AverageSceneItem*>(currentAverageSceneItems.at(i));

        averageSceneItemTemp->m_lAverageData.clear();

        //Do for all selected evoked sets
        for(int u = 0; u < selectedRows.size(); ++u) {
            //Get only the necessary data from the average model (use column 4)
            QModelIndex index = selectedRows.at(u);

            const FiffInfo* fiffInfo = m_pAverageModel->data(m_pAverageModel->index(index.row(), 4), AverageModelRoles::GetFiffInfo).value<const FiffInfo*>();
            RowVectorPair averageData = m_pAverageModel->data(m_pAverageModel->index(index.row(), 4), AverageModelRoles::GetAverageData).value<RowVectorPair>();
            int first = m_pAverageModel->data(m_pAverageModel->index(index.row(), 2), AverageModelRoles::GetFirstSample).toInt();
            int last = m_pAverageModel->data(m_pAverageModel->index(index.row(), 3), AverageModelRoles::GetLastSample).toInt();

            //Get the averageScenItem specific data row
            int channelNumber = m_mappedChannelNames.indexOf(averageSceneItemTemp->m_sChannelName);

            if(channelNumber != -1) {
                averageSceneItemTemp->m_firstLastSample.first = first;
                averageSceneItemTemp->m_firstLastSample.second = last;
                averageSceneItemTemp->m_iChannelKind = fiffInfo->chs.at(channelNumber).kind;
                averageSceneItemTemp->m_iChannelUnit = fiffInfo->chs.at(channelNumber).unit;;
                averageSceneItemTemp->m_iChannelNumber = channelNumber;
                averageSceneItemTemp->m_iTotalNumberChannels = fiffInfo->ch_names.size();
                averageSceneItemTemp->m_lAverageData.append(QPair<QString, RowVectorPair>("0",averageData));
            }
        }
    }

    m_pAverageScene->update();

    //Draw butterfly plot
    m_pButterflyScene->clear();

    for(int i = 0; i < selectedRows.size(); ++i) {
        QModelIndex index = selectedRows.at(i);
        QString setName = m_pAverageModel->data(m_pAverageModel->index(index.row(), 0), Qt::DisplayRole).toString();
        const FIFFLIB::FiffEvoked* sourceEvoked = m_pAverageModel->getEvoked(index.row());

        if(!sourceEvoked) {
            continue;
        }

        fiff_int_t setUnit, setKind;
        if(ui->m_comboBox_channelKind->currentText() == "MEG_grad")
            setUnit = FIFF_UNIT_T_M;
        else
            setUnit = FIFF_UNIT_T;

        if(ui->m_comboBox_channelKind->currentText() == "EEG")
            setKind = FIFFV_EEG_CH;
        else
            setKind = FIFFV_MEG_CH;

        bool whiteningApplied = false;
        FIFFLIB::FiffEvoked displayEvoked = buildButterflyEvoked(*sourceEvoked,
                                                                 ui->m_comboBox_channelKind->currentText(),
                                                                 m_noiseCovariance,
                                                                 m_bWhitenButterfly,
                                                                 &whiteningApplied);

        if(displayEvoked.data.rows() == 0 || displayEvoked.data.cols() == 0) {
            continue;
        }

        if(whiteningApplied) {
            setName.append(" [whitened]");
        }

        ButterflySceneItem* butterflySceneItemTemp = new ButterflySceneItem(setName,
                                                                            setKind,
                                                                            setUnit,
                                                                            m_lButterflyColors);

        butterflySceneItemTemp->setEvokedData(displayEvoked);

        m_pButterflyScene->addItem(butterflySceneItemTemp);
    }

    m_pButterflyScene->update();
}


//*************************************************************************************************************

void AverageWindow::selectLoadedSets()
{
    if(!ui->m_tableView_loadedSets->selectionModel() || m_pAverageModel->rowCount() <= 0) {
        return;
    }

    const QModelIndex first = m_pAverageModel->index(0, 0, QModelIndex());
    const QModelIndex last = m_pAverageModel->index(m_pAverageModel->rowCount() - 1, 0, QModelIndex());
    const QItemSelection selection(first, last);

    ui->m_tableView_loadedSets->selectionModel()->clearSelection();
    ui->m_tableView_loadedSets->selectionModel()->select(selection,
                                                         QItemSelectionModel::Select | QItemSelectionModel::Rows);
    ui->m_tableView_loadedSets->selectionModel()->setCurrentIndex(first,
                                                                  QItemSelectionModel::Current | QItemSelectionModel::Rows);
}


//*************************************************************************************************************

void AverageWindow::exportAverageLayoutPlot()
{
    // Open file dialog
    QDate date;
    QString fileName = QFileDialog::getSaveFileName(this,
                                                    "Save average plot",
                                                    QString("%1/%2_%3_%4_AveragePlot").arg(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation)).arg(date.currentDate().year()).arg(date.currentDate().month()).arg(date.currentDate().day()),
                                                    tr("Vector graphic(*.svg);;Images (*.png)"));

    if(!fileName.isEmpty())
    {
        // Generate screenshot
        if(fileName.contains(".svg"))
        {
            QSvgGenerator svgGen;

            svgGen.setFileName(fileName);
            QRectF rect = m_pAverageScene->itemsBoundingRect();
            svgGen.setSize(QSize(rect.width(), rect.height()));
            //svgGen.setViewBox(QRect(0, 0, rect.width(), rect.height()));

            QPainter painter(&svgGen);
            m_pAverageScene->render(&painter);
        }

        if(fileName.contains(".png"))
        {
            m_pAverageScene->setSceneRect(m_pAverageScene->itemsBoundingRect());                  // Re-shrink the scene to it's bounding contents
            QImage image(m_pAverageScene->sceneRect().size().toSize(), QImage::Format_ARGB32);       // Create the image with the exact size of the shrunk scene
            image.fill(Qt::transparent);                                                                // Start all pixels transparent

            QPainter painter(&image);
            m_pAverageScene->render(&painter);
            image.save(fileName);
        }
    }
}


//*************************************************************************************************************

void AverageWindow::exportAverageButterflyPlot()
{
    // Open file dialog
    QDate date;
    QString fileName = QFileDialog::getSaveFileName(this,
                                                    "Save butterfly plot",
                                                    QString("%1/%2_%3_%4_ButterflyPlot").arg(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation)).arg(date.currentDate().year()).arg(date.currentDate().month()).arg(date.currentDate().day()),
                                                    tr("Vector graphic(*.svg);;Images (*.png)"));

    if(!fileName.isEmpty())
    {
        // Generate screenshot
        if(fileName.contains(".svg"))
        {
            QSvgGenerator svgGen;

            svgGen.setFileName(fileName);
            QRectF rect = m_pButterflyScene->itemsBoundingRect();
            svgGen.setSize(QSize(rect.width(), rect.height()));
            //svgGen.setViewBox(QRect(0, 0, rect.width(), rect.height()));

            QPainter painter(&svgGen);
            m_pButterflyScene->render(&painter);
        }

        if(fileName.contains(".png"))
        {
            m_pButterflyScene->setSceneRect(m_pButterflyScene->itemsBoundingRect());                  // Re-shrink the scene to it's bounding contents
            QImage image(m_pButterflyScene->sceneRect().size().toSize(), QImage::Format_ARGB32);       // Create the image with the exact size of the shrunk scene
            image.fill(Qt::transparent);                                                                // Start all pixels transparent

            QPainter painter(&image);
            m_pButterflyScene->render(&painter);
            image.save(fileName);
        }
    }
}


//*************************************************************************************************************

void AverageWindow::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event);
}
