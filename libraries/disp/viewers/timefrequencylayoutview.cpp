//=============================================================================================================
/**
 * @file     timefrequencylaoutview.cpp
 * @author   Gabriel Motta <gbmotta@mgh.harvard.edu>
 * @since    0.1.9
 * @date     April, 2021
 *
 * @section  LICENSE
 *
 * Copyright (C) 2021, Gabriel Motta. All rights reserved.
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
 * @brief    Declaration of the TimeFrequencyLayoutView Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "timefrequencylayoutview.h"

#include "helpers/timefrequencyscene.h"
#include "helpers/timefrequencysceneitem.h"
#include "helpers/selectionsceneitem.h"
#include "helpers/timefrequencymodel.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QVBoxLayout>
#include <QDebug>
#if !defined(NO_QOPENGLWIDGET)
    #include <QOpenGLWidget>
#endif

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

TimeFrequencyLayoutView::TimeFrequencyLayoutView()
: TimeFrequencyLayoutView("", Q_NULLPTR)
{

}

//=============================================================================================================


TimeFrequencyLayoutView::TimeFrequencyLayoutView(const QString& sSettingsPath,
                                                 QWidget *parent,
                                                 Qt::WindowFlags f)
: AbstractView(parent, f)
{
    m_sSettingsPath = sSettingsPath;
    this->setWindowTitle("Time-Frequency Layout");

    m_pTimeFreqGraphicsView = new QGraphicsView();

#if !defined(NO_QOPENGLWIDGET)
    m_pTimeFreqGraphicsView->setViewport(new QOpenGLWidget);
#endif

    m_pTimeFreqGraphicsView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_pTimeFreqGraphicsView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_pTimeFreqScene = new TimeFrequencyScene(m_pTimeFreqGraphicsView, this);
    m_pTimeFreqScene->setBackgroundBrush(QBrush(Qt::white));

    m_pTimeFreqGraphicsView->setScene(m_pTimeFreqScene);

    QVBoxLayout *neLayout = new QVBoxLayout(this);
    neLayout->setContentsMargins(0,0,0,0);
    neLayout->addWidget(m_pTimeFreqGraphicsView);
    this->setLayout(neLayout);
}

//=============================================================================================================

void TimeFrequencyLayoutView::updateGuiMode(GuiMode mode)
{
    switch(mode) {
        case GuiMode::Clinical:
            break;
        default: // default is research mode
            break;
    }
}

//=============================================================================================================

void TimeFrequencyLayoutView::updateProcessingMode(ProcessingMode mode)
{
    switch(mode) {
        case ProcessingMode::Offline:
            break;
        default: // default is realtime mode
            break;
    }
}

//=============================================================================================================

void TimeFrequencyLayoutView::saveSettings()
{

}

//=============================================================================================================

void TimeFrequencyLayoutView::loadSettings()
{

}


//=============================================================================================================

void TimeFrequencyLayoutView::clearView()
{

}

//=============================================================================================================

void TimeFrequencyLayoutView::channelSelectionChanged(const QVariant &data)
{
    if(!m_pTimeFreqScene) {
        qDebug() << "AverageLayoutView::channelSelectionManagerChanged - m_pAverageScene is NULL. Returning. ";
        return;
    }

//    if (m_listWidgets.empty()){

//    }

    SelectionItem* pSelectionItem = data.value<DISPLIB::SelectionItem*>();

    //Repaint the average items in the average scene based on the input parameter selectedChannelItems and update them with current data
    m_pTimeFreqScene->repaintSelectionItems(*pSelectionItem);
//    updateData();
}
//=============================================================================================================

void TimeFrequencyLayoutView::updateData()
{
    if(m_pTFModel){
        auto sceneItems = m_pTimeFreqScene->getItems();

        for (auto item : sceneItems){
            item->setSampleRate(m_pTFModel->getSamplingFrequency());
//            item->setData(m_pTFModel->data(item->getChannelNumber(), 2).value<Eigen::MatrixXd>());
//            qDebug() << "ch num:" << item->getChannelNumber();
        }
        qDebug() << "vec size:" << sceneItems.size();
        sceneItems.front()->setData(m_pTFModel->data(sceneItems.front()->getChannelNumber(), 2).value<Eigen::MatrixXd>());

//        DISPLIB::TFplot* tfplot = new DISPLIB::TFplot(m_pTFModel->data(1,2).value<Eigen::MatrixXd>(), m_pTFModel->getSamplingFrequency(), 0, 100, DISPLIB::ColorMaps::Jet);
//        tfplot->show();
        m_pTimeFreqScene->updateScene();
    }
}

//=============================================================================================================

void TimeFrequencyLayoutView::setTimeFrequencyModel(QSharedPointer<DISPLIB::TimeFrequencyModel> pModel)
{
    if(pModel){
        m_pTFModel = pModel;
        connect(m_pTFModel.data(), &TimeFrequencyModel::dataChanged,
                this, &TimeFrequencyLayoutView::updateData, Qt::UniqueConnection);
    }
}
