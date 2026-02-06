#include "braintreemodel.h"
#include "surfacetreeitem.h"
#include "bemtreeitem.h"
#include "sensortreeitem.h"
#include "dipoletreeitem.h"
#include <inverse/dipoleFit/ecd_set.h>

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
