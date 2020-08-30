#ifndef EQHOVERER_H
#define EQHOVERER_H

#include "curvepoint.h"
#include "filtercurve.h"
#include <QGraphicsItem>
#include <QObject>

class EqHoverer : public QObject, public QGraphicsItem
{
    Q_OBJECT
    Q_INTERFACES(QGraphicsItem)
public:
    explicit EqHoverer(FilterCurve *curve, CurvePoint *point, QObject *parent = nullptr);
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
protected:
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;
private:
    void maybeShowPoint();

public slots:
    void resync(FilterCurve *curve);

signals:

private:
    FilterCurve *curve;
    CurvePoint *point;
    unsigned int pointState;
};

#endif // EQHOVERER_H
