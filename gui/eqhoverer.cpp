#include "eqhoverer.h"
#include <QGraphicsScene>
#include <QDebug>
#include <QGraphicsSceneHoverEvent>
#include <QPainter>

static const unsigned int Default   = 1;
static const unsigned int Collision = 1 << 1;
static const unsigned int Hover     = 1 << 2;

EqHoverer::EqHoverer(FilterCurve *curve, CurvePoint *point, QObject *parent) : QObject(parent), curve(curve), point(point), pointState(0)
{
    Q_ASSERT(curve);
    Q_ASSERT(point);
    setAcceptHoverEvents(true);
    pointState |= Default;
}


QRectF EqHoverer::boundingRect() const
{
    QRectF r = curve->boundingRect();
    if (scene()) {
        r.setY(scene()->sceneRect().y());
        r.setHeight(scene()->height());
        return r;
    } else
        return QRectF(QPointF(0, 0), QPointF(0, 0));
}

void EqHoverer::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
#if 0
    // debugging boundingRect()
    QPen wpen(Qt::white);
    wpen.setWidth(3);
    painter->save();
    painter->setPen(wpen);
    painter->drawRect(boundingRect());
    painter->restore();
#endif

    bool nocollide = true;
    for(QGraphicsItem *i : collidingItems()) {
        EqHoverer *cc = dynamic_cast<EqHoverer*>(i);
        if (cc && cc != this) {
            if (this->isUnderMouse() || cc->isUnderMouse()) {
                pointState |= Collision;
                maybeShowPoint();
                nocollide = false;
            }
        }
    }
    if (nocollide) {
        pointState &= ~Collision;
        maybeShowPoint();
    }
}

void EqHoverer::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
#if 0
    qDebug() << "hoverEnterEvent();";
#endif
    curve->setColorState(PrettyState);
    pointState |= Hover;
    maybeShowPoint();
}

void EqHoverer::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
#if 0
    qDebug() << "hoverLeaveEvent();";
#endif
    curve->setColorState(DefaultState);
    pointState &= ~Hover;
    maybeShowPoint();
}

void EqHoverer::maybeShowPoint()
{
    if (pointState == 0) {
        point->hide();
        curve->setColorState(DefaultState);
    } else {
        point->show();
        curve->setColorState(PrettyState);
    }
}

void EqHoverer::resync(FilterCurve *curve)
{
    pointState &= ~Default;
    prepareGeometryChange();
}
