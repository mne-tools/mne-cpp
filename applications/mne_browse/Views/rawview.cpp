#include "rawview.h"

RawView::RawView(QWidget *parent)
    : QAbstractItemView(parent)
{
    horizontalScrollBar()->setRange(0, 0);
    verticalScrollBar()->setRange(0, 0);

    //set private members
//    margin = 8;
    totalSize = 300;
//    pieSize = totalSize - 2 * margin;
    validItems = 0;
//    totalValue = 0.0;
//    rubberBand = 0;
}

void RawView::dataChanged(const QModelIndex &topLeft,
                          const QModelIndex &bottomRight,
                          const QVector<int> &)
{
    QAbstractItemView::dataChanged(topLeft, bottomRight);

//    validItems = 0;
//    totalValue = 0.0;

    for (int row = 0; row < model()->rowCount(rootIndex()); ++row) {

        QModelIndex index = model()->index(row, 1, rootIndex());
        double value = model()->data(index).toDouble();

        if (value > 0.0) {
//            totalValue += value;
//            validItems++;
        }
    }
    viewport()->update();
}

/*
    Returns the item that covers the coordinate given in the view.
 */

QModelIndex RawView::indexAt(const QPoint &point) const
{
    if (validItems == 0)
        return QModelIndex();

    if (wx < totalSize) {
        double cx = wx - totalSize / 2;
        double cy = totalSize / 2 - wy; // positive cy for items above the center

        // Determine the distance from the center point of the pie chart.
        double d = pow(pow(cx, 2) + pow(cy, 2), 0.5);

        if (d == 0 || d > pieSize / 2)
            return QModelIndex();

        // Determine the angle of the point.
        double angle = (180 / M_PI) * acos(cx / d);
        if (cy < 0)
            angle = 360 - angle;

        // Find the relevant slice of the pie.
        double startAngle = 0.0;

        for (int row = 0; row < model()->rowCount(rootIndex()); ++row) {

            QModelIndex index = model()->index(row, 1, rootIndex());
            double value = model()->data(index).toDouble();

            if (value > 0.0) {
                double sliceAngle = 360 * value / totalValue;

                if (angle >= startAngle && angle < (startAngle + sliceAngle))
                    return model()->index(row, 1, rootIndex());

                startAngle += sliceAngle;
            }
        }
    }

    return QModelIndex();
}

bool RawView::isIndexHidden(const QModelIndex & /*index*/) const
{
    return false;
}

/*
    Returns the rectangle of the item at position \a index in the
    model. The rectangle is in contents coordinates.
 */

QRect RawView::itemRect(const QModelIndex &index) const
{
    if (!index.isValid())
        return QRect();

    // Check whether the index's row is in the list of rows represented
    // by slices.
    QModelIndex valueIndex;

    if (index.column() != 1)
        valueIndex = model()->index(index.row(), 1, rootIndex());
    else
        valueIndex = index;

    if (model()->data(valueIndex).toDouble() <= 0.0)
        return QRect();

    int listItem = 0;
    for (int row = index.row()-1; row >= 0; --row) {
        if (model()->data(model()->index(row, 1, rootIndex())).toDouble() > 0.0)
            listItem++;
    }

    double itemHeight;

    switch (index.column()) {
    case 0:
        itemHeight = QFontMetrics(viewOptions().font).height();

        return QRect(totalSize,
                     int(margin + listItem*itemHeight),
                     totalSize - margin, int(itemHeight));
    case 1:
        return viewport()->rect();
    }
    return QRect();
}


QRegion RawView::itemRegion(const QModelIndex &index) const
{
    if (!index.isValid())
        return QRegion();

    if (index.column() != 1)
        return itemRect(index);

    if (model()->data(index).toDouble() <= 0.0)
        return QRegion();

    double startAngle = 0.0;
    for (int row = 0; row < model()->rowCount(rootIndex()); ++row) {

        QModelIndex sliceIndex = model()->index(row, 1, rootIndex());
        double value = model()->data(sliceIndex).toDouble();

        if (value > 0.0) {
            double angle = 360 * value / totalValue;

            if (sliceIndex == index) {
                QPainterPath slicePath;
                slicePath.moveTo(totalSize / 2, totalSize / 2);
                slicePath.arcTo(margin, margin, margin+pieSize, margin+pieSize,
                                startAngle, angle);
                slicePath.closeSubpath();

                return QRegion(slicePath.toFillPolygon().toPolygon());
            }

            startAngle += angle;
        }
    }

    return QRegion();
}

int RawView::horizontalOffset() const
{
    return horizontalScrollBar()->value();
}

void RawView::mousePressEvent(QMouseEvent *event)
{
//    QAbstractItemView::mousePressEvent(event);
//    origin = event->pos();
//    if (!rubberBand)
//        rubberBand = new QRubberBand(QRubberBand::Rectangle, viewport());
//    rubberBand->setGeometry(QRect(origin, QSize()));
//    rubberBand->show();
}

void RawView::mouseMoveEvent(QMouseEvent *event)
{
//    if (rubberBand)
//        rubberBand->setGeometry(QRect(origin, event->pos()).normalized());
//    QAbstractItemView::mouseMoveEvent(event);
}

void RawView::mouseReleaseEvent(QMouseEvent *event)
{
//    QAbstractItemView::mouseReleaseEvent(event);
//    if (rubberBand)
//        rubberBand->hide();
//    viewport()->update();
}

QModelIndex RawView::moveCursor(QAbstractItemView::CursorAction cursorAction,
                                Qt::KeyboardModifiers /*modifiers*/)
{
    QModelIndex current = currentIndex();

    switch (cursorAction) {
        case MoveLeft:
        case MoveUp:
            if (current.row() > 0)
                current = model()->index(current.row() - 1, current.column(),
                                         rootIndex());
            else
                current = model()->index(0, current.column(), rootIndex());
            break;
        case MoveRight:
        case MoveDown:
            if (current.row() < rows(current) - 1)
                current = model()->index(current.row() + 1, current.column(),
                                         rootIndex());
            else
                current = model()->index(rows(current) - 1, current.column(),
                                         rootIndex());
            break;
        default:
            break;
    }

    viewport()->update();
    return current;
}

void RawView::paintEvent(QPaintEvent *event)
{
    QItemSelectionModel *selections = selectionModel();
    QStyleOptionViewItem option = viewOptions();

    QBrush background = option.palette.base();
    QPen foreground(option.palette.color(QPalette::WindowText));

    QPainter painter(viewport());
    painter.setRenderHint(QPainter::Antialiasing);

    painter.fillRect(event->rect(), background);
    painter.setPen(foreground);

    // Viewport rectangles
    QRect pieRect = QRect(margin, margin, pieSize, pieSize);

    if (validItems <= 0)
        return;

    painter.save();
    painter.translate(pieRect.x() - horizontalScrollBar()->value(),
                      pieRect.y() - verticalScrollBar()->value());
    painter.drawEllipse(0, 0, pieSize, pieSize);
    double startAngle = 0.0;
    int row;

    for (row = 0; row < model()->rowCount(rootIndex()); ++row) {
        QModelIndex index = model()->index(row, 1, rootIndex());
        double value = model()->data(index).toDouble();

        if (value > 0.0) {
            double angle = 360*value/totalValue;

            QModelIndex colorIndex = model()->index(row, 0, rootIndex());
            QColor color = QColor(model()->data(colorIndex, Qt::DecorationRole).toString());

            if (currentIndex() == index)
                painter.setBrush(QBrush(color, Qt::Dense4Pattern));
            else if (selections->isSelected(index))
                painter.setBrush(QBrush(color, Qt::Dense3Pattern));
            else
                painter.setBrush(QBrush(color));

            painter.drawPie(0, 0, pieSize, pieSize, int(startAngle*16), int(angle*16));

            startAngle += angle;
        }
    }
    painter.restore();

    int keyNumber = 0;

    for (row = 0; row < model()->rowCount(rootIndex()); ++row) {
        QModelIndex index = model()->index(row, 1, rootIndex());
        double value = model()->data(index).toDouble();

        if (value > 0.0) {
            QModelIndex labelIndex = model()->index(row, 0, rootIndex());

            QStyleOptionViewItem option = viewOptions();
            option.rect = visualRect(labelIndex);
            if (selections->isSelected(labelIndex))
                option.state |= QStyle::State_Selected;
            if (currentIndex() == labelIndex)
                option.state |= QStyle::State_HasFocus;
            itemDelegate()->paint(&painter, option, labelIndex);

            ++keyNumber;
        }
    }
}

void RawView::resizeEvent(QResizeEvent * /* event */)
{
    updateGeometries();
}

int RawView::rows(const QModelIndex &index) const
{
    return model()->rowCount(model()->parent(index));
}

//void RawView::rowsInserted(const QModelIndex &parent, int start, int end)
//{
//    for (int row = start; row <= end; ++row) {
//        QModelIndex index = model()->index(row, 1, rootIndex());
//        double value = model()->data(index).toDouble();

//        if (value > 0.0) {
//            totalValue += value;
//            ++validItems;
//        }
//    }

//    QAbstractItemView::rowsInserted(parent, start, end);
//}

//void RawView::rowsAboutToBeRemoved(const QModelIndex &parent, int start, int end)
//{
//    for (int row = start; row <= end; ++row) {
//        QModelIndex index = model()->index(row, 1, rootIndex());
//        double value = model()->data(index).toDouble();
//        if (value > 0.0) {
//            totalValue -= value;
//            --validItems;
//        }
//    }

//    QAbstractItemView::rowsAboutToBeRemoved(parent, start, end);
//}

void RawView::scrollContentsBy(int dx, int dy)
{
    viewport()->scroll(dx, dy);
}

void RawView::scrollTo(const QModelIndex &index, ScrollHint)
{
    QRect area = viewport()->rect();
    QRect rect = visualRect(index);

    if (rect.left() < area.left()) {
        horizontalScrollBar()->setValue(
            horizontalScrollBar()->value() + rect.left() - area.left());
    } else if (rect.right() > area.right()) {
        horizontalScrollBar()->setValue(
            horizontalScrollBar()->value() + qMin(
                rect.right() - area.right(), rect.left() - area.left()));
    }

    if (rect.top() < area.top()) {
        verticalScrollBar()->setValue(
            verticalScrollBar()->value() + rect.top() - area.top());
    } else if (rect.bottom() > area.bottom()) {
        verticalScrollBar()->setValue(
            verticalScrollBar()->value() + qMin(
                rect.bottom() - area.bottom(), rect.top() - area.top()));
    }

    update();
}

/*
    Find the indices corresponding to the extent of the selection.
 */

void PieView::setSelection(const QRect &rect, QItemSelectionModel::SelectionFlags command)
{
    // Use content widget coordinates because we will use the itemRegion()
    // function to check for intersections.

    QRect contentsRect = rect.translated(
                            horizontalScrollBar()->value(),
                            verticalScrollBar()->value()).normalized();

    int rows = model()->rowCount(rootIndex());
    int columns = model()->columnCount(rootIndex());
    QModelIndexList indexes;

    for (int row = 0; row < rows; ++row) {
        for (int column = 0; column < columns; ++column) {
            QModelIndex index = model()->index(row, column, rootIndex());
            QRegion region = itemRegion(index);
            if (region.intersects(contentsRect))
                indexes.append(index);
        }
    }

    if (indexes.size() > 0) {
        int firstRow = indexes[0].row();
        int lastRow = indexes[0].row();
        int firstColumn = indexes[0].column();
        int lastColumn = indexes[0].column();

        for (int i = 1; i < indexes.size(); ++i) {
            firstRow = qMin(firstRow, indexes[i].row());
            lastRow = qMax(lastRow, indexes[i].row());
            firstColumn = qMin(firstColumn, indexes[i].column());
            lastColumn = qMax(lastColumn, indexes[i].column());
        }

        QItemSelection selection(
            model()->index(firstRow, firstColumn, rootIndex()),
            model()->index(lastRow, lastColumn, rootIndex()));
        selectionModel()->select(selection, command);
    } else {
        QModelIndex noIndex;
        QItemSelection selection(noIndex, noIndex);
        selectionModel()->select(selection, command);
    }

    update();
}

void RawView::updateGeometries()
{
    horizontalScrollBar()->setPageStep(viewport()->width());
    horizontalScrollBar()->setRange(0, qMax(0, 2*totalSize - viewport()->width()));
    verticalScrollBar()->setPageStep(viewport()->height());
    verticalScrollBar()->setRange(0, qMax(0, totalSize - viewport()->height()));
}

int RawView::verticalOffset() const
{
    return verticalScrollBar()->value();
}

/*
    Returns the position of the item in viewport coordinates.
 */

QRect RawView::visualRect(const QModelIndex &index) const
{
    QRect rect = itemRect(index);
    if (!rect.isValid())
        return rect;

    return QRect(rect.left() - horizontalScrollBar()->value(),
                 rect.top() - verticalScrollBar()->value(),
                 rect.width(), rect.height());
}

/*
    Returns a region corresponding to the selection in viewport coordinates.
 */

//QRegion RawView::visualRegionForSelection(const QItemSelection &selection) const
//{
//    int ranges = selection.count();

//    if (ranges == 0)
//        return QRect();

//    QRegion region;
//    for (int i = 0; i < ranges; ++i) {
//        QItemSelectionRange range = selection.at(i);
//        for (int row = range.top(); row <= range.bottom(); ++row) {
//            for (int col = range.left(); col <= range.right(); ++col) {
//                QModelIndex index = model()->index(row, col, rootIndex());
//                region += visualRect(index);
//            }
//        }
//    }
//    return region;
//}

