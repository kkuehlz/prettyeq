#ifndef PEAKINGCURVE_H
#define PEAKINGCURVE_H

#include <QBrush>
#include <QGraphicsItem>
#include <QObject>
#include <QPen>
#include "curvepoint.h"
#include "filtercurve.h"

typedef enum SplinePart {
    SplineLeft,
    SplineRight,
} SplinePart;

class PeakingCurve : public QObject, public FilterCurve
{
    Q_OBJECT
    Q_INTERFACES(QGraphicsItem)
public:
    explicit PeakingCurve(QPen pen, QBrush brush, bool guiOnly = false, QObject *parent = nullptr);
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    QPainterPath splinePainter(SplinePart whichSpline) const;

    QPointF controlPoint() const override;

public slots:
    void pointPositionChanged(CurvePoint *point);
    void pointSlopeChanged(int delta);
signals:
    void resync(FilterCurve *curve);
    void filterParamsChanged(ShimFilterPtr filter, PeakingCurve *curve);

private:
    QPointF p0, p1, c1, c2, ip;
};

#endif // PEAKINGCURVE_H
