#ifndef CURVEPOINT_H
#define CURVEPOINT_H

#include "filtercurve.h"
#include <QBrush>
#include <QGraphicsItem>
#include <QObject>

class CurvePoint : public QObject, public QGraphicsItem
{
    Q_OBJECT
    Q_INTERFACES(QGraphicsItem)
public:
    explicit CurvePoint(QBrush normalBrush, QBrush lightBrush, QObject *parent = nullptr);
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    void setPos(const QPointF & pos);
    void setPos(qreal x, qreal y);
protected:
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void wheelEvent(QGraphicsSceneWheelEvent *event) override;

public slots:
    void resync(FilterCurve *curve);

signals:
    void pointPositionChanged(qreal dx, qreal dy);
    void pointSlopeChanged(int delta);

private:
    QBrush normalBrush, lightBrush;
    QPointF lastPos;
    int wheelDeltaSum = 0;
};

#endif // CURVEPOINT_H
