#include "braintreemodel.h"
#include "surfacetreeitem.h"
#include "bemtreeitem.h"
#include "sensortreeitem.h"
#include "dipoletreeitem.h"
#include "sourcespacetreeitem.h"
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
