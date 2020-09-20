#include "eqhoverer.h"
#include <QGraphicsScene>
#include <QDebug>
#include <QGraphicsSceneHoverEvent>
#include <QPainter>

static const unsigned int Default     = 1;
static const unsigned int Collision   = 1 << 1;
static const unsigned int Hover       = 1 << 2;
static const unsigned int ContextMenu = 1 << 3;

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
        EqHoverer *cc = qgraphicsitem_cast<EqHoverer*>(i);
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

int EqHoverer::type() const
{
    /* Make this type work with qgraphicsitem_cast */
    return Type;
}

void EqHoverer::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
#if 0
    qDebug() << "hoverEnterEvent();";
#endif
    pointState |= Hover;
    maybeShowPoint();
}

void EqHoverer::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
#if 0
    qDebug() << "hoverLeaveEvent();";
#endif
    pointState &= ~Hover;
    maybeShowPoint();
}

void EqHoverer::contextMenuToggle(bool on) {
    int contextBit = (ContextMenu & static_cast<int>(on));
    pointState |= contextBit;
    pointState &= (~ContextMenu | contextBit);
    maybeShowPoint();
}

void EqHoverer::reset()
{
    /* Order of these calls *does* matter here because
     * resetting the point signals resync. */
    point->reset();
    curve->reset();
    pointState |= Default;
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
