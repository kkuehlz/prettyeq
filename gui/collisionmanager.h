#ifndef COLLISIONMANAGER_H
#define COLLISIONMANAGER_H

#define NUM_FILTERS 7

class EqHoverer;
class FilterCurve;

class CollisionManager
{
public:
    explicit CollisionManager();
    void addEqHoverer(EqHoverer *hover);
    void notifyFriends();

private:
    int numItems;
    EqHoverer *hoverItems[NUM_FILTERS];
};

#endif // COLLISIONMANAGER_H
