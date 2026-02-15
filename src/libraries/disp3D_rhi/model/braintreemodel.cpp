//=============================================================================================================
/**
 * @file     braintreemodel.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Gabriel B Motta <gabrielbenmotta@gmail.com>;
 *           Juan Garcia-Prieto <juangpc@gmail.com>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    2.0.0
 * @date     February, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
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
 * @brief    BrainTreeModel class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "braintreemodel.h"
#include "model/items/surfacetreeitem.h"
#include "model/items/bemtreeitem.h"
#include "model/items/sensortreeitem.h"
#include "model/items/dipoletreeitem.h"
#include "model/items/sourcespacetreeitem.h"
#include "model/items/digitizersettreeitem.h"
#include "model/items/networktreeitem.h"
#include <inverse/dipoleFit/ecd_set.h>
#include <mne/mne_hemisphere.h>

BrainTreeModel::BrainTreeModel(QObject *parent)
    : QStandardItemModel(parent)
{
    // Set headers
    setHorizontalHeaderLabels(QStringList() << "Data" << "Description");
}

SurfaceTreeItem* BrainTreeModel::addSurface(const QString &subject, const QString &hemi, const QString &surfType, const FSLIB::Surface &surface)
{
    QStandardItem* subjectItem = getSubjectItem(subject);
    
    // Find or create Hemi item
    QStandardItem* hemiItem = nullptr;
    for(int i = 0; i < subjectItem->rowCount(); ++i) {
        if (subjectItem->child(i)->text() == hemi) {
            hemiItem = subjectItem->child(i);
            break;
        }
    }
    
    if (!hemiItem) {
        hemiItem = new QStandardItem(hemi);
        subjectItem->appendRow(hemiItem);
    }
    
    // Create Surface Item
    SurfaceTreeItem* surfItem = new SurfaceTreeItem(surfType);
    surfItem->setSurfaceData(surface);
    
    hemiItem->appendRow(surfItem);
    return surfItem;
}

bool BrainTreeModel::addAnnotation(const QString &subject, const QString &hemi, const FSLIB::Annotation &annotation)
{
    QStandardItem* subjectItem = getSubjectItem(subject);
    if (!subjectItem) return false;

    QStandardItem* hemiItem = nullptr;
    for(int i = 0; i < subjectItem->rowCount(); ++i) {
        if (subjectItem->child(i)->text() == hemi) {
            hemiItem = subjectItem->child(i);
            break;
        }
    }
    
    if (!hemiItem) return false;

    // Apply annotation to ALL surfaces in this hemi? Or just one?
    // Ex_brain_view logic applies atlas to the loaded surfaces of that hemi.
    // So we iterate children of hemiItem and if they are SurfaceTreeItems, set annotation.
    bool applied = false;
    for(int i = 0; i < hemiItem->rowCount(); ++i) {
        SurfaceTreeItem* item = dynamic_cast<SurfaceTreeItem*>(hemiItem->child(i));
        if (item) {
            item->setAnnotationData(annotation);
            applied = true;
        }
    }
    return applied;
}

QStandardItem* BrainTreeModel::getSubjectItem(const QString &subject)
{
    // Search top level
    QList<QStandardItem*> items = findItems(subject);
    if (!items.isEmpty()) {
        return items.first();
    }
    
    // Create new
    QStandardItem* item = new QStandardItem(subject);
    appendRow(item);
    return item;
}

BemTreeItem* BrainTreeModel::addBemSurface(const QString &subject, const QString &bemName, const MNELIB::MNEBemSurface &bemSurf)
{
    QStandardItem *subjItem = getSubjectItem(subject);
    
    // Check if BEM item already exists to avoid duplicates? 
    // Usually BEM is added once.
    
    BemTreeItem *bemItem = new BemTreeItem(bemName, bemSurf);
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
    
    subjItem->appendRow(bemItem);
    
    return bemItem;
}

//=============================================================================================================

void BrainTreeModel::addSensors(const QString &type, const QList<QStandardItem*> &items)
{
    QStandardItem* parentItem = new QStandardItem(type);
    parentItem->setCheckable(true);
    parentItem->setCheckState(Qt::Checked);
    
    for(auto* item : items) {
        parentItem->appendRow(item);
    }
    
    this->appendRow(parentItem);
}

//=============================================================================================================

void BrainTreeModel::addDipoles(const INVERSELIB::ECDSet &set)
{
    DipoleTreeItem* item = new DipoleTreeItem("Dipoles", set);
    item->setCheckable(true);
    item->setCheckState(Qt::Checked);
    this->appendRow(item);
}

//=============================================================================================================

void BrainTreeModel::addSourceSpace(const MNELIB::MNESourceSpace &srcSpace)
{
    QStandardItem* parentItem = new QStandardItem("Source Space");
    parentItem->setCheckable(true);
    parentItem->setCheckState(Qt::Checked);

    // Color: pinkish-red matching disp3D's source space rendering
    QColor srcColor(212, 28, 92);

    for (int h = 0; h < srcSpace.size(); ++h) {
        const MNELIB::MNEHemisphere &hemi = srcSpace[h];
        QString hemiLabel = (h == 0) ? "LH" : "RH";

        // Collect all source point positions for this hemisphere
        QVector<QVector3D> positions;
        positions.reserve(hemi.vertno.size());
        for (int i = 0; i < hemi.vertno.size(); ++i) {
            int vIdx = hemi.vertno(i);
            if (vIdx < 0 || vIdx >= hemi.rr.rows()) continue;
            positions.append(QVector3D(hemi.rr(vIdx, 0), hemi.rr(vIdx, 1), hemi.rr(vIdx, 2)));
        }

        // One item per hemisphere with all positions batched (0.00075 = 0.75mm radius, same as disp3D)
        parentItem->appendRow(new SourceSpaceTreeItem(hemiLabel, positions, srcColor, 0.00075f));
        qDebug() << "BrainTreeModel: Source space" << hemiLabel
                 << "- points:" << positions.size()
                 << "coord_frame:" << hemi.coord_frame;
        if (!positions.isEmpty()) {
            qDebug() << "  First point:" << positions.first()
                     << "  Last point:" << positions.last();
        }
    }

    this->appendRow(parentItem);
}

//=============================================================================================================

void BrainTreeModel::addDigitizerData(const QList<FIFFLIB::FiffDigPoint> &digitizerPoints)
{
    if (digitizerPoints.isEmpty()) return;

    auto *setItem = new DigitizerSetTreeItem("Digitizer", digitizerPoints);
    this->appendRow(setItem);

    qDebug() << "BrainTreeModel: Added digitizer set with"
             << setItem->totalPointCount() << "points in"
             << setItem->rowCount() << "categories";
}

//=============================================================================================================

NetworkTreeItem* BrainTreeModel::addNetwork(const CONNECTIVITYLIB::Network &network, const QString &name)
{
    QString displayName = name;
    if (displayName.isEmpty()) {
        displayName = network.getConnectivityMethod();
        if (displayName.isEmpty()) displayName = "Network";
    }

    QString objectKey = "net_" + displayName.toLower().replace(" ", "_");

    auto *item = new NetworkTreeItem(displayName, objectKey);
    item->setCheckable(true);
    item->setCheckState(Qt::Checked);

    this->appendRow(item);

    qDebug() << "BrainTreeModel: Added network" << displayName
             << "with" << network.getNodes().size() << "nodes and"
             << network.getFullEdges().size() << "edges";

    return item;
}
