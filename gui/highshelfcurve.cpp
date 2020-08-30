#include "highshelfcurve.h"

HighShelfCurve::HighShelfCurve(QPen pen, QBrush brush, bool guiOnly, QObject *parent)
    : ShelfCurve(pen, brush, guiOnly, parent)
{
    p0 = QPointF(0, 0);
    p3 = QPointF(-330, 0);
    p1 = QPointF((p3.x() - p0.x()) * 0.3, p0.y());
    p2 = QPointF((p1.x() + p3.x())/ 2, p3.y());
}

HighShelfCurve::~HighShelfCurve()
{

}

QPointF HighShelfCurve::clampP2() const
{
    return (p2.x() > 0) ? QPointF(0, p2.y()) : p2;
}

void HighShelfCurve::pointSlopeChanged(int delta)
{
    bool reduce = delta > 0;
    int offset = -delta * SLOPE_DELTA;
    if ((qAbs(p3.x() - p2.x()) - offset < SLOPE_MAX  || !reduce) && (p2.x() - offset > p3.x() || reduce)) {
        p2.setX(p2.x() - offset);
        this->update();
        emit resync(this);
        emit filterParamsChanged(filter, this);
    }
}
