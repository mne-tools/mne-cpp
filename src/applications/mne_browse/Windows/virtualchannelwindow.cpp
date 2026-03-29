//=============================================================================================================
/**
 * @file     virtualchannelwindow.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @version  2.1.0
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
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalBlocker>
#include <QTableView>
#include <QTableWidget>
#include <QToolBar>
#include <QVBoxLayout>

#include <algorithm>

using namespace MNEBROWSE;

namespace
{

QString kindToDisplayString(VirtualChannelKind kind)
{
    switch(kind) {
        case VirtualChannelKind::AverageReference:
            return QStringLiteral("Average Reference");
        case VirtualChannelKind::WeightedReference:
            return QStringLiteral("Weighted Reference");
        case VirtualChannelKind::Bipolar:
        default:
            return QStringLiteral("Bipolar");
    }
}

QString referenceSetDisplayText(const VirtualReferenceSetDefinition& definition)
{
    return QStringLiteral("%1 [%2]")
        .arg(definition.name, definition.channels.join(QStringLiteral(", ")));
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

    m_pHintLabel = new QLabel(QStringLiteral("Create live browser-level derived channels, including bipolar, average-reference, weighted-reference, and reusable reference-set workflows."), m_pContents);
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

    QAction* referenceSetsAction = new QAction(tr("Manage reference sets"), this);
    connect(referenceSetsAction, &QAction::triggered,
            this, &VirtualChannelWindow::manageReferenceSets);
    m_pToolBar->addAction(referenceSetsAction);

    QAction* removeAction = new QAction(tr("Remove virtual channel"), this);
    connect(removeAction, &QAction::triggered,
            this, &VirtualChannelWindow::removeSelectedVirtualChannels);
    m_pToolBar->addAction(removeAction);

    m_pLayout->insertWidget(1, m_pToolBar);
}

//=============================================================================================================

bool VirtualChannelWindow::promptForReferenceSet(VirtualReferenceSetDefinition& definition) const
{
    QDialog dialog(const_cast<VirtualChannelWindow*>(this));
    dialog.setWindowTitle(QStringLiteral("Add Reference Set"));

    QVBoxLayout* mainLayout = new QVBoxLayout(&dialog);
    QFormLayout* formLayout = new QFormLayout();

    QLineEdit* nameEdit = new QLineEdit(&dialog);
    QListWidget* channelListWidget = new QListWidget(&dialog);
    channelListWidget->setSelectionMode(QAbstractItemView::MultiSelection);
    channelListWidget->addItems(m_availableChannelNames);

    formLayout->addRow(QStringLiteral("Name"), nameEdit);
    formLayout->addRow(QStringLiteral("Channels"), channelListWidget);
    mainLayout->addLayout(formLayout);

    QLabel* hintLabel = new QLabel(QStringLiteral("Reference sets can be reused by average-reference and weighted-reference virtual channels."), &dialog);
    hintLabel->setWordWrap(true);
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
    definition.channels.clear();
    for(int row = 0; row < channelListWidget->count(); ++row) {
        QListWidgetItem* item = channelListWidget->item(row);
        if(item->isSelected()) {
            definition.channels.append(item->text().trimmed());
        }
    }

    return true;
}

//=============================================================================================================

bool VirtualChannelWindow::promptForVirtualChannel(VirtualChannelDefinition& definition) const
{
    QDialog dialog(const_cast<VirtualChannelWindow*>(this));
    dialog.setWindowTitle(QStringLiteral("Add Virtual Channel"));
    dialog.resize(620, 560);

    QVBoxLayout* mainLayout = new QVBoxLayout(&dialog);
    QFormLayout* formLayout = new QFormLayout();

    QLineEdit* nameEdit = new QLineEdit(&dialog);
    QComboBox* typeComboBox = new QComboBox(&dialog);
    QComboBox* sourceComboBox = new QComboBox(&dialog);
    QComboBox* referenceSourceComboBox = new QComboBox(&dialog);
    QListWidget* referenceListWidget = new QListWidget(&dialog);
    QTableWidget* weightTable = new QTableWidget(&dialog);
    QLabel* hintLabel = new QLabel(&dialog);

    typeComboBox->addItem(kindToDisplayString(VirtualChannelKind::Bipolar),
                          static_cast<int>(VirtualChannelKind::Bipolar));
    typeComboBox->addItem(kindToDisplayString(VirtualChannelKind::AverageReference),
                          static_cast<int>(VirtualChannelKind::AverageReference));
    typeComboBox->addItem(kindToDisplayString(VirtualChannelKind::WeightedReference),
                          static_cast<int>(VirtualChannelKind::WeightedReference));

    sourceComboBox->addItems(m_availableChannelNames);

    referenceSourceComboBox->addItem(QStringLiteral("Custom Selection"), QString());
    for(const QString& referenceSetName : m_pVirtualChannelModel->referenceSetNames()) {
        referenceSourceComboBox->addItem(QStringLiteral("Reference Set: %1").arg(referenceSetName), referenceSetName);
    }

    referenceListWidget->addItems(m_availableChannelNames);
    referenceListWidget->setSelectionMode(QAbstractItemView::MultiSelection);

    weightTable->setColumnCount(2);
    weightTable->setHorizontalHeaderLabels({QStringLiteral("Channel"), QStringLiteral("Weight")});
    weightTable->horizontalHeader()->setStretchLastSection(true);
    weightTable->verticalHeader()->setVisible(false);
    weightTable->setSelectionMode(QAbstractItemView::NoSelection);
    weightTable->setEditTriggers(QAbstractItemView::DoubleClicked
                                 | QAbstractItemView::EditKeyPressed
                                 | QAbstractItemView::AnyKeyPressed);

    hintLabel->setWordWrap(true);

    formLayout->addRow(QStringLiteral("Name"), nameEdit);
    formLayout->addRow(QStringLiteral("Type"), typeComboBox);
    formLayout->addRow(QStringLiteral("Source"), sourceComboBox);
    formLayout->addRow(QStringLiteral("References"), referenceSourceComboBox);
    formLayout->addRow(QStringLiteral("Channel List"), referenceListWidget);
    formLayout->addRow(QStringLiteral("Weights"), weightTable);
    mainLayout->addLayout(formLayout);
    mainLayout->addWidget(hintLabel);

    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                           Qt::Horizontal,
                                           &dialog);
    QObject::connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    mainLayout->addWidget(buttonBox);

    QMap<QString, double> weightByChannel;

    const auto currentReferenceSet = [this, referenceSourceComboBox]() {
        return m_pVirtualChannelModel->referenceSet(referenceSourceComboBox->currentData().toString());
    };

    const auto currentReferenceChannels = [currentReferenceSet, referenceSourceComboBox, referenceListWidget, sourceComboBox]() {
        QStringList referenceChannels;

        if(!referenceSourceComboBox->currentData().toString().isEmpty()) {
            referenceChannels = currentReferenceSet().channels;
        } else {
            for(int row = 0; row < referenceListWidget->count(); ++row) {
                QListWidgetItem* item = referenceListWidget->item(row);
                if(item->isSelected()) {
                    referenceChannels.append(item->text().trimmed());
                }
            }
        }

        referenceChannels.removeAll(sourceComboBox->currentText().trimmed());
        referenceChannels.removeDuplicates();
        return referenceChannels;
    };

    const auto rebuildWeightTable = [&]() {
        const QStringList referenceChannels = currentReferenceChannels();
        const VirtualChannelKind kind = static_cast<VirtualChannelKind>(typeComboBox->currentData().toInt());
        const bool weighted = kind == VirtualChannelKind::WeightedReference;

        weightTable->setVisible(weighted);
        if(!weighted) {
            return;
        }

        QSignalBlocker blocker(weightTable);
        weightTable->setRowCount(referenceChannels.size());
        for(int row = 0; row < referenceChannels.size(); ++row) {
            const QString channelName = referenceChannels.at(row);

            QTableWidgetItem* channelItem = new QTableWidgetItem(channelName);
            channelItem->setFlags(channelItem->flags() & ~Qt::ItemIsEditable);
            weightTable->setItem(row, 0, channelItem);

            const double weight = weightByChannel.contains(channelName) ? weightByChannel.value(channelName) : 1.0;
            QTableWidgetItem* weightItem = new QTableWidgetItem(QString::number(weight, 'g', 6));
            weightTable->setItem(row, 1, weightItem);
        }
        weightTable->resizeColumnsToContents();
    };

    const auto updateReferenceWidgets = [&]() {
        const QString sourceChannel = sourceComboBox->currentText().trimmed();
        const VirtualChannelKind kind = static_cast<VirtualChannelKind>(typeComboBox->currentData().toInt());
        const bool useReferenceSet = !referenceSourceComboBox->currentData().toString().isEmpty();

        referenceListWidget->setEnabled(!useReferenceSet);
        referenceListWidget->setSelectionMode(kind == VirtualChannelKind::Bipolar
                                              ? QAbstractItemView::SingleSelection
                                              : QAbstractItemView::MultiSelection);

        bool selectedOne = false;
        for(int row = 0; row < referenceListWidget->count(); ++row) {
            QListWidgetItem* item = referenceListWidget->item(row);
            const bool isSourceItem = item->text().trimmed() == sourceChannel;
            Qt::ItemFlags flags = item->flags();
            flags.setFlag(Qt::ItemIsSelectable, !isSourceItem);
            item->setFlags(flags);

            if(isSourceItem) {
                item->setSelected(false);
                continue;
            }

            if(useReferenceSet) {
                item->setSelected(false);
                continue;
            }

            if(kind == VirtualChannelKind::Bipolar && item->isSelected()) {
                if(!selectedOne) {
                    selectedOne = true;
                } else {
                    item->setSelected(false);
                }
            }
        }

        if(!useReferenceSet && kind == VirtualChannelKind::Bipolar && !selectedOne) {
            for(int row = 0; row < referenceListWidget->count(); ++row) {
                QListWidgetItem* item = referenceListWidget->item(row);
                if(item->flags().testFlag(Qt::ItemIsSelectable)) {
                    item->setSelected(true);
                    break;
                }
            }
        }

        rebuildWeightTable();
    };

    const auto updateNameAndHint = [&]() {
        const VirtualChannelKind kind = static_cast<VirtualChannelKind>(typeComboBox->currentData().toInt());
        const QString sourceChannel = sourceComboBox->currentText().trimmed();
        const QString referenceSetName = referenceSourceComboBox->currentData().toString();
        const QStringList referenceChannels = currentReferenceChannels();

        if(!nameEdit->isModified()) {
            switch(kind) {
                case VirtualChannelKind::AverageReference:
                    nameEdit->setText(referenceSetName.isEmpty()
                        ? QStringLiteral("%1-avgref").arg(sourceChannel)
                        : QStringLiteral("%1-avgref-%2").arg(sourceChannel, referenceSetName));
                    break;
                case VirtualChannelKind::WeightedReference:
                    nameEdit->setText(referenceSetName.isEmpty()
                        ? QStringLiteral("%1-wref").arg(sourceChannel)
                        : QStringLiteral("%1-wref-%2").arg(sourceChannel, referenceSetName));
                    break;
                case VirtualChannelKind::Bipolar:
                default:
                    nameEdit->setText(QStringLiteral("%1-%2")
                        .arg(sourceChannel,
                             referenceChannels.isEmpty() ? QStringLiteral("?") : referenceChannels.first()));
                    break;
            }
        }

        switch(kind) {
            case VirtualChannelKind::AverageReference:
                hintLabel->setText(referenceSetName.isEmpty()
                    ? QStringLiteral("The virtual channel will be computed as Source - average(Reference(s)).")
                    : QStringLiteral("The virtual channel will be computed as Source - average(Reference Set)."));
                break;
            case VirtualChannelKind::WeightedReference:
                hintLabel->setText(referenceSetName.isEmpty()
                    ? QStringLiteral("The virtual channel will be computed as Source - sum(weight_i * Reference_i).")
                    : QStringLiteral("The virtual channel will reuse the selected reference set and apply editable weights to each reference channel."));
                break;
            case VirtualChannelKind::Bipolar:
            default:
                hintLabel->setText(QStringLiteral("The virtual channel will be computed as Source - Reference."));
                break;
        }
    };

    QObject::connect(typeComboBox,
                     QOverload<int>::of(&QComboBox::currentIndexChanged),
                     &dialog,
                     updateReferenceWidgets);
    QObject::connect(typeComboBox,
                     QOverload<int>::of(&QComboBox::currentIndexChanged),
                     &dialog,
                     updateNameAndHint);
    QObject::connect(sourceComboBox, &QComboBox::currentTextChanged, &dialog, updateReferenceWidgets);
    QObject::connect(sourceComboBox, &QComboBox::currentTextChanged, &dialog, updateNameAndHint);
    QObject::connect(referenceSourceComboBox,
                     QOverload<int>::of(&QComboBox::currentIndexChanged),
                     &dialog,
                     updateReferenceWidgets);
    QObject::connect(referenceSourceComboBox,
                     QOverload<int>::of(&QComboBox::currentIndexChanged),
                     &dialog,
                     updateNameAndHint);
    QObject::connect(referenceListWidget, &QListWidget::itemSelectionChanged, &dialog, rebuildWeightTable);
    QObject::connect(referenceListWidget, &QListWidget::itemSelectionChanged, &dialog, updateNameAndHint);
    QObject::connect(weightTable, &QTableWidget::itemChanged, &dialog, [&weightByChannel](QTableWidgetItem* item) {
        if(!item || item->column() != 1) {
            return;
        }

        bool ok = false;
        const double weight = item->text().trimmed().toDouble(&ok);
        if(!ok) {
            item->setText(QStringLiteral("1"));
            return;
        }

        if(QTableWidgetItem* channelItem = item->tableWidget()->item(item->row(), 0)) {
            weightByChannel.insert(channelItem->text().trimmed(), weight);
        }
    });

    updateReferenceWidgets();
    updateNameAndHint();

    if(dialog.exec() != QDialog::Accepted) {
        return false;
    }

    definition.name = nameEdit->text().trimmed();
    definition.kind = static_cast<VirtualChannelKind>(typeComboBox->currentData().toInt());
    definition.primaryChannel = sourceComboBox->currentText().trimmed();
    definition.referenceSetName = referenceSourceComboBox->currentData().toString().trimmed();
    definition.referenceChannels = currentReferenceChannels();
    definition.referenceWeights.clear();

    if(definition.kind == VirtualChannelKind::WeightedReference) {
        definition.referenceWeights.reserve(definition.referenceChannels.size());
        for(int index = 0; index < definition.referenceChannels.size(); ++index) {
            definition.referenceWeights.append(weightByChannel.value(definition.referenceChannels.at(index), 1.0));
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
        || ((definition.kind == VirtualChannelKind::AverageReference
             || definition.kind == VirtualChannelKind::WeightedReference)
            && definition.referenceChannels.isEmpty());

    if(definition.primaryChannel.isEmpty() || invalidReferenceCount) {
        QMessageBox::warning(this,
                             QStringLiteral("Add Virtual Channel"),
                             definition.kind == VirtualChannelKind::Bipolar
                                 ? QStringLiteral("Choose one source channel and exactly one reference channel.")
                                 : QStringLiteral("Choose one source channel and at least one reference channel."));
        return;
    }

    const int row = m_pVirtualChannelModel->addVirtualChannel(definition.name,
                                                              definition.kind,
                                                              definition.primaryChannel,
                                                              definition.referenceChannels,
                                                              definition.referenceWeights,
                                                              definition.referenceSetName);
    if(row >= 0) {
        m_pTableView->selectRow(row);
        m_pTableView->scrollTo(m_pVirtualChannelModel->index(row, 0));
    }
}

//=============================================================================================================

void VirtualChannelWindow::manageReferenceSets()
{
    QDialog dialog(this);
    dialog.setWindowTitle(QStringLiteral("Manage Reference Sets"));
    dialog.resize(520, 420);

    QVBoxLayout* mainLayout = new QVBoxLayout(&dialog);
    QLabel* hintLabel = new QLabel(QStringLiteral("Reference sets are reusable channel groups for average-reference and weighted-reference virtual channels."), &dialog);
    hintLabel->setWordWrap(true);
    mainLayout->addWidget(hintLabel);

    QListWidget* setListWidget = new QListWidget(&dialog);
    mainLayout->addWidget(setListWidget, 1);

    auto reloadSets = [this, setListWidget]() {
        setListWidget->clear();
        for(const VirtualReferenceSetDefinition& definition : m_pVirtualChannelModel->referenceSets()) {
            QListWidgetItem* item = new QListWidgetItem(referenceSetDisplayText(definition), setListWidget);
            item->setData(Qt::UserRole, definition.name);
        }
    };

    reloadSets();

    QHBoxLayout* buttonRow = new QHBoxLayout();
    QPushButton* addButton = new QPushButton(QStringLiteral("Add"), &dialog);
    QPushButton* removeButton = new QPushButton(QStringLiteral("Remove"), &dialog);
    buttonRow->addWidget(addButton);
    buttonRow->addWidget(removeButton);
    buttonRow->addStretch(1);
    mainLayout->addLayout(buttonRow);

    QObject::connect(addButton, &QPushButton::clicked, &dialog, [this, &reloadSets]() {
        if(m_availableChannelNames.isEmpty()) {
            QMessageBox::warning(this,
                                 QStringLiteral("Reference Sets"),
                                 QStringLiteral("Load a raw file before creating reference sets."));
            return;
        }

        VirtualReferenceSetDefinition definition;
        if(!promptForReferenceSet(definition)) {
            return;
        }

        if(definition.name.isEmpty() || definition.channels.isEmpty()) {
            QMessageBox::warning(this,
                                 QStringLiteral("Reference Sets"),
                                 QStringLiteral("Provide a set name and choose at least one channel."));
            return;
        }

        m_pVirtualChannelModel->addReferenceSet(definition.name, definition.channels);
        reloadSets();
    });

    QObject::connect(removeButton, &QPushButton::clicked, &dialog, [this, setListWidget, &reloadSets]() {
        QListWidgetItem* item = setListWidget->currentItem();
        if(!item) {
            return;
        }

        m_pVirtualChannelModel->removeReferenceSet(item->data(Qt::UserRole).toString());
        reloadSets();
    });

    QDialogButtonBox* closeBox = new QDialogButtonBox(QDialogButtonBox::Close,
                                                      Qt::Horizontal,
                                                      &dialog);
    QObject::connect(closeBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    QObject::connect(closeBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    mainLayout->addWidget(closeBox);

    dialog.exec();
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
