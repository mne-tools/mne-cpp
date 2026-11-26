//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     braintreemodel.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Population helpers (addSurface / addBemSurface / addSensors / addDipoles / addSourceSpace / addNetwork / addDigitizerData) for the mne_viz_qt scene model.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "braintreemodel.h"

#include "items/surfacetreeitem.h"
#include "items/bemtreeitem.h"
#include "items/sensortreeitem.h"
#include "items/dipoletreeitem.h"
#include "items/sourcespacetreeitem.h"
#include "items/digitizersettreeitem.h"
#include "items/networktreeitem.h"
#include <mne/base/log.h>
#include <mne/inv/dipole_fit/inv_ecd_set.h>
#include <mne/mne/mne_hemisphere.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <memory>
#include <vector>

namespace mne::viz::qt
{

BrainTreeModel::BrainTreeModel(QObject* parent)
: QStandardItemModel(parent)
{
    // Set headers
    setHorizontalHeaderLabels(QStringList() << "Data" << "Description");
}

SurfaceTreeItem* BrainTreeModel::addSurface(const QString& subject, const QString& hemi, const QString& surfType, const mne::FsSurface& surface)
{
    QStandardItem* subjectItem = getSubjectItem(subject);

    // Find or create Hemi item
    QStandardItem* hemiItem = nullptr;
    for (int i = 0; i < subjectItem->rowCount(); ++i) {
        if (subjectItem->child(i)->text() == hemi) {
            hemiItem = subjectItem->child(i);
            break;
        }
    }

    if (!hemiItem) {
        subjectItem->appendRow(new QStandardItem(hemi));
        hemiItem = subjectItem->child(subjectItem->rowCount() - 1);
    }

    // Create FsSurface Item
    auto surfItem = std::make_unique<SurfaceTreeItem>(surfType);
    surfItem->setSurfaceData(surface);

    SurfaceTreeItem* pSurfItem = surfItem.get();
    hemiItem->appendRow(surfItem.release());
    return pSurfItem;
}

bool BrainTreeModel::addAnnotation(const QString& subject, const QString& hemi, const mne::FsAnnotation& annotation)
{
    QStandardItem* subjectItem = getSubjectItem(subject);
    if (!subjectItem)
        return false;

    QStandardItem* hemiItem = nullptr;
    for (int i = 0; i < subjectItem->rowCount(); ++i) {
        if (subjectItem->child(i)->text() == hemi) {
            hemiItem = subjectItem->child(i);
            break;
        }
    }

    if (!hemiItem)
        return false;

    // Apply annotation to ALL surfaces in this hemi? Or just one?
    // Ex_brain_view logic applies atlas to the loaded surfaces of that hemi.
    // So we iterate children of hemiItem and if they are SurfaceTreeItems, set annotation.
    bool applied = false;
    for (int i = 0; i < hemiItem->rowCount(); ++i) {
        SurfaceTreeItem* item = dynamic_cast<SurfaceTreeItem*>(hemiItem->child(i));
        if (item) {
            item->setAnnotationData(annotation);
            applied = true;
        }
    }
    return applied;
}

QStandardItem* BrainTreeModel::getSubjectItem(const QString& subject)
{
    // Search top level
    QList<QStandardItem*> items = findItems(subject);
    if (!items.empty()) {
        return items.front();
    }

    // Create new
    appendRow(new QStandardItem(subject));
    return this->item(rowCount() - 1);
}

BemTreeItem* BrainTreeModel::addBemSurface(const QString& subject, const QString& bemName, const mne::MNEBemSurface& bemSurf)
{
    QStandardItem* subjItem = getSubjectItem(subject);

    // Check if BEM item already exists to avoid duplicates?
    // Usually BEM is added once.

    auto bemItem = std::make_unique<BemTreeItem>(bemName, bemSurf);
    bemItem->setVisible(true); // Default to visible

    // Set default colors based on type/name
    // Set colors based on ID (matching zeiss-intent)
    // 4=Head (Reddish), 3=OuterSkull (Greenish), 1=InnerSkull (Blueish)
    if (bemSurf.id == 4) {
        bemItem->setColor(QColor(128, 77, 77)); // Reddish
    } else if (bemSurf.id == 3) {
        bemItem->setColor(QColor(77, 128, 77)); // Greenish
    } else if (bemSurf.id == 1) {
        bemItem->setColor(QColor(77, 77, 128)); // Blueish
    } else {
        bemItem->setColor(QColor(100, 100, 100)); // Grey default
    }

    BemTreeItem* pBemItem = bemItem.get();
    subjItem->appendRow(bemItem.release());

    return pBemItem;
}

//=============================================================================================================

void BrainTreeModel::addSensors(const QString& type, std::vector<std::unique_ptr<QStandardItem>> items)
{
    auto parentItem = std::make_unique<QStandardItem>(type);
    parentItem->setCheckable(true);
    parentItem->setCheckState(Qt::Checked);

    for (auto& item : items) {
        parentItem->appendRow(item.release());
    }

    this->appendRow(parentItem.release());
}

//=============================================================================================================

void BrainTreeModel::addDipoles(const mne::InvEcdSet& set)
{
    auto item = std::make_unique<DipoleTreeItem>("Dipoles", set);
    item->setCheckable(true);
    item->setCheckState(Qt::Checked);
    this->appendRow(item.release());
}

//=============================================================================================================

void BrainTreeModel::addSourceSpace(const mne::MNESourceSpaces& srcSpace)
{
    auto parentItem = std::make_unique<QStandardItem>("Source Space");
    parentItem->setCheckable(true);
    parentItem->setCheckState(Qt::Checked);

    // Color: pinkish-red matching mne_viz_qt's source space rendering
    QColor srcColor(212, 28, 92);

    for (int h = 0; h < srcSpace.size(); ++h) {
        const auto& hemi = srcSpace[h];
        QString hemiLabel = (h == 0) ? "LH" : "RH";

        // Collect all source point positions for this hemisphere
        std::vector<QVector3D> positions;
        positions.reserve(hemi.vertno.size());
        for (int i = 0; i < hemi.vertno.size(); ++i) {
            int vIdx = hemi.vertno(i);
            if (vIdx < 0 || vIdx >= hemi.rr.rows())
                continue;
            positions.push_back(QVector3D(hemi.rr(vIdx, 0), hemi.rr(vIdx, 1), hemi.rr(vIdx, 2)));
        }

        // One item per hemisphere with all positions batched (0.00075 = 0.75mm radius, same as mne_viz_qt)
        parentItem->appendRow(new SourceSpaceTreeItem(hemiLabel, positions, srcColor, 0.00075f));
        mne::Log::debug("BrainTreeModel: Source space {} - points: {} coord_frame: {}", hemiLabel.toStdString(), positions.size(), hemi.coord_frame);
        if (!positions.empty()) {
            mne::Log::debug("First point: ({}, {}, {}) Last point: ({}, {}, {})", positions.front().x(), positions.front().y(), positions.front().z(), positions.back().x(), positions.back().y(), positions.back().z());
        }
    }

    this->appendRow(parentItem.release());
}

//=============================================================================================================

void BrainTreeModel::addDigitizerData(std::span<const mne::FiffDigPoint> digitizerPoints)
{
    if (digitizerPoints.empty())
        return;

    auto setItem = std::make_unique<DigitizerSetTreeItem>("Digitizer", digitizerPoints);
    const auto totalPoints = setItem->totalPointCount();
    const auto categories = setItem->rowCount();
    this->appendRow(setItem.release());

    mne::Log::info("BrainTreeModel: Added digitizer set with {} points in {} categories", totalPoints, categories);
}

//=============================================================================================================

NetworkTreeItem* BrainTreeModel::addNetwork(const mne::Network& network, const QString& name)
{
    QString displayName = name;
    if (displayName.isEmpty()) {
        displayName = QString::fromStdString(network.method());
        if (displayName.isEmpty())
            displayName = "Network";
    }

    const std::string objectKey = "net_" + displayName.toLower().replace(" ", "_").toStdString();

    auto item = std::make_unique<NetworkTreeItem>(displayName, objectKey);
    item->setCheckable(true);
    item->setCheckState(Qt::Checked);

    NetworkTreeItem* pItem = item.get();
    this->appendRow(item.release());

    return pItem;
}

} // namespace mne::viz::qt
