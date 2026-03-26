//=============================================================================================================
/**
 * @file     workflowminimapwidget.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @version  dev
 * @date     March, 2026
 *
 * @brief    Declares a compact mini-canvas for visualizing workflow DAG relationships.
 */

#ifndef MNE_ANALYZE_STUDIO_WORKFLOWMINIMAPWIDGET_H
#define MNE_ANALYZE_STUDIO_WORKFLOWMINIMAPWIDGET_H

#include <QHash>
#include <QJsonObject>
#include <QRectF>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QWidget>

namespace MNEANALYZESTUDIO
{

/**
 * @brief Compact graph canvas used by the workflow navigator page.
 */
class WorkflowMiniMapWidget : public QWidget
{
    Q_OBJECT

public:
    explicit WorkflowMiniMapWidget(QWidget* parent = nullptr);

    void setWorkflowGraph(const QJsonObject& graph);
    void setFocusNodeUid(const QString& nodeUid);

    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;

signals:
    void nodeActivated(const QString& nodeUid);

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    struct NodeVisual
    {
        QString uid;
        QString label;
        QString stage;
        QString skillId;
        QString status;
        QStringList dependencies;
        QStringList dependents;
        int depth = 0;
        int topologicalIndex = 0;
        QRectF rect;
    };

    void rebuildGraphCache();
    void rebuildFocusState();
    void rebuildLayout();
    QString relationForNode(const QString& nodeUid) const;
    QString edgeRelationForNodes(const QString& sourceNodeUid, const QString& targetNodeUid) const;
    int nodeIndexAt(const QPoint& position) const;

    QJsonObject m_graph;
    QString m_focusNodeUid;
    QVector<NodeVisual> m_nodes;
    QHash<QString, int> m_indexByUid;
    QHash<QString, QStringList> m_dependenciesByNodeUid;
    QHash<QString, QStringList> m_dependentsByNodeUid;
    QStringList m_directDependencies;
    QStringList m_directDependents;
    QStringList m_upstreamNodeUids;
    QStringList m_downstreamNodeUids;
};

} // namespace MNEANALYZESTUDIO

#endif // MNE_ANALYZE_STUDIO_WORKFLOWMINIMAPWIDGET_H
