//=============================================================================================================
/**
 * @file     virtualchannelwindow.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @version  dev
 * @date     March, 2026
 *
 * @brief    Definition of the VirtualChannelWindow class.
 */

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "virtualchannelwindow.h"

#include "mainwindow.h"

#include <QAction>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QEvent>
#include <QFormLayout>
#include <QHeaderView>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QTableView>
#include <QToolBar>
#include <QVBoxLayout>

#include <algorithm>

using namespace MNEBROWSE;

//=============================================================================================================

VirtualChannelWindow::VirtualChannelWindow(QWidget *parent)
: QDockWidget(parent)
, m_pMainWindow(static_cast<MainWindow*>(parent))
, m_pVirtualChannelModel(new VirtualChannelModel(this))
{
    setupUi();
}

//=============================================================================================================

VirtualChannelWindow::~VirtualChannelWindow() = default;

//=============================================================================================================

void VirtualChannelWindow::init()
{
    initTable();
    initToolBar();
}

//=============================================================================================================

VirtualChannelModel* VirtualChannelWindow::getVirtualChannelModel() const
{
    return m_pVirtualChannelModel;
}

//=============================================================================================================

QTableView* VirtualChannelWindow::getVirtualChannelTableView() const
{
    return m_pTableView;
}

//=============================================================================================================

void VirtualChannelWindow::setAvailableChannelNames(const QStringList& channelNames)
{
    m_availableChannelNames = channelNames;
}

//=============================================================================================================

bool VirtualChannelWindow::event(QEvent *event)
{
    if(event->type() == QEvent::KeyPress && m_pTableView && m_pTableView->hasFocus()) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        if(keyEvent->key() == Qt::Key_Delete) {
            removeSelectedVirtualChannels();
            return true;
        }
    }

    return QDockWidget::event(event);
}

//=============================================================================================================

void VirtualChannelWindow::setupUi()
{
    setWindowTitle(QStringLiteral("Virtual Channel Manager"));
    setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea | Qt::BottomDockWidgetArea);

    m_pContents = new QWidget(this);
    m_pLayout = new QVBoxLayout(m_pContents);
    m_pLayout->setContentsMargins(6, 6, 6, 6);
    m_pLayout->setSpacing(6);

    m_pHintLabel = new QLabel(QStringLiteral("Create browser-level bipolar channels that are rendered live in the raw view."), m_pContents);
    m_pHintLabel->setWordWrap(true);
    m_pLayout->addWidget(m_pHintLabel);

    m_pTableView = new QTableView(m_pContents);
    m_pLayout->addWidget(m_pTableView, 1);

    setWidget(m_pContents);
}

//=============================================================================================================

void VirtualChannelWindow::initTable()
{
    m_pTableView->setModel(m_pVirtualChannelModel);
    m_pTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_pTableView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_pTableView->setEditTriggers(QAbstractItemView::DoubleClicked
                                  | QAbstractItemView::EditKeyPressed
                                  | QAbstractItemView::AnyKeyPressed);
    m_pTableView->horizontalHeader()->setStretchLastSection(true);
    m_pTableView->verticalHeader()->setVisible(false);
    m_pTableView->resizeColumnsToContents();

    connect(m_pVirtualChannelModel, &VirtualChannelModel::virtualChannelsChanged, this, [this]() {
        if(m_pTableView) {
            m_pTableView->resizeColumnsToContents();
        }
    });
}

//=============================================================================================================

void VirtualChannelWindow::initToolBar()
{
    m_pToolBar = new QToolBar(this);
    m_pToolBar->setMovable(false);

    QAction* addAction = new QAction(tr("Add bipolar channel"), this);
    connect(addAction, &QAction::triggered,
            this, &VirtualChannelWindow::addVirtualChannel);
    m_pToolBar->addAction(addAction);

    QAction* removeAction = new QAction(tr("Remove virtual channel"), this);
    connect(removeAction, &QAction::triggered,
            this, &VirtualChannelWindow::removeSelectedVirtualChannels);
    m_pToolBar->addAction(removeAction);

    m_pLayout->insertWidget(1, m_pToolBar);
}

//=============================================================================================================

bool VirtualChannelWindow::promptForVirtualChannel(QString& name,
                                                   QString& positiveChannel,
                                                   QString& negativeChannel) const
{
    QDialog dialog(const_cast<VirtualChannelWindow*>(this));
    dialog.setWindowTitle(QStringLiteral("Add Virtual Bipolar Channel"));

    QVBoxLayout* mainLayout = new QVBoxLayout(&dialog);
    QFormLayout* formLayout = new QFormLayout();

    QLineEdit* nameEdit = new QLineEdit(&dialog);
    QComboBox* positiveComboBox = new QComboBox(&dialog);
    QComboBox* negativeComboBox = new QComboBox(&dialog);

    positiveComboBox->addItems(m_availableChannelNames);
    negativeComboBox->addItems(m_availableChannelNames);

    if(m_availableChannelNames.size() > 1) {
        negativeComboBox->setCurrentIndex(1);
    }

    const auto updateName = [nameEdit, positiveComboBox, negativeComboBox]() {
        if(nameEdit->isModified()) {
            return;
        }

        nameEdit->setText(QStringLiteral("%1-%2")
            .arg(positiveComboBox->currentText(), negativeComboBox->currentText()));
    };

    QObject::connect(positiveComboBox, &QComboBox::currentTextChanged, &dialog, updateName);
    QObject::connect(negativeComboBox, &QComboBox::currentTextChanged, &dialog, updateName);
    updateName();

    formLayout->addRow(QStringLiteral("Name"), nameEdit);
    formLayout->addRow(QStringLiteral("Positive"), positiveComboBox);
    formLayout->addRow(QStringLiteral("Negative"), negativeComboBox);
    mainLayout->addLayout(formLayout);

    QLabel* hint = new QLabel(QStringLiteral("The virtual channel will be computed as Positive - Negative."), &dialog);
    hint->setWordWrap(true);
    mainLayout->addWidget(hint);

    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                                       Qt::Horizontal,
                                                       &dialog);
    QObject::connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    mainLayout->addWidget(buttonBox);

    if(dialog.exec() != QDialog::Accepted) {
        return false;
    }

    name = nameEdit->text().trimmed();
    positiveChannel = positiveComboBox->currentText().trimmed();
    negativeChannel = negativeComboBox->currentText().trimmed();
    return true;
}

//=============================================================================================================

void VirtualChannelWindow::addVirtualChannel()
{
    if(m_availableChannelNames.size() < 2) {
        QMessageBox::warning(this,
                             QStringLiteral("Add Virtual Channel"),
                             QStringLiteral("Load a raw file with at least two channels before creating a virtual channel."));
        return;
    }

    QString name;
    QString positiveChannel;
    QString negativeChannel;
    if(!promptForVirtualChannel(name, positiveChannel, negativeChannel)) {
        return;
    }

    if(positiveChannel == negativeChannel) {
        QMessageBox::warning(this,
                             QStringLiteral("Add Virtual Channel"),
                             QStringLiteral("Choose two different source channels for a bipolar virtual channel."));
        return;
    }

    const int row = m_pVirtualChannelModel->addVirtualChannel(name, positiveChannel, negativeChannel);
    if(row >= 0) {
        m_pTableView->selectRow(row);
        m_pTableView->scrollTo(m_pVirtualChannelModel->index(row, 0));
    }
}

//=============================================================================================================

void VirtualChannelWindow::removeSelectedVirtualChannels()
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
        m_pVirtualChannelModel->removeRow(index.row());
    }
}
