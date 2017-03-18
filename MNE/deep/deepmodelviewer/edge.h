#ifndef EDGE_H
#define EDGE_H

#include <QGraphicsItem>

class Node;

class Edge : public QGraphicsItem
{
public:
    Edge(Node *sourceNode, Node *destNode);

    Node *sourceNode() const;
    Node *destNode() const;

    void adjust();

    void setWeight(float weight);

    enum { Type = UserType + 2 };
    int type() const override { return Type; }

protected:
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    void updateLineWidth();
    void updateColor();

private:
    Node *source, *dest;

    QPointF m_sourcePoint;
    QPointF m_destPoint;
    qreal   m_arrowSize;
    QColor  m_color;
    qreal   m_penWidth;

    float   m_weight;
};

#endif // EDGE_H
