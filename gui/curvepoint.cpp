#include "curvepoint.h"
#include <QDebug>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#define RADIUS 40
#define ESCALE 0.60

CurvePoint::CurvePoint(QBrush normalBrush, QBrush lightBrush, QObject *parent)
    : QObject(parent),
      normalBrush(normalBrush),
      lightBrush(lightBrush)
{
    setZValue(100001);
    hide();
    setFlags(GraphicsItemFlag::ItemIsSelectable | GraphicsItemFlag::ItemIsMovable);
    lastPos = pos();
}

QRectF CurvePoint::boundingRect() const
{
    return QRectF(-RADIUS/2, -RADIUS/2, RADIUS, RADIUS);
}

void CurvePoint::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setPen(Qt::NoPen);

    /* outer circle */
    painter->setBrush(lightBrush);
    painter->drawEllipse(-RADIUS/2, -RADIUS/2, RADIUS, RADIUS);

    /* inner circle */
    painter->scale(ESCALE, ESCALE);
    painter->setBrush(normalBrush);
    painter->drawEllipse(-RADIUS/2, -RADIUS/2, RADIUS, RADIUS);

}

void CurvePoint::setPos(const QPointF &pos)
{
    QGraphicsItem::setPos(pos);
    lastPos = pos;
}

void CurvePoint::setPos(qreal x, qreal y)
{
    QGraphicsItem::setPos(x, y);
    lastPos = QPointF(x, y);
}

void CurvePoint::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    QGraphicsItem::mouseMoveEvent(event);
    if (scene() && event->type() == QGraphicsSceneMouseEvent::GraphicsSceneMouseMove) {
        QPointF delta = pos() - lastPos;

        /* x bounds detection */
        if (scene()->sceneRect().x() >= mapToScene(boundingRect().center()).x()) {
            setPos(scene()->sceneRect().x(), pos().y());
            delta.setX(0);
        } else if (scene()->sceneRect().x() + scene()->width() <= mapToScene(boundingRect().center()).x()) {
            setPos(scene()->sceneRect().x() + scene()->width(), pos().y());
            delta.setX(0);
        }

        /* y bounds detection */
        if (scene()->sceneRect().y() >= mapToScene(boundingRect().center()).y()) {
            setPos(pos().x(), scene()->sceneRect().y());
            delta.setY(0);
        } else if (scene()->sceneRect().y() + scene()->height() <= mapToScene(boundingRect().center()).y()) {
            setPos(pos().x(), scene()->sceneRect().y() + scene()->sceneRect().height());
            delta.setY(0);
        }

        lastPos = pos();
        emit pointPositionChanged(delta.x(), delta.y());
    }
}

void CurvePoint::wheelEvent(QGraphicsSceneWheelEvent *event)
{
    QGraphicsItem::wheelEvent(event);
    wheelDeltaSum += event->delta();
    if (wheelDeltaSum >= 120) {
        wheelDeltaSum = 0;
        emit pointSlopeChanged(-1);
    } else if (wheelDeltaSum <= -120) {
        wheelDeltaSum = 0;
        emit pointSlopeChanged(1);
    }
}

void CurvePoint::resync(FilterCurve *curve)
{
    setPos(curve->controlPoint());
    this->update();
}
