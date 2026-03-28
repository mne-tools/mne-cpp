//=============================================================================================================
/**
 * @file     epochwindow.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @version  dev
 * @date     March, 2026
 *
 * @brief    Definition of the EpochWindow class.
 */

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "epochwindow.h"

#include "mainwindow.h"

#include <QAction>
#include <QCheckBox>
#include <QEvent>
#include <QHeaderView>
#include <QKeyEvent>
#include <QLabel>
#include <QSignalBlocker>
#include <QTableView>
#include <QToolBar>
#include <QVBoxLayout>
#include <QWidget>

using namespace MNEBROWSE;

//=============================================================================================================

EpochWindow::EpochWindow(QWidget *parent)
: QDockWidget(parent)
, m_pMainWindow(static_cast<MainWindow*>(parent))
, m_pEpochModel(new EpochModel(this))
{
    setupUi();
}

//=============================================================================================================

EpochWindow::~EpochWindow() = default;

//=============================================================================================================

void EpochWindow::init()
{
    initTable();
    initToolBar();
    updateSummary();
}

//=============================================================================================================

EpochModel* EpochWindow::getEpochModel() const
{
    return m_pEpochModel;
}

//=============================================================================================================

void EpochWindow::setRespectAutoRejects(bool enabled)
{
    if(!m_pAutoRejectCheckBox) {
        return;
    }

    QSignalBlocker blocker(m_pAutoRejectCheckBox);
    m_pAutoRejectCheckBox->setChecked(enabled);
    m_pEpochModel->setRespectAutoRejects(enabled);
}

//=============================================================================================================

void EpochWindow::refreshFromModel()
{
    if(m_pTableView) {
        m_pTableView->resizeColumnsToContents();
    }

    updateSummary();
}

//=============================================================================================================

bool EpochWindow::event(QEvent *event)
{
    if(event->type() == QEvent::KeyPress && m_pTableView && m_pTableView->hasFocus()) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        if(keyEvent->key() == Qt::Key_Space) {
            const QModelIndex current = m_pTableView->currentIndex();
            if(current.isValid()) {
                const QModelIndex checkboxIndex = m_pEpochModel->index(current.row(), 0);
                const bool checked =
                    m_pEpochModel->data(checkboxIndex, Qt::CheckStateRole).toInt() == Qt::Checked;
                m_pEpochModel->setData(checkboxIndex,
                                       checked ? Qt::Unchecked : Qt::Checked,
                                       Qt::CheckStateRole);
                return true;
            }
        }
    }

    return QDockWidget::event(event);
}

//=============================================================================================================

void EpochWindow::setupUi()
{
    setWindowTitle(QStringLiteral("Epoch Manager"));
    setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea | Qt::BottomDockWidgetArea);

    m_pContents = new QWidget(this);
    m_pLayout = new QVBoxLayout(m_pContents);
    m_pLayout->setContentsMargins(6, 6, 6, 6);
    m_pLayout->setSpacing(6);

    m_pHintLabel = new QLabel(QStringLiteral("Review automatically detected epochs, exclude individual trials, and the evoked view updates immediately."), m_pContents);
    m_pHintLabel->setWordWrap(true);
    m_pLayout->addWidget(m_pHintLabel);

    m_pAutoRejectCheckBox = new QCheckBox(QStringLiteral("Honor automatic artifact rejection"), m_pContents);
    m_pAutoRejectCheckBox->setChecked(true);
    m_pLayout->addWidget(m_pAutoRejectCheckBox);

    m_pSummaryLabel = new QLabel(m_pContents);
    m_pSummaryLabel->setWordWrap(true);
    m_pLayout->addWidget(m_pSummaryLabel);

    m_pTableView = new QTableView(m_pContents);
    m_pLayout->addWidget(m_pTableView, 1);

    setWidget(m_pContents);
}

//=============================================================================================================

void EpochWindow::initTable()
{
    m_pTableView->setModel(m_pEpochModel);
    m_pTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_pTableView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_pTableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_pTableView->horizontalHeader()->setStretchLastSection(true);
    m_pTableView->verticalHeader()->setVisible(false);
    m_pTableView->resizeColumnsToContents();

    connect(m_pTableView->selectionModel(), &QItemSelectionModel::currentRowChanged,
            this, &EpochWindow::jumpToEpoch);

    connect(m_pEpochModel, &EpochModel::epochsChanged, this, [this]() {
        if(m_pTableView) {
            m_pTableView->resizeColumnsToContents();
        }
        updateSummary();
    });

    connect(m_pAutoRejectCheckBox, &QCheckBox::toggled,
            m_pEpochModel, &EpochModel::setRespectAutoRejects);
}

//=============================================================================================================

void EpochWindow::initToolBar()
{
    m_pToolBar = new QToolBar(this);
    m_pToolBar->setMovable(false);

    QAction* resetManualAction = new QAction(tr("Reset manual exclusions"), this);
    connect(resetManualAction, &QAction::triggered,
            this, &EpochWindow::resetManualExclusions);
    m_pToolBar->addAction(resetManualAction);

    m_pLayout->insertWidget(3, m_pToolBar);
}

//=============================================================================================================

void EpochWindow::updateSummary()
{
    if(!m_pSummaryLabel) {
        return;
    }

    m_pSummaryLabel->setText(m_pEpochModel->summaryText());
}

//=============================================================================================================

void EpochWindow::resetManualExclusions()
{
    m_pEpochModel->resetManualExclusions();
}

//=============================================================================================================

void EpochWindow::jumpToEpoch(const QModelIndex &current, const QModelIndex &previous)
{
    Q_UNUSED(previous)

    if(!current.isValid() || !m_pMainWindow || !m_pMainWindow->dataWindow()
       || !m_pMainWindow->dataWindow()->getChannelDataView()) {
        return;
    }

    m_pMainWindow->dataWindow()->getChannelDataView()->scrollToSample(m_pEpochModel->sampleAt(current.row()), true);
}
