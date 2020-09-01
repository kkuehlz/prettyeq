#include "macro.h"
#include "shelfcurve.h"
#include <QDebug>
#include <QGraphicsScene>
#include <QPainter>
#include <QPainterPath>

ShelfCurve::ShelfCurve(QPen pen, QBrush brush, bool guiOnly, QObject *parent)
    : QObject(parent), FilterCurve(pen, brush, guiOnly)
{
    setZValue(100000);
    p0 = QPointF(0, 0);
    p1 = QPointF(0, 0);
    p2 = QPointF(0, 0);
    p3 = QPointF(0, 0);
}

ShelfCurve::~ShelfCurve()
{

}

void ShelfCurve::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setPen(getActivePen());
    QPainterPath shelf = bezierPainter();
    painter->drawPath(shelf);
    shelf.lineTo(0, 0);
    shelf.lineTo(p0);
    painter->fillPath(shelf, getActiveBrush());
#if 0
    // debugging - draw control points
    QPen rpen(Qt::red);
    QPen ypen(Qt::yellow);
    QPen gpen(Qt::green);
    QPen bpen(Qt::blue);
    rpen.setWidth(10);
    ypen.setWidth(10);
    bpen.setWidth(10);
    gpen.setWidth(10);
    painter->save();
    painter->setPen(rpen);
    painter->drawPoint(p0);
    painter->setPen(ypen);
    painter->drawPoint(p1);
    painter->setPen(gpen);
    painter->drawPoint(p2);
    painter->setPen(bpen);
    painter->drawPoint(p3);
    painter->restore();
#endif

#if 0
    // debugging - draw boundingRect()
    QPen rpen(Qt::red);
    rpen.setWidth(10);
    QPen bpen(Qt::blue);
    bpen.setWidth(10);
    QPen gpen(Qt::green);
    gpen.setWidth(10);
    QPen ypen(Qt::yellow);
    ypen.setWidth(10);
    painter->save();
    painter->setPen(rpen);
    painter->drawPoint(boundingRect().bottomLeft());
    painter->setPen(bpen);
    painter->drawPoint(boundingRect().topLeft());
    painter->setPen(gpen);
    painter->drawPoint(boundingRect().bottomRight());
    painter->setPen(ypen);
    painter->drawPoint(boundingRect().topRight());
    painter->restore();
#endif

}

QPointF ShelfCurve::controlPoint() const
{
    return mapToScene(p1);
}

qreal ShelfCurve::slope() const
{
    /* The i have no idea what the fuck I'm doing equation. */
    return qAbs(bezierPainter().slopeAtPercent(0.7)) + 0.5;
}

void ShelfCurve::pointPositionChanged(CurvePoint *point) {
    QPointF curvePoint = mapFromScene(point->pos());
    QPointF delta = curvePoint - p1;
    p0.setY(p0.y() + delta.y());
    p1 = curvePoint;
    p2.setX(p2.x() + delta.x());
    p3.setX(p3.x() + delta.x());
    prepareGeometryChange();
    this->update();
    emit resync(this);
    emit filterParamsChanged(filter, this);
}

QPainterPath ShelfCurve::bezierPainter() const
{
    QPainterPath shelf;
    shelf.moveTo(p0);
    shelf.cubicTo(p1, clampP2(), p3);
    return shelf;
}

QRectF ShelfCurve::boundingRect() const
{
    return bezierPainter().boundingRect();
}
