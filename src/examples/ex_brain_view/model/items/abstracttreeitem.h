#ifndef ABSTRACTTREEITEM_H
#define ABSTRACTTREEITEM_H

#include <QStandardItem>
#include <QVariant>
#include <QMatrix4x4>
#include <QVector3D>

class AbstractTreeItem : public QStandardItem
{
public:
    enum ItemRole {
        TypeRole = Qt::UserRole + 100,
        VisibleRole,
        TransformRole,
        ColorRole,
        AlphaRole
    };

    enum ItemType {
        AbstractItem = 0,
        SurfaceItem,
        BemItem,
        SensorItem,
        DipoleItem,
        SourceSpaceItem,
        DigitizerItem
    };

    explicit AbstractTreeItem(const QString &text = "", int type = AbstractItem);
    virtual ~AbstractTreeItem() = default;

    int type() const override;

    // Type-safe accessors for common properties
    void setVisible(bool visible);
    bool isVisible() const;

    void setTransform(const QMatrix4x4 &transform);
    QMatrix4x4 transform() const;

    void setColor(const QColor &color);
    QColor color() const;

    void setAlpha(float alpha);
    float alpha() const;

protected:
    int m_type;
};

#endif // ABSTRACTTREEITEM_H
