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
#include <QListWidget>
#include <QMessageBox>
#include <QTableView>
#include <QToolBar>
#include <QVBoxLayout>

#include <algorithm>

using namespace MNEBROWSE;

namespace
{

QString kindToDisplayString(VirtualChannelKind kind)
{
    return kind == VirtualChannelKind::AverageReference
        ? QStringLiteral("Average Reference")
        : QStringLiteral("Bipolar");
}

} // namespace

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

    m_pHintLabel = new QLabel(QStringLiteral("Create live browser-level derived channels, including classic bipolar pairs and average-reference derivations."), m_pContents);
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

    QAction* addAction = new QAction(tr("Add virtual channel"), this);
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

bool VirtualChannelWindow::promptForVirtualChannel(VirtualChannelDefinition& definition) const
{
    QDialog dialog(const_cast<VirtualChannelWindow*>(this));
    dialog.setWindowTitle(QStringLiteral("Add Virtual Channel"));

    QVBoxLayout* mainLayout = new QVBoxLayout(&dialog);
    QFormLayout* formLayout = new QFormLayout();

    QLineEdit* nameEdit = new QLineEdit(&dialog);
    QComboBox* typeComboBox = new QComboBox(&dialog);
    QComboBox* sourceComboBox = new QComboBox(&dialog);
    QListWidget* referenceListWidget = new QListWidget(&dialog);
    QLabel* hintLabel = new QLabel(&dialog);

    typeComboBox->addItem(kindToDisplayString(VirtualChannelKind::Bipolar),
                          static_cast<int>(VirtualChannelKind::Bipolar));
    typeComboBox->addItem(kindToDisplayString(VirtualChannelKind::AverageReference),
                          static_cast<int>(VirtualChannelKind::AverageReference));

    sourceComboBox->addItems(m_availableChannelNames);
    referenceListWidget->addItems(m_availableChannelNames);
    referenceListWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    hintLabel->setWordWrap(true);

    const auto updateReferenceItems = [typeComboBox, sourceComboBox, referenceListWidget]() {
        const QString sourceChannel = sourceComboBox->currentText().trimmed();
        const VirtualChannelKind kind = static_cast<VirtualChannelKind>(
            typeComboBox->currentData().toInt());

        referenceListWidget->setSelectionMode(kind == VirtualChannelKind::AverageReference
                                              ? QAbstractItemView::MultiSelection
                                              : QAbstractItemView::SingleSelection);

        bool firstReferenceSelected = false;
        for(int row = 0; row < referenceListWidget->count(); ++row) {
            QListWidgetItem* item = referenceListWidget->item(row);
            const bool isSourceItem = item->text().trimmed() == sourceChannel;

            Qt::ItemFlags flags = item->flags();
            flags.setFlag(Qt::ItemIsSelectable, !isSourceItem);
            item->setFlags(flags);
            item->setSelected(false);

            if(isSourceItem) {
                continue;
            }

            if(kind == VirtualChannelKind::AverageReference) {
                item->setSelected(true);
            } else if(!firstReferenceSelected) {
                item->setSelected(true);
                firstReferenceSelected = true;
            }
        }
    };

    const auto updateNameAndHint = [nameEdit, typeComboBox, sourceComboBox, referenceListWidget, hintLabel]() {
        const VirtualChannelKind kind = static_cast<VirtualChannelKind>(
            typeComboBox->currentData().toInt());

        QStringList selectedReferences;
        for(int row = 0; row < referenceListWidget->count(); ++row) {
            QListWidgetItem* item = referenceListWidget->item(row);
            if(item->isSelected()) {
                selectedReferences.append(item->text().trimmed());
            }
        }

        if(!nameEdit->isModified()) {
            if(kind == VirtualChannelKind::AverageReference) {
                nameEdit->setText(QStringLiteral("%1-avgref").arg(sourceComboBox->currentText()));
            } else {
                nameEdit->setText(QStringLiteral("%1-%2")
                    .arg(sourceComboBox->currentText(),
                         selectedReferences.isEmpty() ? QStringLiteral("?") : selectedReferences.first()));
            }
        }

        if(kind == VirtualChannelKind::AverageReference) {
            hintLabel->setText(QStringLiteral("The virtual channel will be computed as Source - average(Reference(s))."));
        } else {
            hintLabel->setText(QStringLiteral("The virtual channel will be computed as Source - Reference."));
        }
    };

    QObject::connect(typeComboBox,
                     QOverload<int>::of(&QComboBox::currentIndexChanged),
                     &dialog,
                     updateReferenceItems);
    QObject::connect(typeComboBox,
                     QOverload<int>::of(&QComboBox::currentIndexChanged),
                     &dialog,
                     updateNameAndHint);
    QObject::connect(sourceComboBox, &QComboBox::currentTextChanged, &dialog, updateReferenceItems);
    QObject::connect(sourceComboBox, &QComboBox::currentTextChanged, &dialog, updateNameAndHint);
    QObject::connect(referenceListWidget, &QListWidget::itemSelectionChanged, &dialog, updateNameAndHint);

    updateReferenceItems();
    updateNameAndHint();

    formLayout->addRow(QStringLiteral("Name"), nameEdit);
    formLayout->addRow(QStringLiteral("Type"), typeComboBox);
    formLayout->addRow(QStringLiteral("Source"), sourceComboBox);
    formLayout->addRow(QStringLiteral("Reference(s)"), referenceListWidget);
    mainLayout->addLayout(formLayout);
    mainLayout->addWidget(hintLabel);

    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                                       Qt::Horizontal,
                                                       &dialog);
    QObject::connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    mainLayout->addWidget(buttonBox);

    if(dialog.exec() != QDialog::Accepted) {
        return false;
    }

    definition.name = nameEdit->text().trimmed();
    definition.kind = static_cast<VirtualChannelKind>(typeComboBox->currentData().toInt());
    definition.primaryChannel = sourceComboBox->currentText().trimmed();
    definition.referenceChannels.clear();

    for(int row = 0; row < referenceListWidget->count(); ++row) {
        QListWidgetItem* item = referenceListWidget->item(row);
        if(item->isSelected()) {
            definition.referenceChannels.append(item->text().trimmed());
        }
    }

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

    VirtualChannelDefinition definition;
    if(!promptForVirtualChannel(definition)) {
        return;
    }

    definition.referenceChannels.removeAll(definition.primaryChannel);
    definition.referenceChannels.removeDuplicates();

    const bool invalidReferenceCount =
        (definition.kind == VirtualChannelKind::Bipolar && definition.referenceChannels.size() != 1)
        || (definition.kind == VirtualChannelKind::AverageReference && definition.referenceChannels.isEmpty());

    if(definition.primaryChannel.isEmpty() || invalidReferenceCount) {
        QMessageBox::warning(this,
                             QStringLiteral("Add Virtual Channel"),
                             definition.kind == VirtualChannelKind::AverageReference
                                 ? QStringLiteral("Choose one source channel and at least one reference channel.")
                                 : QStringLiteral("Choose one source channel and exactly one reference channel."));
        return;
    }

    const int row = m_pVirtualChannelModel->addVirtualChannel(definition.name,
                                                              definition.kind,
                                                              definition.primaryChannel,
                                                              definition.referenceChannels);
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
