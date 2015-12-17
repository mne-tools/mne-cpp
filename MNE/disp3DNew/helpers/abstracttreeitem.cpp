#include "abstracttreeitem.h"

using namespace DISP3DNEWLIB;

AbstractTreeItem::AbstractTreeItem(const int& iType, const QString& text)
: QStandardItem(text)
, m_iType(iType)
{
    createToolTip();
}


//*************************************************************************************************************

AbstractTreeItem::~AbstractTreeItem()
{
}


//*************************************************************************************************************

int  AbstractTreeItem::type() const
{
    return m_iType;
}


//*************************************************************************************************************

QList<QStandardItem*> AbstractTreeItem::findChildren(const int& type)
{
    QList<QStandardItem*> itemList;

    if(this->hasChildren()) {
        for(int row = 0; row<this->rowCount(); row++) {
            for(int col = 0; col<this->columnCount(); col++) {
                if(this->child(row, col)->type() == type) {
                    itemList.append(this->child(row, col));
                }
            }
        }
    }

    return itemList;
}


//*************************************************************************************************************

QList<QStandardItem*> AbstractTreeItem::findChildren(const QString& text)
{
    QList<QStandardItem*> itemList;

    if(this->hasChildren()) {
        for(int row = 0; row<this->rowCount(); row++) {
            for(int col = 0; col<this->columnCount(); col++) {
                if(this->child(row, col)->text() == text) {
                    itemList.append(this->child(row, col));
                }
            }
        }
    }

    return itemList;
}


//*************************************************************************************************************

AbstractTreeItem& AbstractTreeItem::operator<<(AbstractTreeItem* newItem)
{
    this->appendRow(newItem);

    return *this;
}


//*************************************************************************************************************

AbstractTreeItem& AbstractTreeItem::operator<<(AbstractTreeItem& newItem)
{
    this->appendRow(&newItem);

    return *this;
}


//*************************************************************************************************************

void AbstractTreeItem::createToolTip()
{
    QString sToolTip;

    switch(m_iType) {
    case BrainTreeModelItemTypes::UnknownItem:
        sToolTip = "Unknown";
        break;
    case BrainTreeModelItemTypes::SurfaceSetItem:
        sToolTip = "Brain surface set";
        break;
    case BrainTreeModelItemTypes::HemisphereItem:
        sToolTip = "Brain hemisphere";
        break;
    case BrainTreeModelItemTypes::SurfaceItem:
        sToolTip = "Brain surface";
        break;
    case BrainTreeModelItemTypes::AnnotationItem:
        sToolTip = "Brain annotation";
        break;
    case BrainTreeModelItemTypes::SurfaceFileName:
        sToolTip = "Surface file name";
        break;
    case BrainTreeModelItemTypes::SurfaceFilePath:
        sToolTip = "Surface file path";
        break;
    case BrainTreeModelItemTypes::AnnotFileName:
        sToolTip = "Annotation file name";
        break;
    case BrainTreeModelItemTypes::AnnotFilePath:
        sToolTip = "Annotation file path";
        break;
    case BrainTreeModelItemTypes::SurfaceType:
        sToolTip = "Surface type";
        break;
    case BrainTreeModelItemTypes::SurfaceColorGyri:
        sToolTip = "Color Gyri";
        break;
    case BrainTreeModelItemTypes::SurfaceColorSulci:
        sToolTip = "Color Sulci";
        break;
    case BrainTreeModelItemTypes::SurfaceColorInfoOrigin:
        sToolTip = "Information used to color the surface";
        break;
    case BrainTreeModelItemTypes::RTDataItem:
        sToolTip = "Real time data item";
        break;
    case BrainTreeModelItemTypes::RTDataStreamStatus:
        sToolTip = "Turn real time data streaming on/off";
        break;
    case BrainTreeModelItemTypes::RTDataSourceSpaceType:
        sToolTip = "Used source space type";
        break;
    case BrainTreeModelItemTypes::RTDataColormapType:
        sToolTip = "Used color mapping";
        break;
    default:
        sToolTip = "Unknown";
        break;
    }

    this->setToolTip(sToolTip);
}
