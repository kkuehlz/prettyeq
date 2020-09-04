#include "peakingcurve.h"
#include <QDebug>
#include <QGraphicsScene>
#include <QPainter>
#include <QTransform>
#define MIN_WIDTH 40
#define MAX_WIDTH 500
#define SLOPE 10
#define SPLINE_CAPACITY 16

PeakingCurve::PeakingCurve(QPen pen, QBrush brush, bool guiOnly, QObject *parent) : QObject(parent), FilterCurve(pen, brush, guiOnly)
{
    setZValue(100000);

    lineSpline.reserve(SPLINE_CAPACITY);
    fillSpline.reserve(SPLINE_CAPACITY);
    reset();
    updateSplineGeometry();
}

QRectF PeakingCurve::boundingRect() const
{
    return fillSpline.boundingRect();
}

void PeakingCurve::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setPen(getActivePen());
    painter->drawPath(lineSpline);
    painter->fillPath(fillSpline, getActiveBrush());

#if 0
    // debugging - draw control points
    QPen rpen(Qt::red);
    rpen.setWidth(10);
    painter->save();
    painter->setPen(rpen);
    painter->drawPoint(c1);
    painter->drawPoint(c2);
    painter->restore();
#endif

#if 0
    // debugging - draw boundingRect()
    QPen rpen(Qt::red);
    rpen.setWidth(2);
    painter->save();
    painter->setPen(rpen);
    painter->drawRect(boundingRect());
    painter->restore();
#endif
}

QPointF PeakingCurve::controlPoint() const
{
    return mapToScene(ip);
}

void PeakingCurve::reset()
{
    p0 = QPointF(0, 0);
    ip = QPointF(100, 0);
    p1 = QPointF(200, 0);
    c1 = QPointF((p0.x() + ip.x()) / 2, ip.y());
    c2 = QPointF((ip.x() + p1.x()) / 2, ip.y());
    updateSplineGeometry();
    prepareGeometryChange();
    this->update();
}

void PeakingCurve::updateSplineGeometry()
{
    lineSpline.clear();
    lineSpline.moveTo(p0);
    lineSpline.quadTo(c1, ip);
    lineSpline.moveTo(ip);
    lineSpline.quadTo(c2, p1);
    Q_ASSERT(lineSpline.elementCount() == 8);
    Q_ASSERT(lineSpline.capacity() == SPLINE_CAPACITY);

    fillSpline.clear();
    fillSpline.moveTo(p0);
    fillSpline.quadTo(c1, ip);
    fillSpline.moveTo(ip);
    fillSpline.quadTo(c2, p1);
    fillSpline.lineTo(p0);
    Q_ASSERT(fillSpline.elementCount() == 9);
    Q_ASSERT(fillSpline.capacity() == SPLINE_CAPACITY);
}

void PeakingCurve::pointPositionChanged(CurvePoint *point)
{
    QPointF curvePoint = mapFromScene(point->pos());
    QPointF delta = curvePoint - ip;
    ip = curvePoint;
    p0.setX(p0.x() + delta.x());
    p1.setX(p1.x() + delta.x());
    c1 = QPointF((p0.x() + ip.x()) / 2, ip.y());
    c2 = QPointF((ip.x() + p1.x()) / 2, ip.y());
    updateSplineGeometry();
    prepareGeometryChange();
    this->update();
    emit resync(this);
    emit filterParamsChanged(filter, this);
}

void PeakingCurve::pointSlopeChanged(int delta)
{
    qreal p0x = p0.x() + SLOPE*delta;
    qreal p1x = p1.x() - SLOPE*delta;
    if (p1x - p0x >= MIN_WIDTH && p1x - p0x <= MAX_WIDTH) {
        p0.setX(p0x);
        p1.setX(p1x);
        c1 = QPointF((p0.x() + ip.x()) / 2, ip.y());
        c2 = QPointF((ip.x() + p1.x()) / 2, ip.y());
        updateSplineGeometry();
        prepareGeometryChange();
        this->update();
        emit resync(this);
        emit filterParamsChanged(filter, this);
    }

}
