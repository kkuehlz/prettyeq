#include "peakingcurve.h"
#include <QDebug>
#include <QGraphicsScene>
#include <QPainter>
#include <QTransform>
#define MIN_WIDTH 40
#define MAX_WIDTH 500
#define SLOPE 10

PeakingCurve::PeakingCurve(QPen pen, QBrush brush, bool guiOnly, QObject *parent) : QObject(parent), FilterCurve(pen, brush, guiOnly)
{
    setZValue(100000); // TODO: move to interface

    p0 = QPointF(0, 0);
    ip = QPointF(100, 0);
    p1 = QPointF(200, 0);

    c1 = QPointF((p0.x() + ip.x()) / 2, ip.y());
    c2 = QPointF((ip.x() + p1.x()) / 2, ip.y());
}

QRectF PeakingCurve::boundingRect() const
{
    /* Top left of the left spline, bottom right of right spline. */
    QPainterPath lspline = splinePainter(SplineLeft);
    QPainterPath rspline = splinePainter(SplineRight);
    Q_ASSERT(lspline.boundingRect().bottomRight() == rspline.boundingRect().bottomLeft());
    return QRectF(lspline.boundingRect().topLeft(), rspline.boundingRect().bottomRight());
}

void PeakingCurve::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setPen(getActivePen());

    /* Left spline */
    QPainterPath lspline = splinePainter(SplineLeft);
    painter->drawPath(lspline);
    lspline.lineTo(ip.x(), 0);
    lspline.lineTo(p0);
    painter->fillPath(lspline, getActiveBrush());

    /* Right spline */
    QPainterPath rspline = splinePainter(SplineRight);
    painter->drawPath(rspline);
    rspline.lineTo(ip.x(), 0);
    rspline.lineTo(ip);
    painter->fillPath(rspline, getActiveBrush());

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

QPainterPath PeakingCurve::splinePainter(SplinePart whichSpline) const
{
    QPainterPath splinePart;
    switch(whichSpline) {
    case SplineLeft:
        splinePart.moveTo(p0);
        splinePart.quadTo(c1, ip);
        break;
    case SplineRight:
        splinePart.moveTo(ip);
        splinePart.quadTo(c2, p1);
        break;
    default:
        Q_ASSERT(0);
    }
    return splinePart;
}

QPointF PeakingCurve::controlPoint() const
{
    return mapToScene(ip);
}

void PeakingCurve::pointPositionChanged(qreal dx, qreal dy)
{
    QPointF delta(dx, dy);
    ip += delta;
    p0.setX(p0.x() + dx);
    p1.setX(p1.x() + dx);
    c1 = QPointF((p0.x() + ip.x()) / 2, ip.y());
    c2 = QPointF((ip.x() + p1.x()) / 2, ip.y());
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
        prepareGeometryChange();
        this->update();
        emit resync(this);
        emit filterParamsChanged(filter, this);
    }

}
