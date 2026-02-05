#ifndef BRAINTREEMODEL_H
#define BRAINTREEMODEL_H

#include <QStandardItemModel>
#include <fs/surface.h>
#include <fs/annotation.h>
#include <mne/mne_bem_surface.h>

namespace INVERSELIB {
class ECDSet;
}

class SurfaceTreeItem;
class BemTreeItem;

class BrainTreeModel : public QStandardItemModel
{
    Q_OBJECT
public:
    explicit BrainTreeModel(QObject *parent = nullptr);
    ~BrainTreeModel() override = default;

    // Helper functions to populate the tree
    SurfaceTreeItem* addSurface(const QString &subject, const QString &hemi, const QString &surfType, const FSLIB::Surface &surface);
    
    // Add annotation to an existing surface item? Or separate item?
    // Usually annotation is property of a surface or child of surface.
    // Let's attach it to the surface item for now or find the surface item and update it.
    bool addAnnotation(const QString &subject, const QString &hemi, const FSLIB::Annotation &annotation);

    // Add BEM surface
    BemTreeItem* addBemSurface(const QString &subject, const QString &bemName, const MNELIB::MNEBemSurface &bemSurf);

    // Helpers to populate the tree
    void addSensors(const QString &type, const QList<QStandardItem*> &items);
    void addDipoles(const INVERSELIB::ECDSet &set);

private:
   // Helpers to find specific items
   QStandardItem* getSubjectItem(const QString &subject);
};

#endif // BRAINTREEMODEL_H
