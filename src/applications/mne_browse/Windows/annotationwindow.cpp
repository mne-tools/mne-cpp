//=============================================================================================================
/**
 * @file     annotationwindow.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @version  2.1.0
 * @date     March, 2026
 *
 * @brief    Definition of the AnnotationWindow class.
 */

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "annotationwindow.h"

#include "mainwindow.h"

#include <QAction>
#include <QCheckBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QEvent>
#include <QHeaderView>
#include <QKeyEvent>
#include <QLabel>
#include <QTableView>
#include <QToolBar>
#include <QVBoxLayout>

#include <algorithm>

using namespace MNEBROWSE;

//=============================================================================================================

AnnotationWindow::AnnotationWindow(QWidget *parent)
: QDockWidget(parent)
, m_pMainWindow(static_cast<MainWindow*>(parent))
, m_pAnnotationModel(new AnnotationModel(this))
{
    setupUi();
}

//=============================================================================================================

AnnotationWindow::~AnnotationWindow() = default;

//=============================================================================================================

void AnnotationWindow::init()
{
    initTable();
    initToolBar();
}

//=============================================================================================================

AnnotationModel* AnnotationWindow::getAnnotationModel() const
{
    return m_pAnnotationModel;
}

//=============================================================================================================

QTableView* AnnotationWindow::getAnnotationTableView() const
{
    return m_pTableView;
}

//=============================================================================================================

void AnnotationWindow::addAnnotation(int startSample, int endSample, const QString& label)
{
    const int row = m_pAnnotationModel->addAnnotation(startSample, endSample, label);
    if(row >= 0 && m_pTableView) {
        m_pTableView->selectRow(row);
        m_pTableView->scrollTo(m_pAnnotationModel->index(row, 0));
    }
}

//=============================================================================================================

bool AnnotationWindow::isDescriptionVisible(const QString& description) const
{
    return !m_hiddenDescriptions.contains(description);
}

//=============================================================================================================

bool AnnotationWindow::event(QEvent *event)
{
    if(event->type() == QEvent::KeyPress && m_pTableView && m_pTableView->hasFocus()) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        if(keyEvent->key() == Qt::Key_Delete) {
            removeSelectedAnnotations();
            return true;
        }
    }

    return QDockWidget::event(event);
}

//=============================================================================================================

void AnnotationWindow::setupUi()
{
    setWindowTitle(QStringLiteral("Annotation Manager"));
    setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea | Qt::BottomDockWidgetArea);

    m_pContents = new QWidget(this);
    m_pLayout = new QVBoxLayout(m_pContents);
    m_pLayout->setContentsMargins(6, 6, 6, 6);
    m_pLayout->setSpacing(6);

    m_pHintLabel = new QLabel(QStringLiteral("Enable annotation mode and Shift-drag in the raw view to create time spans. The manager can edit MNE-style descriptions, channels, comments, and import/export JSON, CSV, TXT, or FIF annotation files."), m_pContents);
    m_pHintLabel->setWordWrap(true);
    m_pLayout->addWidget(m_pHintLabel);

    m_pTableView = new QTableView(m_pContents);
    m_pLayout->addWidget(m_pTableView, 1);

    setWidget(m_pContents);
}

//=============================================================================================================

void AnnotationWindow::initTable()
{
    m_pTableView->setModel(m_pAnnotationModel);
    m_pTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_pTableView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_pTableView->setEditTriggers(QAbstractItemView::DoubleClicked
                                  | QAbstractItemView::EditKeyPressed
                                  | QAbstractItemView::AnyKeyPressed);
    m_pTableView->horizontalHeader()->setStretchLastSection(true);
    m_pTableView->verticalHeader()->setVisible(false);
    m_pTableView->resizeColumnsToContents();

    connect(m_pTableView->selectionModel(), &QItemSelectionModel::currentRowChanged,
            this, &AnnotationWindow::jumpToAnnotation);

    connect(m_pAnnotationModel, &AnnotationModel::annotationsChanged, this, [this]() {
        if(m_pTableView) {
            m_pTableView->resizeColumnsToContents();
        }
    });
}

//=============================================================================================================

void AnnotationWindow::initToolBar()
{
    m_pToolBar = new QToolBar(this);
    m_pToolBar->setMovable(false);

    QAction* removeAnnotationAction = new QAction(tr("Remove annotation"), this);
    connect(removeAnnotationAction, &QAction::triggered,
            this, &AnnotationWindow::removeSelectedAnnotations);
    m_pToolBar->addAction(removeAnnotationAction);

    QAction* selectVisibleAction = new QAction(tr("Select visible"), this);
    selectVisibleAction->setToolTip(tr("Choose which annotation descriptions are visible on the raw data"));
    connect(selectVisibleAction, &QAction::triggered,
            this, &AnnotationWindow::selectVisibleDescriptions);
    m_pToolBar->addAction(selectVisibleAction);

    m_pLayout->insertWidget(1, m_pToolBar);
}

//=============================================================================================================

void AnnotationWindow::removeSelectedAnnotations()
{
    if(!m_pTableView || !m_pTableView->selectionModel()) {
        return;
    }

    QModelIndexList selectedRows = m_pTableView->selectionModel()->selectedRows();
    std::sort(selectedRows.begin(), selectedRows.end(),
              [](const QModelIndex& left, const QModelIndex& right) {
                  return left.row() > right.row();
              });

    for(const QModelIndex& index : selectedRows) {
        m_pAnnotationModel->removeRow(index.row());
    }
}

//=============================================================================================================

void AnnotationWindow::jumpToAnnotation(const QModelIndex &current, const QModelIndex &previous)
{
    Q_UNUSED(previous)

    if(!current.isValid() || !m_pMainWindow || !m_pMainWindow->dataWindow()
       || !m_pMainWindow->dataWindow()->getChannelDataView()) {
        return;
    }

    const QPair<int, int> range = m_pAnnotationModel->getSampleRange(current.row());
    m_pMainWindow->dataWindow()->getChannelDataView()->scrollToSample(range.first, true);
}

//=============================================================================================================

void AnnotationWindow::selectVisibleDescriptions()
{
    // Collect unique descriptions from model
    const QVector<AnnotationSpanData> spans = m_pAnnotationModel->getAnnotationSpans();
    QSet<QString> descriptions;
    for (const auto& span : spans)
        descriptions.insert(span.label);

    if (descriptions.isEmpty())
        return;

    QDialog dlg(this);
    dlg.setWindowTitle(tr("Select Visible Descriptions"));
    QVBoxLayout* layout = new QVBoxLayout(&dlg);

    QList<QCheckBox*> checkboxes;
    QStringList sorted = descriptions.values();
    sorted.sort();
    for (const QString& desc : sorted) {
        QCheckBox* cb = new QCheckBox(desc, &dlg);
        cb->setChecked(!m_hiddenDescriptions.contains(desc));
        layout->addWidget(cb);
        checkboxes.append(cb);
    }

    QDialogButtonBox* buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    layout->addWidget(buttons);

    if (dlg.exec() == QDialog::Accepted) {
        m_hiddenDescriptions.clear();
        for (int i = 0; i < checkboxes.size(); ++i) {
            if (!checkboxes[i]->isChecked())
                m_hiddenDescriptions.insert(sorted[i]);
        }
        emit visibilityFilterChanged();
    }
}
