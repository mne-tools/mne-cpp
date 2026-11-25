//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2014-2026 MNE-CPP Authors
 *
 * @file     channelselectionview.cpp
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Andreas Griesshammer <ag@fieldlineinc.com>
 * @since    0.1.0
 * @date     September 2014
 * @brief    Implementation of the ChannelSelectionView interactive channel-group picker.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "channelselectionview.h"
#include <mne/base/log.h>
#include <mne/base/string_utils.h>

#include "ui_channelselectionview.h"
#include "helpers/channelinfomodel.h"
#include "helpers/selectionitem.h"
#include "helpers/sensorlayoutwidget.h"
#include <mne/fiff/fiff_constants.h>
#include <mne/mne/mne_layout.h>
#include <mne/channels/selectionio.h>
#include <mne/channels/layoutmaker.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDate>
#include <QVector3D>
#include <QFileDialog>
#include <QListWidgetItem>
#include <QSettings>
#include <QApplication>
#include <QKeyEvent>
#include <QSignalBlocker>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <memory>
#include <filesystem>
#include <optional>
#include <utility>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::disp;
using namespace mne;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ChannelSelectionView::ChannelSelectionView(const QString& sSettingsPath,
                                           QWidget* parent,
                                           std::shared_ptr<ChannelInfoModel> pChannelInfoModel,
                                           Qt::WindowType f)
: AbstractView(parent, f)
, m_pUi(std::make_unique<Ui::ChannelSelectionViewWidget>())
, m_pChannelInfoModel(std::move(pChannelInfoModel))
, m_bSetup(false)
{
    m_sSettingsPath = sSettingsPath;
    m_pUi->setupUi(this);

    //Init gui elements
    initListWidgets();
    initSelectionSceneView();
    initComboBoxes();
    initButtons();
    initCheckBoxes();

    loadSettings();

    m_bSetup = true;

    setCurrentlyMappedFiffChannels(m_pChannelInfoModel->getMappedChannelsList());
}

//=============================================================================================================

ChannelSelectionView::~ChannelSelectionView()
{
    // The layout widget emits nothing while it is destroyed, so no connection needs cutting here
}

//=============================================================================================================

void ChannelSelectionView::initListWidgets()
{
    //Install event filter to receive key press events
    m_pUi->m_listWidget_userDefined->installEventFilter(this);
    m_pUi->m_listWidget_selectionGroups->installEventFilter(this);

    //Connect list widgets to update themselves and other list widgets when changed
    connect(m_pUi->m_listWidget_selectionGroups, &QListWidget::currentItemChanged,
            this, &ChannelSelectionView::updateSelectionGroupsList);

    //Update data view whenever a drag and drop item movement is performed
    //TODO: This is inefficient because updateDataView is called everytime the list's viewport is entered
    connect(m_pUi->m_listWidget_userDefined->model(), &QAbstractTableModel::dataChanged,
            this, &ChannelSelectionView::updateDataView);
}

//=============================================================================================================

void ChannelSelectionView::initSelectionSceneView()
{
    connect(m_pUi->m_sensorLayout, &SensorLayoutWidget::selectionChanged,
            this, &ChannelSelectionView::updateUserDefinedChannelsList);
}

//=============================================================================================================

void ChannelSelectionView::initComboBoxes()
{
    m_pUi->m_comboBox_layoutFile->clear();
    m_pUi->m_comboBox_layoutFile->insertItems(0, QStringList() << "babymeg-mag-inner-layer.lout"
                                                               << "babymeg-mag-outer-layer.lout"
                                                               //        << "babymeg-mag-ref.lout"
                                                               << "Vectorview-grad.lout"
                                                               << "Vectorview-all.lout"
                                                               << "Vectorview-mag.lout"
                                                               << "standard_waveguard64_duke.lout"
                                              //     << "CTF-275.lout"
                                              //     << "magnesWH3600.lout"
    );

    connect(m_pUi->m_comboBox_layoutFile, &QComboBox::currentTextChanged,
            this, &ChannelSelectionView::onComboBoxLayoutChanged);

    //Initialise layout as neuromag vectorview with all channels
    QString selectionName("Vectorview-all.lout");
    //loadLayout(QCoreApplication::applicationDirPath() + selectionName.prepend("/../resources/general/2DLayouts/"));
    setCurrentLayoutFile(selectionName);

    //Load selection groups again because they need to be reinitialised every time a new layout hase been loaded
    selectionName = QString("mne_browse_raw_vv.sel");
    loadSelectionGroups(QCoreApplication::applicationDirPath() + selectionName.prepend("/../resources/general/selectionGroups/"));
}

//=============================================================================================================

void ChannelSelectionView::initButtons()
{
    connect(m_pUi->m_pushButton_saveSelection, &QPushButton::clicked,
            this, &ChannelSelectionView::onBtnSaveUserSelection);

    connect(m_pUi->m_pushButton_loadSelection, &QPushButton::clicked,
            this, &ChannelSelectionView::onBtnLoadUserSelection);

    connect(m_pUi->m_pushButton_addToSelectionGroups, &QPushButton::clicked,
            this, &ChannelSelectionView::onBtnAddToSelectionGroups);
}

//=============================================================================================================

void ChannelSelectionView::initCheckBoxes()
{
    connect(m_pUi->m_checkBox_showBadChannelsAsRed, &QCheckBox::clicked,
            this, &ChannelSelectionView::updateBadChannels);
}

//=============================================================================================================

void ChannelSelectionView::setCurrentlyMappedFiffChannels(const std::vector<std::string>& mappedLayoutChNames)
{
    m_currentlyLoadedFiffChannels = mappedLayoutChNames;

    //Clear the visible channel list
    m_pUi->m_listWidget_visibleChannels->clear();

    //Keep the entry All in the selection list and m_selectionGroupsMap -> delete the rest
    m_pUi->m_listWidget_selectionGroups->clear();

    //Create group 'All' manually (because this group depends on the loaded channels from the fiff data file, not on the loaded selection file)
    auto it = m_selectionGroupsMap.find("All");
    if (it != m_selectionGroupsMap.end()) {
        m_selectionGroupsMap.erase(it);
    }
    m_selectionGroupsMap.emplace("All", m_currentlyLoadedFiffChannels);

    //Add selection groups to list widget
    for (const auto& [name, channels] : m_selectionGroupsMap) {
        m_pUi->m_listWidget_selectionGroups->insertItem(m_pUi->m_listWidget_selectionGroups->count(), QString::fromStdString(name));
    }

    //Set group all as slected item
    m_pUi->m_listWidget_selectionGroups->setCurrentItem(getItemForChName(m_pUi->m_listWidget_selectionGroups, "All"), QItemSelectionModel::Select);

    //Update selection
    updateSelectionGroupsList(getItemForChName(m_pUi->m_listWidget_selectionGroups, "All"), nullptr);
}

//=============================================================================================================

void ChannelSelectionView::highlightChannels(const QModelIndexList& channelIndexList)
{
    std::vector<std::string> channelList;
    for (int i = 0; i < channelIndexList.size(); i++) {
        QModelIndex nameIndex = m_pChannelInfoModel->index(channelIndexList.at(i).row(), 3);
        channelList.push_back(m_pChannelInfoModel->data(nameIndex, ChannelInfoModelRoles::GetMappedLayoutChName).toString().toStdString());
    }

    m_pUi->m_sensorLayout->highlightNames(channelList);
}

//=============================================================================================================

void ChannelSelectionView::selectChannels(const std::vector<std::string>& channelList)
{
    m_pUi->m_sensorLayout->selectNames(channelList);
}

//=============================================================================================================

std::vector<std::string> ChannelSelectionView::getSelectedChannels()
{
    //if no channels have been selected by the user -> show selected group channels
    QListWidget* targetListWidget;
    if (m_pUi->m_listWidget_userDefined->count() > 0)
        targetListWidget = m_pUi->m_listWidget_userDefined;
    else
        targetListWidget = m_pUi->m_listWidget_visibleChannels;

    //Create list of channels which are to be visible in the view
    std::vector<std::string> selectedChannels;

    for (int i = 0; i < targetListWidget->count(); i++) {
        QListWidgetItem* item = targetListWidget->item(i);
        selectedChannels.push_back(item->text().toStdString());
    }

    return selectedChannels;
}

//=============================================================================================================

QListWidgetItem* ChannelSelectionView::getItemForChName(QListWidget* listWidget,
                                                        const QString& channelName)
{
    for (int i = 0; i < listWidget->count(); i++)
        if (listWidget->item(i)->text() == channelName)
            return listWidget->item(i);

    return nullptr;
}

//=============================================================================================================

const std::map<std::string, QPointF>& ChannelSelectionView::getLayoutMap()
{
    return m_layoutMap;
}

//=============================================================================================================

void ChannelSelectionView::newFiffFileLoaded(std::shared_ptr<FiffInfo>& pFiffInfo)
{
    Q_UNUSED(pFiffInfo);

    loadLayout(resolveLayoutPath(m_pUi->m_comboBox_layoutFile->currentText()));
}

//=============================================================================================================

QString ChannelSelectionView::getCurrentLayoutFile()
{
    return m_pUi->m_comboBox_layoutFile->currentText();
}

//=============================================================================================================

QString ChannelSelectionView::getCurrentGroupFile()
{
    return m_pUi->m_lineEdit_loadedFile->text();
}

//=============================================================================================================

void ChannelSelectionView::setCurrentLayoutFile(const QString& currentLayoutFile)
{
    mne::Log::debug("setCurrentLayoutFile: {}", currentLayoutFile.toStdString());
    const QSignalBlocker blocker(m_pUi->m_comboBox_layoutFile);
    m_pUi->m_comboBox_layoutFile->setCurrentText(currentLayoutFile);

    loadLayout(resolveLayoutPath(currentLayoutFile));
    updateBadChannels();
}

//=============================================================================================================

void ChannelSelectionView::updateBadChannels()
{
    std::vector<std::string> badChannelMappedNames;
    const StringList badChannelList = m_pChannelInfoModel->getBadChannelList();
    if (m_pUi->m_checkBox_showBadChannelsAsRed->isChecked()) {
        for (int i = 0; i < m_pChannelInfoModel->rowCount(); i++) {
            QModelIndex digIndex = m_pChannelInfoModel->index(i, 3);
            QString mappedChName = m_pChannelInfoModel->data(digIndex, ChannelInfoModelRoles::GetMappedLayoutChName).toString();

            digIndex = m_pChannelInfoModel->index(i, 1);
            QString origChName = m_pChannelInfoModel->data(digIndex, ChannelInfoModelRoles::GetOrigChName).toString();

            if (std::ranges::contains(badChannelList, origChName.toStdString())) {
                badChannelMappedNames.push_back(mappedChName.toStdString());
            }
        }
    }

    m_pUi->m_sensorLayout->setLayout(m_layoutMap, badChannelMappedNames);

    updateSceneItems();
    updateDataView();
}

//=============================================================================================================

void ChannelSelectionView::updateDataView()
{
    //if no channels have been selected by the user -> show selected group channels
    QListWidget* targetListWidget;
    if (m_pUi->m_listWidget_userDefined->count() > 0)
        targetListWidget = m_pUi->m_listWidget_userDefined;
    else
        targetListWidget = m_pUi->m_listWidget_visibleChannels;

    //Create list of channels which are to be visible in the view
    std::vector<std::string> selectedChannels;

    for (int i = 0; i < targetListWidget->count(); i++) {
        const std::string name = targetListWidget->item(i)->text().toStdString();
        int indexTemp = m_pChannelInfoModel->getIndexFromMappedChName(name);

        if (indexTemp != -1) {
            QModelIndex mappedNameIndex = m_pChannelInfoModel->index(indexTemp, 1);
            selectedChannels.push_back(m_pChannelInfoModel->data(mappedNameIndex, ChannelInfoModelRoles::GetOrigChName).toString().toStdString());
        } else
            selectedChannels.push_back(name);
    }

    emit showSelectedChannelsOnly(selectedChannels);

    // The selected sensors, or without a selection the shown ones (EEG or MEG)
    emit selectionChanged(m_pUi->m_sensorLayout->selection());
}

//=============================================================================================================

void ChannelSelectionView::saveSettings()
{
    if (m_sSettingsPath.isEmpty()) {
        return;
    }

    QSettings settings("MNECPP");

    mne::Log::info("saveSettings: {}", getCurrentLayoutFile().toStdString());

    settings.setValue(m_sSettingsPath + QString("/ChannelSelectionView/selectedLayoutFile"), getCurrentLayoutFile());
    settings.setValue(m_sSettingsPath + QString("/ChannelSelectionView/channelSelectionViewPos"), this->pos());
    settings.setValue(m_sSettingsPath + QString("/ChannelSelectionView/selectedGroupFile"), getCurrentGroupFile());
}

//=============================================================================================================

void ChannelSelectionView::loadSettings()
{
    if (m_sSettingsPath.isEmpty()) {
        return;
    }

    QSettings settings("MNECPP");

    setCurrentLayoutFile(settings.value(m_sSettingsPath + QString("/ChannelSelectionView/selectedLayoutFile"), "Vectorview-all.lout").toString());
    loadSelectionGroups(QCoreApplication::applicationDirPath() + settings.value(m_sSettingsPath + QString("/ChannelSelectionView/selectedGroupFile"), "mne_browse_raw_vv.sel").toString().prepend("/../resources/general/selectionGroups/"));

    mne::Log::debug("loadSettings: {}", getCurrentLayoutFile().toStdString());

    QPoint pos = settings.value(m_sSettingsPath + QString("/ChannelSelectionView/channelSelectionViewPos"), QPoint(100, 100)).toPoint();

    QList<QScreen*> screensList = QGuiApplication::screens();
    if (screensList.isEmpty()) {
        move(QPoint(100, 100));
    } else {
        move(pos);
    }
}

//=============================================================================================================

void ChannelSelectionView::updateGuiMode(GuiMode mode)
{
    switch (mode) {
        case GuiMode::Clinical:
            break;
        default: // default is research mode
            break;
    }
}

//=============================================================================================================

void ChannelSelectionView::updateProcessingMode(ProcessingMode mode)
{
    switch (mode) {
        case ProcessingMode::Offline:
            break;
        default: // default is realtime mode
            break;
    }
}

//=============================================================================================================

bool ChannelSelectionView::loadLayout(const QString& path)
{
    mne::Log::debug("loadLayout: {}", path.toStdString());
    const std::optional<mne::MNELayout> layout = mne::MNELayout::read(path.toStdU16String());
    const bool state = layout.has_value();
    m_layoutMap.clear();
    if (state) {
        for (const auto& [name, position] : layout->channelPositions())
            m_layoutMap.insert_or_assign(name, QPointF(position.x(), position.y()));
    }

    //if no layout for EEG is specified generate from digitizer points
    std::vector<Eigen::Vector3d> positions;
    std::vector<std::string> names;

    for (int i = 0; i < m_pChannelInfoModel->rowCount(); i++) {
        QModelIndex digIndex = m_pChannelInfoModel->index(i, 1);
        QString chName = m_pChannelInfoModel->data(digIndex, ChannelInfoModelRoles::GetOrigChName).toString();

        digIndex = m_pChannelInfoModel->index(i, 8);
        QVector3D channelDig = m_pChannelInfoModel->data(digIndex, ChannelInfoModelRoles::GetChDigitizer).value<QVector3D>();

        digIndex = m_pChannelInfoModel->index(i, 4);
        int kind = m_pChannelInfoModel->data(digIndex, ChannelInfoModelRoles::GetChKind).toInt();

        if (kind == FIFFV_EEG_CH) {
            positions.emplace_back(channelDig.x(), channelDig.y(), channelDig.z());
            names.push_back(chName.toStdString());
        }
    }

    //Add new EEG points to Layout Map
    if (!positions.empty()) {
        const auto corners = mne::LayoutMaker::make_layout(positions, {.radius = 60.0, .box_width = 5.0, .box_height = 4.0, .mirror_y = true});
        if (corners) {
            for (std::size_t i = 0; i < corners->size(); ++i) {
                const std::string& name = names.at(i);
                if (!m_layoutMap.contains(name))
                    m_layoutMap[name] = QPointF((*corners)[i].x(), (*corners)[i].y());
            }
        }
    }

    m_pUi->m_sensorLayout->setLayout(m_layoutMap, {});
    updateSceneItems();
    m_pUi->m_sensorLayout->fitToContents();

    if (state)
        emit loadedLayoutMap(m_layoutMap);

    if (m_bSetup) {
        saveSettings();
    }

    return state;
}

//=============================================================================================================

QString ChannelSelectionView::resolveLayoutPath(const QString& layoutFile) const
{
    if (layoutFile.isEmpty()) {
        return QString();
    }

    if (mne::path_from_utf8(layoutFile.toStdString()).is_absolute()) {
        return layoutFile;
    }

    return QCoreApplication::applicationDirPath() + QStringLiteral("/../resources/general/2DLayouts/") + layoutFile;
}

//=============================================================================================================

bool ChannelSelectionView::loadSelectionGroups(const QString& path)
{
    //Clear the visible channel list
    m_pUi->m_listWidget_visibleChannels->clear();

    //Keep the entry All in the selection list and m_selectionGroupsMap -> delete the rest
    m_pUi->m_listWidget_selectionGroups->clear();

    //Read selection from file and store to map
    QString newPath = path; //QCoreApplication::applicationDirPath() + path.prepend("/../resources/general/selectionGroups/");

    m_selectionGroupsMap.clear();

    // An empty path, or one with neither recognised extension, reads nothing
    // and must report failure rather than an indeterminate value.
    std::optional<std::vector<ChannelSelection>> groups;
    if (!path.isEmpty()) {
        const std::filesystem::path file = mne::path_from_utf8(newPath.toStdString());
        if (path.contains(".sel"))
            groups = SelectionIO::read_sel(file);
        if (path.contains(".mon")) {
            if (auto group = SelectionIO::read_mon(file))
                groups = std::vector<ChannelSelection>{std::move(*group)};
        }
    }
    const bool state = groups.has_value();
    if (groups) {
        for (const ChannelSelection& group : *groups) {
            m_selectionGroupsMap.emplace(group.name, group.channels);
        }
    }

    //Create group 'All' and 'All EEG' manually (bcause this group depends on the loaded channels from the Info data file, not on the loaded selection file)
    auto it = m_selectionGroupsMap.find("All");
    if (it != m_selectionGroupsMap.end()) {
        m_selectionGroupsMap.erase(it);
    }
    m_selectionGroupsMap.emplace("All", m_currentlyLoadedFiffChannels);

    std::vector<std::string> names;
    for (int i = 0; i < m_pChannelInfoModel->rowCount(); i++) {
        QModelIndex digIndex = m_pChannelInfoModel->index(i, 1);
        QString chName = m_pChannelInfoModel->data(digIndex, ChannelInfoModelRoles::GetOrigChName).toString();

        digIndex = m_pChannelInfoModel->index(i, 4);
        int kind = m_pChannelInfoModel->data(digIndex, ChannelInfoModelRoles::GetChKind).toInt();

        if (kind == FIFFV_EEG_CH) //FIFFV_MEG_CH
            names.push_back(chName.toStdString());
    }

    //Add 'Add EEG' group to selection groups
    it = m_selectionGroupsMap.find("All EEG");
    if (it != m_selectionGroupsMap.end()) {
        m_selectionGroupsMap.erase(it);
    }
    m_selectionGroupsMap.emplace("All EEG", names);

    //Add selection groups to list widget

    for (const auto& [name, channels] : m_selectionGroupsMap) {
        m_pUi->m_listWidget_selectionGroups->insertItem(m_pUi->m_listWidget_selectionGroups->count(), QString::fromStdString(name));
    }

    //Update selection
    updateSelectionGroupsList(getItemForChName(m_pUi->m_listWidget_selectionGroups, "All"), nullptr);

    //Set group all as slected item
    m_pUi->m_listWidget_selectionGroups->setCurrentItem(getItemForChName(m_pUi->m_listWidget_selectionGroups, "All"), QItemSelectionModel::Select);

    m_pUi->m_lineEdit_loadedFile->setText(QString::fromStdString(mne::utf8_from_path(mne::path_from_utf8(path.toStdString()).filename())));

    if (m_bSetup) {
        saveSettings();
    }

    return state;
}

//=============================================================================================================

void ChannelSelectionView::cleanUpMEGChannels()
{
    //Iterate through all loaded selection groups
    for (auto& [name, channels] : m_selectionGroupsMap) {
        //Delete the MEG channels which are not in the currently loaded layout file
        std::erase_if(channels, [this](const std::string& channel) { return !m_layoutMap.contains(channel) && channel.contains("MEG"); });
    }
}

//=============================================================================================================

void ChannelSelectionView::updateSelectionGroupsList(QListWidgetItem* current, QListWidgetItem* previous)
{
    Q_UNUSED(previous);

    if (current == 0) {
        return;
    }
    m_pUi->m_sensorLayout->setChannelKind(current->text().contains("EEG") ? FIFFV_EEG_CH : FIFFV_MEG_CH);

    //update visible channel list widget
    m_pUi->m_listWidget_visibleChannels->clear();
    if (auto items = m_selectionGroupsMap.find(current->text().toStdString()); items != m_selectionGroupsMap.end()) {
        for (const std::string& channel : items->second)
            m_pUi->m_listWidget_visibleChannels->addItem(QString::fromStdString(channel));
    }

    //update scene items based o nthe new selection group
    updateSceneItems();

    //update the channels plotted in the data view
    updateDataView();

    updateBadChannels();
}

//=============================================================================================================

void ChannelSelectionView::updateSceneItems()
{
    std::vector<std::string> visibleItems;
    visibleItems.reserve(static_cast<std::size_t>(m_pUi->m_listWidget_visibleChannels->count()));

    for (int i = 0; i < m_pUi->m_listWidget_visibleChannels->count(); i++)
        visibleItems.push_back(m_pUi->m_listWidget_visibleChannels->item(i)->text().toStdString());

    m_pUi->m_sensorLayout->setSelectable(visibleItems);
}

//=============================================================================================================

void ChannelSelectionView::updateUserDefinedChannelsList()
{
    QStringList userDefinedChannels;
    for (const std::string& name : m_pUi->m_sensorLayout->selectedNames()) {
        userDefinedChannels << QString::fromStdString(name);
    }

    m_pUi->m_listWidget_userDefined->clear();
    m_pUi->m_listWidget_userDefined->addItems(userDefinedChannels);

    updateDataView();
}

//=============================================================================================================

void ChannelSelectionView::onBtnLoadUserSelection()
{
    QString path = QFileDialog::getOpenFileName(this,
                                                QString("Open selection file"),
                                                QString("../resources/general/selectionGroups/"),
                                                tr("Selection files (*.sel *.mon)"));

    if (path.isEmpty())
        return;

    loadSelectionGroups(path);
    m_pUi->m_lineEdit_loadedFile->setText(QString::fromStdString(mne::utf8_from_path(mne::path_from_utf8(path.toStdString()).filename())));
}

//=============================================================================================================

void ChannelSelectionView::onBtnSaveUserSelection()
{
    QDate date;
    QString path = QFileDialog::getSaveFileName(this,
                                                "Save user channel selection",
                                                QString("../resources/general/selectionGroups/%1_%2_%3_UserSelection").arg(date.currentDate().year()).arg(date.currentDate().month()).arg(date.currentDate().day()),
                                                tr("MNE selection file(*.sel);; Brainstorm montage file(*.mon)"));

    if (!path.isEmpty()) {
        std::vector<ChannelSelection> groups;
        for (const auto& [name, channels] : m_selectionGroupsMap) {
            if (name != "All" && name != "All EEG")
                groups.push_back(ChannelSelection{name, channels});
        }
        const std::filesystem::path file = mne::path_from_utf8(path.toStdString());
        if (path.contains(".sel"))
            SelectionIO::write_sel(file, groups);
        if (path.contains(".mon"))
            SelectionIO::write_mon(file.parent_path(), groups);
    }
}

//=============================================================================================================

void ChannelSelectionView::onBtnAddToSelectionGroups()
{
    std::vector<std::string> temp;
    temp.reserve(static_cast<std::size_t>(m_pUi->m_listWidget_userDefined->count()));
    for (int i = 0; i < m_pUi->m_listWidget_userDefined->count(); i++)
        temp.push_back(m_pUi->m_listWidget_userDefined->item(i)->text().toStdString());

    m_selectionGroupsMap.emplace(m_pUi->m_lineEdit_selectionGroupName->text().toStdString(), temp);
    m_pUi->m_listWidget_selectionGroups->insertItem(m_pUi->m_listWidget_selectionGroups->count(), m_pUi->m_lineEdit_selectionGroupName->text());
}

//=============================================================================================================

void ChannelSelectionView::onComboBoxLayoutChanged()
{
    QString selectionName(m_pUi->m_comboBox_layoutFile->currentText());
    loadLayout(resolveLayoutPath(selectionName));
    updateBadChannels();
}

//=============================================================================================================

void ChannelSelectionView::resizeEvent(QResizeEvent* event)
{
    Q_UNUSED(event);
}

//=============================================================================================================

bool ChannelSelectionView::eventFilter(QObject* obj, QEvent* event)
{
    //Setup delete key on user defined channel list
    if (obj == m_pUi->m_listWidget_userDefined && event->type() == QEvent::KeyRelease) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);

        if (keyEvent->key() == Qt::Key_Delete) {
            qDeleteAll(m_pUi->m_listWidget_userDefined->selectedItems());
            updateDataView();
            return true;
        } else
            return false;
    }

    if (obj == m_pUi->m_listWidget_selectionGroups && event->type() == QEvent::KeyRelease) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);

        if (keyEvent->key() == Qt::Key_Delete) {
            QList<QListWidgetItem*> tempSelectedList;

            for (int i = 0; i < m_pUi->m_listWidget_selectionGroups->selectedItems().size(); i++) {
                if (m_pUi->m_listWidget_selectionGroups->selectedItems().at(i)->text() != "All" &&
                    m_pUi->m_listWidget_selectionGroups->selectedItems().at(i)->text() != "All EEG") {
                    tempSelectedList.append(m_pUi->m_listWidget_selectionGroups->selectedItems().at(i));
                    m_selectionGroupsMap.erase(m_pUi->m_listWidget_selectionGroups->selectedItems().at(i)->text().toStdString());
                }
            }

            qDeleteAll(tempSelectedList);
            updateDataView();

            return true;
        } else
            return false;
    }

    // pass the event on to the parent class
    return QWidget::eventFilter(obj, event);
}

//=============================================================================================================

QWidget* ChannelSelectionView::getViewWidget()
{
    return m_pUi->viewWidget;
}

//=============================================================================================================

QWidget* ChannelSelectionView::getControlWidget()
{
    return m_pUi->controlWidget;
}

//=============================================================================================================

bool ChannelSelectionView::isSelectionEmpty()
{
    return ((m_pUi->m_listWidget_selectionGroups->currentItem()->text() == "All") && (m_pUi->m_listWidget_userDefined->count() == 0));
}

//=============================================================================================================

void ChannelSelectionView::clearView()
{
    m_pUi->m_sensorLayout->clear();
}
