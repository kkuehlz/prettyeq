#include "collisionmanager.h"
#include "eqhoverer.h"
#include "filtercurve.h"

#include <QDebug>

CollisionManager::CollisionManager() : numItems(0)
{

}

void CollisionManager::addEqHoverer(EqHoverer *hover)
{
    Q_ASSERT(hover);
    hoverItems[numItems++] = hover;
}

void CollisionManager::notifyFriends()
{
    Q_ASSERT(numItems == NUM_FILTERS);

    for (int i = 0; i < numItems; i++)
        hoverItems[i]->collisionStateChanged();
}
