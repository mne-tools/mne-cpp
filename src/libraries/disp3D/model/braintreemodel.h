//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file braintreemodel.h
 * @since 2026
 * @date  March 2026
 * @brief QStandardItemModel hierarchy that organises every 3-D scene object (surfaces, sensors, sources, networks) for a QTreeView.
 *
 * BrainTreeModel is the single source of truth for what is in the
 * scene: each renderable owns a @ref AbstractTreeItem-derived row
 * and exposes its visibility, transform, colour and alpha through
 * the standard Qt item-data roles. The tree is structured by
 * subject &rarr; modality &rarr; instance so a multi-subject study can
 * be browsed and toggled from a single side-panel.
 *
 * BrainView listens to @c dataChanged / @c rowsInserted on the
 * model, rebuilds its scene-map entries on the fly and pushes the
 * resulting render-state into the QRhi pipeline &mdash; no other
 * controller needs to know that a surface was added or hidden.
 */

#ifndef BRAINTREEMODEL_H
#define BRAINTREEMODEL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp3D_global.h"

#include <QStandardItemModel>
#include <fs/fs_surface.h>
#include <fs/fs_annotation.h>
#include <mne/mne_bem_surface.h>
#include <mne/mne_source_spaces.h>
#include <fiff/fiff_dig_point.h>
#include <conn/network/network.h>

namespace INVLIB {
class InvEcdSet;
}

class NetworkTreeItem;

class SurfaceTreeItem;
class BemTreeItem;

/**
 * @brief Hierarchical item model organizing all 3-D scene objects (surfaces, sensors, sources, networks) for QTreeView.
 */
class DISP3DSHARED_EXPORT BrainTreeModel : public QStandardItemModel
{
    Q_OBJECT
public:
    explicit BrainTreeModel(QObject *parent = nullptr);
    ~BrainTreeModel() override = default;

    // Helper functions to populate the tree
    SurfaceTreeItem* addSurface(const QString &subject, const QString &hemi, const QString &surfType, const FSLIB::FsSurface &surface);
    
    // Add annotation to an existing surface item? Or separate item?
    // Usually annotation is property of a surface or child of surface.
    // Let's attach it to the surface item for now or find the surface item and update it.
    bool addAnnotation(const QString &subject, const QString &hemi, const FSLIB::FsAnnotation &annotation);

    // Add BEM surface
    BemTreeItem* addBemSurface(const QString &subject, const QString &bemName, const MNELIB::MNEBemSurface &bemSurf);

    // Helpers to populate the tree
    void addSensors(const QString &type, const QList<QStandardItem*> &items);
    void addDipoles(const INVLIB::InvEcdSet &set);

    // Add digitizer points with proper categorization (Cardinal, HPI, EEG, Extra)
    void addDigitizerData(const QList<FIFFLIB::FiffDigPoint> &digitizerPoints);

    // Add source space points
    void addSourceSpace(const MNELIB::MNESourceSpaces &srcSpace);

    // Add connectivity network
    NetworkTreeItem* addNetwork(const CONNLIB::Network &network, const QString &name = "Network");

private:
   // Helpers to find specific items
   QStandardItem* getSubjectItem(const QString &subject);
};

#endif // BRAINTREEMODEL_H
