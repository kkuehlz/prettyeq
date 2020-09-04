#include "curvepoint.h"
#include "eqhoverer.h"
#include "prettygraphicsscene.h"
#include <QDebug>
#include <QGraphicsItem>
#include <QGraphicsSceneContextMenuEvent>
#include <QMenu>

PrettyGraphicsScene::PrettyGraphicsScene(QObject *parent) : QGraphicsScene(parent) {}

void PrettyGraphicsScene::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
    CurvePoint *point = nullptr;
    for (auto item : items()) {
        point = qgraphicsitem_cast<CurvePoint*>(item);
        if ( !point)
            continue;

        if (point->sceneBoundingRect().contains(event->scenePos()))
            break;
    }

    if (point) {
        EqHoverer *hover;
        for (auto item : items()) {
            hover = qgraphicsitem_cast<EqHoverer*>(item);
            if (! hover)
                continue;

            if (point == hover->point) {
                hover->contextMenuToggle(true);
                break;
            }
        }

        Q_ASSERT(hover);
        QMenu menu(event->widget());
        menu.addAction("Reset");
        if (menu.exec(event->screenPos()))
            hover->reset();

        for (auto item : items()) {
            EqHoverer *hover = qgraphicsitem_cast<EqHoverer*>(item);
            if (! hover)
                continue;

            if (point == hover->point) {
                hover->contextMenuToggle(false);
                break;
            }
        }
    }
}
