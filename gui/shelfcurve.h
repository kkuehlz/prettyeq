#ifndef SHELFCURVE_H
#define SHELFCURVE_H

#include "filtercurve.h"
#include <QBrush>
#include <QGraphicsItem>
#include <QObject>
#include <QPen>

#define SLOPE_DELTA 20
#define SLOPE_MAX 330

class ShelfCurve : public QObject, public FilterCurve
{
    Q_OBJECT
    Q_INTERFACES(QGraphicsItem)
public:
    explicit ShelfCurve(QPen pen, QBrush brush, bool guiOnly = false, QObject *parent = nullptr);
    virtual ~ShelfCurve();
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    QPointF controlPoint() const override;
    qreal slope() const;

protected:
    virtual QPointF clampP2() const = 0;

private:
    QPainterPath bezierPainter() const;

public slots:
    void pointPositionChanged(qreal dx, qreal dy);
    virtual void pointSlopeChanged(int delta) = 0;
signals:
    void resync(FilterCurve *curve);
    void filterParamsChanged(ShimFilterPtr filter, ShelfCurve *curve);

protected:
    QPointF p0, p1, p2, p3;
};

#endif // SHELFCURVE_H
