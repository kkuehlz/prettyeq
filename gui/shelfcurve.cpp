#include "macro.h"
#include "shelfcurve.h"
#include <QtGlobal>
#include <QDebug>
#include <QGraphicsScene>
#include <QPainter>
#include <QPainterPath>
#define CURVE_CAPACITY 16

ShelfCurve::ShelfCurve(QPen pen, QBrush brush, bool guiOnly, QObject *parent)
    : QObject(parent), FilterCurve(pen, brush, guiOnly)
{
    setZValue(100000);

    lineCurve = std::unique_ptr<QPainterPath>(new QPainterPath());
    fillCurve = std::unique_ptr<QPainterPath>(new QPainterPath());
#if (QT_VERSION >= QT_VERSION_CHECK(5, 13, 0))
    lineCurve->reserve(CURVE_CAPACITY);
    fillCurve->reserve(CURVE_CAPACITY);
#endif
}

ShelfCurve::~ShelfCurve()
{

}

void ShelfCurve::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setPen(getActivePen());
    painter->drawPath(*lineCurve);
    painter->fillPath(*fillCurve, getActiveBrush());
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
    updateCurveGeometry();
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

void ShelfCurve::updateCurveGeometry()
{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 13, 0))
    lineCurve->clear();
#else
    lineCurve.reset(new QPainterPath());
#endif
    lineCurve->moveTo(p0);
    lineCurve->cubicTo(p1, clampP2(), p3);
    Q_ASSERT(lineCurve->elementCount() == 4);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 13, 0))
    Q_ASSERT(lineCurve->capacity() == CURVE_CAPACITY);
#endif

#if (QT_VERSION >= QT_VERSION_CHECK(5, 13, 0))
    fillCurve->clear();
#else
    fillCurve.reset(new QPainterPath());
#endif
    fillCurve->moveTo(p0);
    fillCurve->cubicTo(p1, clampP2(), p3);
    fillCurve->lineTo(0, 0);
    fillCurve->lineTo(p0);
    Q_ASSERT(fillCurve->elementCount() <= 6);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 13, 0))
    Q_ASSERT(fillCurve->capacity() == CURVE_CAPACITY);
#endif
}

QRectF ShelfCurve::boundingRect() const
{
    return bezierPainter().boundingRect();
}
