#include "annotationdelegate.h"

AnnotationDelegate::AnnotationDelegate(QObject *parent)
: QItemDelegate(parent)
{

}

//=============================================================================================================

QWidget* AnnotationDelegate::createEditor(QWidget *parent,
     const QStyleOptionViewItem &/* option */,
     const QModelIndex & index) const
{
    const AnnotationModel* pAnnotationModel = static_cast<const AnnotationModel*>(index.model());

    switch(index.column()) {
        case 0: {
            QSpinBox *editor = new QSpinBox(parent);
            editor->setMinimum(0);
            editor->setMaximum(pAnnotationModel->getFirstLastSample().second);
            return editor;
        }

        case 1: {
            QDoubleSpinBox *editor = new QDoubleSpinBox(parent);
            editor->setMinimum(0.0);
            editor->setMaximum(pAnnotationModel->getFirstLastSample().second / pAnnotationModel->getSampleFreq());
            editor->setSingleStep(0.01);
            return editor;
        }

        case 2: {
            QComboBox *editor = new QComboBox(parent);
            editor->addItems(pAnnotationModel->getEventTypeList());
            return editor;
        }
    }

    QWidget *returnWidget = new QWidget();
    return returnWidget;
}
