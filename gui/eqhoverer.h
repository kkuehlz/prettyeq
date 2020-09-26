#ifndef EQHOVERER_H
#define EQHOVERER_H

#include "curvepoint.h"
#include "filtercurve.h"
#include <QGraphicsItem>
#include <QObject>

class CollisionManager;

class EqHoverer : public QObject, public QGraphicsItem
{
    Q_OBJECT
    Q_INTERFACES(QGraphicsItem)
public:
    explicit EqHoverer(CollisionManager *mgr, FilterCurve *curve, CurvePoint *point, QObject *parent = nullptr);
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    enum { Type  = UserType + 1 };
    int type() const override;

    void collisionStateChanged();
    void contextMenuToggle(bool on = false);
    void reset();
protected:
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;
private:
    void maybeShowPoint();

public slots:
    void resync(FilterCurve *curve);

signals:

public:
    FilterCurve *curve;
    CurvePoint *point;

private:
    unsigned int pointState;
    CollisionManager *mgr;
};

#endif // EQHOVERER_H
