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
    case BrainTreeItemTypes::UnknownItem:
        sToolTip = "Unknown";
        break;
    case BrainTreeItemTypes::SurfaceSetItem:
        sToolTip = "Brain surface set";
        break;
    case BrainTreeItemTypes::HemisphereItem:
        sToolTip = "Brain hemisphere";
        break;
    case BrainTreeItemTypes::SurfaceItem:
        sToolTip = "Brain surface";
        break;
    case BrainTreeItemTypes::AnnotationItem:
        sToolTip = "Brain annotation";
        break;
    case BrainTreeItemTypes::SurfaceFileName:
        sToolTip = "Surface file name";
        break;
    case BrainTreeItemTypes::SurfaceFilePath:
        sToolTip = "Surface file path";
        break;
    case BrainTreeItemTypes::AnnotFileName:
        sToolTip = "Annotation file name";
        break;
    case BrainTreeItemTypes::AnnotFilePath:
        sToolTip = "Annotation file path";
        break;
    case BrainTreeItemTypes::SurfaceFileType:
        sToolTip = "Surface type";
        break;
    case BrainTreeItemTypes::SurfaceColorGyri:
        sToolTip = "Color Gyri";
        break;
    case BrainTreeItemTypes::SurfaceColorSulci:
        sToolTip = "Color Sulci";
        break;
    default:
        sToolTip = "Unknown";
        break;
    }

    this->setToolTip(sToolTip);
}
