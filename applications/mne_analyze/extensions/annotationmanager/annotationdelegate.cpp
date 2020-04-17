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
    const ANSHAREDLIB::AnnotationModel* pAnnotationModel = static_cast<const ANSHAREDLIB::AnnotationModel*>(index.model());

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

//=============================================================================================================

void AnnotationDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    switch(index.column()) {
        case 0: {
            int value = index.model()->data(index, Qt::DisplayRole).toInt();
            QSpinBox *spinBox = static_cast<QSpinBox*>(editor);
            spinBox->setValue(value);
            break;
        }

        case 1: {
            double value = index.model()->data(index, Qt::DisplayRole).toDouble();
            QDoubleSpinBox *spinBox = static_cast<QDoubleSpinBox*>(editor);
            spinBox->setValue(value);
            break;
        }

        case 2: {
            int value = index.model()->data(index, Qt::DisplayRole).toInt();
            QComboBox *spinBox = static_cast<QComboBox*>(editor);
            spinBox->setCurrentText(QString().number(value));
            break;
        }
    }
}

//=============================================================================================================

void AnnotationDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                    const QModelIndex &index) const
{
    switch(index.column()) {
        case 0: {
            QSpinBox *spinBox = static_cast<QSpinBox*>(editor);
            spinBox->interpretText();
            int value = spinBox->value();

            model->setData(index, value, Qt::EditRole);
            break;
        }

        case 1: {
            QDoubleSpinBox *spinBox = static_cast<QDoubleSpinBox*>(editor);
            spinBox->interpretText();
            double value = spinBox->value();

            model->setData(index, value, Qt::EditRole);
            break;
        }

        case 2: {
            QComboBox *spinBox = static_cast<QComboBox*>(editor);
            QString value = spinBox->currentText();

            model->setData(index, value.toInt(), Qt::EditRole);
            break;
        }
    }
}

//=============================================================================================================

void AnnotationDelegate::updateEditorGeometry(QWidget *editor,
    const QStyleOptionViewItem &option, const QModelIndex &/* index */) const
{
    editor->setGeometry(option.rect);
}
