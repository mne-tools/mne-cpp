//=============================================================================================================
/**
 * @file     workflowminimapwidget.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @version  dev
 * @date     March, 2026
 *
 * @brief    Implements a compact mini-canvas for workflow DAG navigation.
 */

#include "workflowminimapwidget.h"

#include <QEvent>
#include <QFont>
#include <QJsonArray>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QSet>
#include <QStyleOption>

#include <algorithm>

using namespace MNEANALYZESTUDIO;

namespace
{

void insertSortedUnique(QStringList& values, const QString& value)
{
    if(value.isEmpty() || values.contains(value)) {
        return;
    }

    const auto insertPosition = std::lower_bound(values.begin(), values.end(), value);
    values.insert(std::distance(values.begin(), insertPosition), value);
}

QHash<QString, QString> outputProducerMap(const QJsonObject& graph)
{
    QHash<QString, QString> producerByOutputUid;
    const QJsonArray pipeline = graph.value(QStringLiteral("pipeline")).toArray();
    for(const QJsonValue& value : pipeline) {
        const QJsonObject node = value.toObject();
        const QString nodeUid = node.value(QStringLiteral("uid")).toString().trimmed();
        const QJsonObject outputs = node.value(QStringLiteral("outputs")).toObject();
        for(auto it = outputs.constBegin(); it != outputs.constEnd(); ++it) {
            const QString outputUid = it.value().toString().trimmed();
            if(!nodeUid.isEmpty() && !outputUid.isEmpty()) {
                producerByOutputUid.insert(outputUid, nodeUid);
            }
        }
    }

    return producerByOutputUid;
}

QStringList dependencyNodeUids(const QJsonObject& node, const QHash<QString, QString>& outputProducerByUid)
{
    QStringList dependencyNodeUids;
    const QJsonObject inputs = node.value(QStringLiteral("inputs")).toObject();
    for(auto it = inputs.constBegin(); it != inputs.constEnd(); ++it) {
        const QString inputUid = it.value().toString().trimmed();
        const QString producerNodeUid = outputProducerByUid.value(inputUid);
        if(!producerNodeUid.isEmpty()) {
            insertSortedUnique(dependencyNodeUids, producerNodeUid);
        }
    }

    return dependencyNodeUids;
}

QVector<QJsonObject> topologicallyOrderedNodes(const QJsonObject& graph)
{
    const QJsonArray pipeline = graph.value(QStringLiteral("pipeline")).toArray();
    QHash<QString, QJsonObject> nodeByUid;
    QStringList sortedNodeUids;
    for(const QJsonValue& value : pipeline) {
        const QJsonObject node = value.toObject();
        const QString nodeUid = node.value(QStringLiteral("uid")).toString().trimmed();
        if(nodeUid.isEmpty()) {
            continue;
        }

        nodeByUid.insert(nodeUid, node);
        insertSortedUnique(sortedNodeUids, nodeUid);
    }

    const QHash<QString, QString> outputProducerByUid = outputProducerMap(graph);
    QHash<QString, QStringList> dependentNodeUidsByNodeUid;
    QHash<QString, int> indegreeByNodeUid;
    for(const QString& nodeUid : sortedNodeUids) {
        indegreeByNodeUid.insert(nodeUid, 0);
    }

    for(const QString& nodeUid : sortedNodeUids) {
        const QStringList dependencies = dependencyNodeUids(nodeByUid.value(nodeUid), outputProducerByUid);
        indegreeByNodeUid[nodeUid] = dependencies.size();
        for(const QString& dependencyNodeUid : dependencies) {
            insertSortedUnique(dependentNodeUidsByNodeUid[dependencyNodeUid], nodeUid);
        }
    }

    QStringList readyNodeUids;
    for(const QString& nodeUid : sortedNodeUids) {
        if(indegreeByNodeUid.value(nodeUid) == 0) {
            insertSortedUnique(readyNodeUids, nodeUid);
        }
    }

    QVector<QJsonObject> orderedNodes;
    orderedNodes.reserve(sortedNodeUids.size());
    while(!readyNodeUids.isEmpty()) {
        const QString nodeUid = readyNodeUids.takeFirst();
        orderedNodes.append(nodeByUid.value(nodeUid));

        const QStringList dependents = dependentNodeUidsByNodeUid.value(nodeUid);
        for(const QString& dependentNodeUid : dependents) {
            const int remainingDependencies = indegreeByNodeUid.value(dependentNodeUid) - 1;
            indegreeByNodeUid[dependentNodeUid] = remainingDependencies;
            if(remainingDependencies == 0) {
                insertSortedUnique(readyNodeUids, dependentNodeUid);
            }
        }
    }

    if(orderedNodes.size() == sortedNodeUids.size()) {
        return orderedNodes;
    }

    QVector<QJsonObject> fallbackNodes;
    fallbackNodes.reserve(pipeline.size());
    for(const QJsonValue& value : pipeline) {
        fallbackNodes.append(value.toObject());
    }
    return fallbackNodes;
}

QStringList reachableNodeUids(const QString& startNodeUid, const QHash<QString, QStringList>& adjacencyByNodeUid)
{
    QStringList visitedNodeUids;
    QStringList pendingNodeUids = adjacencyByNodeUid.value(startNodeUid);
    QSet<QString> seenNodeUids;

    while(!pendingNodeUids.isEmpty()) {
        const QString nodeUid = pendingNodeUids.takeFirst();
        if(nodeUid.isEmpty() || seenNodeUids.contains(nodeUid)) {
            continue;
        }

        seenNodeUids.insert(nodeUid);
        insertSortedUnique(visitedNodeUids, nodeUid);
        for(const QString& adjacentNodeUid : adjacencyByNodeUid.value(nodeUid)) {
            if(!adjacentNodeUid.isEmpty() && !seenNodeUids.contains(adjacentNodeUid)) {
                insertSortedUnique(pendingNodeUids, adjacentNodeUid);
            }
        }
    }

    return visitedNodeUids;
}

QString nodeStatus(const QJsonObject& node)
{
    const QString status = node.value(QStringLiteral("runtime")).toObject().value(QStringLiteral("status")).toString().trimmed();
    return status.isEmpty() ? QStringLiteral("pending") : status;
}

QColor baseFillColor(const QString& status)
{
    if(status == QLatin1String("completed")) {
        return QColor(48, 54, 61, 210);
    }
    if(status == QLatin1String("running")) {
        return QColor(32, 120, 140, 210);
    }
    if(status == QLatin1String("failed")) {
        return QColor(143, 47, 47, 210);
    }

    return QColor(56, 64, 74, 205);
}

QColor baseBorderColor(const QString& status)
{
    if(status == QLatin1String("completed")) {
        return QColor(123, 141, 158, 190);
    }
    if(status == QLatin1String("running")) {
        return QColor(92, 202, 238, 200);
    }
    if(status == QLatin1String("failed")) {
        return QColor(248, 113, 113, 210);
    }

    return QColor(134, 146, 158, 180);
}

QColor relationFillColor(const QString& relation, const QString& status)
{
    if(relation == QLatin1String("focus")) {
        return QColor(47, 129, 247, 210);
    }
    if(relation == QLatin1String("direct_upstream")) {
        return QColor(173, 114, 24, 205);
    }
    if(relation == QLatin1String("direct_downstream")) {
        return QColor(35, 134, 54, 205);
    }
    if(relation == QLatin1String("transitive_upstream")) {
        return QColor(173, 114, 24, 92);
    }
    if(relation == QLatin1String("transitive_downstream")) {
        return QColor(35, 134, 54, 88);
    }

    return baseFillColor(status);
}

QColor relationBorderColor(const QString& relation, const QString& status)
{
    if(relation == QLatin1String("focus")) {
        return QColor(125, 180, 255);
    }
    if(relation == QLatin1String("direct_upstream")) {
        return QColor(242, 204, 96);
    }
    if(relation == QLatin1String("direct_downstream")) {
        return QColor(86, 211, 100);
    }
    if(relation == QLatin1String("transitive_upstream")) {
        return QColor(227, 179, 65);
    }
    if(relation == QLatin1String("transitive_downstream")) {
        return QColor(63, 185, 80);
    }

    return baseBorderColor(status);
}

QColor relationTextColor(const QString& relation)
{
    if(relation == QLatin1String("focus")) {
        return Qt::white;
    }

    return QColor(236, 239, 244);
}

QColor edgeColorForRelation(const QString& relation)
{
    if(relation == QLatin1String("focus")) {
        return QColor(125, 180, 255, 230);
    }
    if(relation == QLatin1String("direct_upstream")) {
        return QColor(242, 204, 96, 225);
    }
    if(relation == QLatin1String("direct_downstream")) {
        return QColor(86, 211, 100, 225);
    }
    if(relation == QLatin1String("transitive_upstream")) {
        return QColor(227, 179, 65, 140);
    }
    if(relation == QLatin1String("transitive_downstream")) {
        return QColor(63, 185, 80, 140);
    }

    return QColor(123, 141, 158, 96);
}

qreal edgeWidthForRelation(const QString& relation)
{
    if(relation == QLatin1String("focus")
       || relation == QLatin1String("direct_upstream")
       || relation == QLatin1String("direct_downstream")) {
        return 2.3;
    }

    if(relation == QLatin1String("transitive_upstream")
       || relation == QLatin1String("transitive_downstream")) {
        return 1.6;
    }

    return 1.1;
}

} // namespace

WorkflowMiniMapWidget::WorkflowMiniMapWidget(QWidget* parent)
: QWidget(parent)
{
    setMinimumHeight(180);
    setMouseTracking(true);
}

void WorkflowMiniMapWidget::setWorkflowGraph(const QJsonObject& graph)
{
    m_graph = graph;
    rebuildGraphCache();
    rebuildLayout();
    update();
}

void WorkflowMiniMapWidget::setFocusNodeUid(const QString& nodeUid)
{
    const QString trimmedNodeUid = nodeUid.trimmed();
    if(m_focusNodeUid == trimmedNodeUid) {
        return;
    }

    m_focusNodeUid = trimmedNodeUid;
    rebuildFocusState();
    update();
}

QSize WorkflowMiniMapWidget::minimumSizeHint() const
{
    return QSize(280, 180);
}

QSize WorkflowMiniMapWidget::sizeHint() const
{
    return QSize(420, 220);
}

void WorkflowMiniMapWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    QStyleOption option;
    option.initFrom(this);
    style()->drawPrimitive(QStyle::PE_Widget, &option, &painter, this);

    const QRectF panelRect = rect().adjusted(2, 2, -2, -2);
    painter.setPen(QPen(QColor(48, 54, 61, 200), 1.0));
    painter.setBrush(QColor(13, 17, 23, 150));
    painter.drawRoundedRect(panelRect, 12.0, 12.0);

    if(m_nodes.isEmpty()) {
        painter.setPen(QColor(157, 167, 179));
        painter.drawText(panelRect.adjusted(18, 18, -18, -18),
                         Qt::AlignCenter | Qt::TextWordWrap,
                         m_graph.isEmpty()
                             ? QStringLiteral("No active workflow graph.\nOpen a .mne file to render the dependency map.")
                             : QStringLiteral("Workflow graph has no pipeline nodes to render."));
        return;
    }

    for(const NodeVisual& node : m_nodes) {
        const int sourceNodeIndex = m_indexByUid.value(node.uid, -1);
        if(sourceNodeIndex < 0) {
            continue;
        }

        for(const QString& dependentNodeUid : node.dependents) {
            const int targetNodeIndex = m_indexByUid.value(dependentNodeUid, -1);
            if(targetNodeIndex < 0) {
                continue;
            }

            const QRectF sourceRect = m_nodes.at(sourceNodeIndex).rect;
            const QRectF targetRect = m_nodes.at(targetNodeIndex).rect;
            const QPointF startPoint(sourceRect.right(), sourceRect.center().y());
            const QPointF endPoint(targetRect.left(), targetRect.center().y());
            const qreal controlOffset = qMax(18.0, (endPoint.x() - startPoint.x()) * 0.45);

            QPainterPath path(startPoint);
            path.cubicTo(QPointF(startPoint.x() + controlOffset, startPoint.y()),
                         QPointF(endPoint.x() - controlOffset, endPoint.y()),
                         endPoint);

            const QString edgeRelation = edgeRelationForNodes(node.uid, dependentNodeUid);
            painter.setPen(QPen(edgeColorForRelation(edgeRelation),
                                edgeWidthForRelation(edgeRelation),
                                Qt::SolidLine,
                                Qt::RoundCap,
                                Qt::RoundJoin));
            painter.setBrush(Qt::NoBrush);
            painter.drawPath(path);

            const QPointF arrowBase = endPoint - QPointF(7.0, 0.0);
            QPolygonF arrowHead;
            arrowHead << endPoint
                      << QPointF(arrowBase.x() - 5.0, arrowBase.y() - 3.5)
                      << QPointF(arrowBase.x() - 5.0, arrowBase.y() + 3.5);
            painter.setBrush(edgeColorForRelation(edgeRelation));
            painter.setPen(Qt::NoPen);
            painter.drawPolygon(arrowHead);
        }
    }

    const QFont baseFont = painter.font();
    for(const NodeVisual& node : m_nodes) {
        const QString relation = relationForNode(node.uid);
        const QColor fillColor = relationFillColor(relation, node.status);
        const QColor borderColor = relationBorderColor(relation, node.status);
        const QColor textColor = relationTextColor(relation);

        painter.setPen(QPen(borderColor, relation == QLatin1String("focus") ? 2.2 : 1.2));
        painter.setBrush(fillColor);
        painter.drawRoundedRect(node.rect, 10.0, 10.0);

        QFont titleFont = baseFont;
        titleFont.setBold(relation == QLatin1String("focus")
                          || relation == QLatin1String("direct_upstream")
                          || relation == QLatin1String("direct_downstream"));
        painter.setFont(titleFont);
        painter.setPen(textColor);
        const QRectF titleRect = node.rect.adjusted(10.0, 7.0, -10.0, -20.0);
        const QString titleSource = node.label.isEmpty() ? node.uid : node.label;
        const QString title = painter.fontMetrics().elidedText(titleSource, Qt::ElideMiddle, static_cast<int>(titleRect.width()));
        painter.drawText(titleRect, Qt::AlignLeft | Qt::AlignVCenter, title);

        QFont subtitleFont = baseFont;
        subtitleFont.setPointSizeF(qMax(8.0, subtitleFont.pointSizeF() - 1.0));
        painter.setFont(subtitleFont);
        const QRectF subtitleRect = node.rect.adjusted(10.0, 24.0, -10.0, -6.0);
        const QString secondaryDescriptor = node.stage.isEmpty()
            ? (node.skillId.isEmpty() ? QStringLiteral("skill") : node.skillId)
            : node.stage;
        const QString subtitleText = QStringLiteral("%1 | %2")
                                         .arg(node.status, secondaryDescriptor);
        const QString subtitle = painter.fontMetrics().elidedText(subtitleText, Qt::ElideRight, static_cast<int>(subtitleRect.width()));
        painter.drawText(subtitleRect, Qt::AlignLeft | Qt::AlignVCenter, subtitle);
    }
}

void WorkflowMiniMapWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    rebuildLayout();
}

void WorkflowMiniMapWidget::mousePressEvent(QMouseEvent* event)
{
    if(event->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(event);
        return;
    }

    const int nodeIndex = nodeIndexAt(event->pos());
    if(nodeIndex < 0 || nodeIndex >= m_nodes.size()) {
        QWidget::mousePressEvent(event);
        return;
    }

    emit nodeActivated(m_nodes.at(nodeIndex).uid);
    event->accept();
}

void WorkflowMiniMapWidget::mouseMoveEvent(QMouseEvent* event)
{
    setCursor(nodeIndexAt(event->pos()) >= 0 ? Qt::PointingHandCursor : Qt::ArrowCursor);
    QWidget::mouseMoveEvent(event);
}

void WorkflowMiniMapWidget::leaveEvent(QEvent* event)
{
    setCursor(Qt::ArrowCursor);
    QWidget::leaveEvent(event);
}

void WorkflowMiniMapWidget::rebuildGraphCache()
{
    m_nodes.clear();
    m_indexByUid.clear();
    m_dependenciesByNodeUid.clear();
    m_dependentsByNodeUid.clear();

    if(m_graph.isEmpty()) {
        rebuildFocusState();
        return;
    }

    const QVector<QJsonObject> orderedNodes = topologicallyOrderedNodes(m_graph);
    const QHash<QString, QString> outputProducerByUid = outputProducerMap(m_graph);
    QHash<QString, int> depthByNodeUid;

    for(int index = 0; index < orderedNodes.size(); ++index) {
        const QJsonObject node = orderedNodes.at(index);
        const QString nodeUid = node.value(QStringLiteral("uid")).toString().trimmed();
        if(nodeUid.isEmpty()) {
            continue;
        }

        const QStringList dependencies = dependencyNodeUids(node, outputProducerByUid);
        m_dependenciesByNodeUid.insert(nodeUid, dependencies);

        int depth = 0;
        for(const QString& dependencyNodeUid : dependencies) {
            depth = qMax(depth, depthByNodeUid.value(dependencyNodeUid, 0) + 1);
            insertSortedUnique(m_dependentsByNodeUid[dependencyNodeUid], nodeUid);
        }
        depthByNodeUid.insert(nodeUid, depth);

        NodeVisual visual;
        visual.uid = nodeUid;
        visual.label = node.value(QStringLiteral("label")).toString().trimmed();
        visual.stage = node.value(QStringLiteral("stage")).toString().trimmed();
        visual.skillId = node.value(QStringLiteral("skill_id")).toString().trimmed();
        visual.status = nodeStatus(node);
        visual.dependencies = dependencies;
        visual.depth = depth;
        visual.topologicalIndex = index;
        m_indexByUid.insert(nodeUid, m_nodes.size());
        m_nodes.append(visual);
    }

    for(NodeVisual& node : m_nodes) {
        node.dependents = m_dependentsByNodeUid.value(node.uid);
    }

    rebuildFocusState();
}

void WorkflowMiniMapWidget::rebuildFocusState()
{
    if(!m_indexByUid.contains(m_focusNodeUid)) {
        m_focusNodeUid.clear();
    }

    m_directDependencies = m_dependenciesByNodeUid.value(m_focusNodeUid);
    m_directDependents = m_dependentsByNodeUid.value(m_focusNodeUid);
    m_upstreamNodeUids = reachableNodeUids(m_focusNodeUid, m_dependenciesByNodeUid);
    m_downstreamNodeUids = reachableNodeUids(m_focusNodeUid, m_dependentsByNodeUid);
}

void WorkflowMiniMapWidget::rebuildLayout()
{
    if(m_nodes.isEmpty()) {
        return;
    }

    const QRectF contentRect = rect().adjusted(16.0, 16.0, -16.0, -16.0);
    if(contentRect.width() <= 12.0 || contentRect.height() <= 12.0) {
        return;
    }

    int maxDepth = 0;
    int maxNodesPerDepth = 0;
    QHash<int, QVector<int>> nodeIndexesByDepth;
    for(int index = 0; index < m_nodes.size(); ++index) {
        nodeIndexesByDepth[m_nodes.at(index).depth].append(index);
        maxDepth = qMax(maxDepth, m_nodes.at(index).depth);
    }
    for(auto it = nodeIndexesByDepth.constBegin(); it != nodeIndexesByDepth.constEnd(); ++it) {
        maxNodesPerDepth = qMax(maxNodesPerDepth, it.value().size());
    }

    const int columnCount = maxDepth + 1;
    const qreal preferredNodeWidth = 158.0;
    const qreal preferredNodeHeight = 42.0;
    qreal nodeWidth = qBound(80.0,
                             columnCount > 0
                                 ? (contentRect.width() - (columnCount - 1) * 18.0) / qMax(1, columnCount)
                                 : preferredNodeWidth,
                             preferredNodeWidth);
    nodeWidth = qMin(nodeWidth, contentRect.width());
    const qreal xStep = columnCount > 1
        ? qMax(0.0, (contentRect.width() - nodeWidth) / static_cast<qreal>(columnCount - 1))
        : 0.0;

    qreal nodeHeight = preferredNodeHeight;
    if(maxNodesPerDepth > 0) {
        const qreal candidateHeight = (contentRect.height() - (maxNodesPerDepth - 1) * 6.0) / maxNodesPerDepth;
        nodeHeight = qBound(24.0, candidateHeight, preferredNodeHeight);
    }

    for(auto it = nodeIndexesByDepth.constBegin(); it != nodeIndexesByDepth.constEnd(); ++it) {
        const QVector<int> nodeIndexes = it.value();
        const int count = nodeIndexes.size();
        if(count <= 0) {
            continue;
        }

        const qreal x = columnCount > 1
            ? contentRect.left() + it.key() * xStep
            : contentRect.left() + (contentRect.width() - nodeWidth) * 0.5;

        const qreal requiredHeight = count * nodeHeight + qMax(0, count - 1) * 8.0;
        qreal y = contentRect.top();
        qreal yStep = nodeHeight + 8.0;
        if(requiredHeight <= contentRect.height()) {
            y = contentRect.top() + (contentRect.height() - requiredHeight) * 0.5;
        } else if(count > 1) {
            yStep = qMax(0.0, (contentRect.height() - nodeHeight) / static_cast<qreal>(count - 1));
        }

        for(int i = 0; i < count; ++i) {
            const int nodeIndex = nodeIndexes.at(i);
            const qreal nodeY = requiredHeight <= contentRect.height()
                ? y + i * (nodeHeight + 8.0)
                : contentRect.top() + i * yStep;
            m_nodes[nodeIndex].rect = QRectF(x, nodeY, nodeWidth, nodeHeight);
        }
    }
}

QString WorkflowMiniMapWidget::relationForNode(const QString& nodeUid) const
{
    if(m_focusNodeUid.isEmpty()) {
        return QStringLiteral("neutral");
    }
    if(nodeUid == m_focusNodeUid) {
        return QStringLiteral("focus");
    }
    if(m_directDependencies.contains(nodeUid)) {
        return QStringLiteral("direct_upstream");
    }
    if(m_directDependents.contains(nodeUid)) {
        return QStringLiteral("direct_downstream");
    }
    if(m_upstreamNodeUids.contains(nodeUid)) {
        return QStringLiteral("transitive_upstream");
    }
    if(m_downstreamNodeUids.contains(nodeUid)) {
        return QStringLiteral("transitive_downstream");
    }

    return QStringLiteral("neutral");
}

QString WorkflowMiniMapWidget::edgeRelationForNodes(const QString& sourceNodeUid, const QString& targetNodeUid) const
{
    if(m_focusNodeUid.isEmpty()) {
        return QStringLiteral("neutral");
    }
    if(sourceNodeUid == m_focusNodeUid || targetNodeUid == m_focusNodeUid) {
        return QStringLiteral("focus");
    }
    if(m_directDependencies.contains(sourceNodeUid) && targetNodeUid == m_focusNodeUid) {
        return QStringLiteral("direct_upstream");
    }
    if(sourceNodeUid == m_focusNodeUid && m_directDependents.contains(targetNodeUid)) {
        return QStringLiteral("direct_downstream");
    }
    if(m_upstreamNodeUids.contains(sourceNodeUid) && m_upstreamNodeUids.contains(targetNodeUid)) {
        return QStringLiteral("transitive_upstream");
    }
    if(m_downstreamNodeUids.contains(sourceNodeUid) && m_downstreamNodeUids.contains(targetNodeUid)) {
        return QStringLiteral("transitive_downstream");
    }
    if(m_directDependencies.contains(sourceNodeUid) || m_directDependencies.contains(targetNodeUid)) {
        return QStringLiteral("transitive_upstream");
    }
    if(m_directDependents.contains(sourceNodeUid) || m_directDependents.contains(targetNodeUid)) {
        return QStringLiteral("transitive_downstream");
    }

    return QStringLiteral("neutral");
}

int WorkflowMiniMapWidget::nodeIndexAt(const QPoint& position) const
{
    for(int index = 0; index < m_nodes.size(); ++index) {
        if(m_nodes.at(index).rect.contains(position)) {
            return index;
        }
    }

    return -1;
}
