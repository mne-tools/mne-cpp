#ifndef ANNOTATIONDELEGATE_H
#define ANNOTATIONDELEGATE_H

#include "annotationmodel.h"
#include "annotationview.h"

#include <QItemDelegate>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QDebug>
#include <QSettings>

class AnnotationDelegate : public QItemDelegate
{
    Q_OBJECT
public:
    AnnotationDelegate(QObject *parent = 0);

    QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;
//    void setEditorData(QWidget *editor, const QModelIndex &index) const;
//    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;
//    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

#endif // ANNOTATIONDELEGATE_H
