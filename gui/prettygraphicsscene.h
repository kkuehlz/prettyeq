#ifndef PRETTYGRAPHICSSCENE_H
#define PRETTYGRAPHICSSCENE_H

#include <QGraphicsScene>

class PrettyGraphicsScene : public QGraphicsScene
{
public:
    PrettyGraphicsScene(QObject *parent = nullptr);

protected:
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event) override;
};

#endif // PRETTYGRAPHICSSCENE_H
