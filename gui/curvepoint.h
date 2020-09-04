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
    void setResetPos(QPointF resetPoint);
    void reset();
protected:
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void wheelEvent(QGraphicsSceneWheelEvent *event) override;

signals:
    void pointPositionChanged(CurvePoint *point);
    void pointSlopeChanged(int delta);

private:
    QBrush normalBrush, lightBrush;
    QPointF sceneResetPoint = QPointF(0, 0);
    int wheelDeltaSum = 0;
};

#endif // CURVEPOINT_H
