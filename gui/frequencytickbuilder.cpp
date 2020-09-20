#include "frequencytick.h"
#include "frequencytickbuilder.h"
#include "macro.h"

#include <QGraphicsScene>

FrequencyTickBuilder::FrequencyTickBuilder(QGraphicsScene *scene, int width, int xmin, int xmax, int ymin, int ymax)
{
    /* y-axis frequency markers */
    int tickWidth = width / (NUM_TICKS - 1);
    tick[0] = new FrequencyTick(scene, xmin,                 ymin, ymax, F1);
    tick[1] = new FrequencyTick(scene, xmin + tickWidth * 1, ymin, ymax, F2);
    tick[2] = new FrequencyTick(scene, xmin + tickWidth * 2, ymin, ymax, F3);
    tick[3] = new FrequencyTick(scene, xmin + tickWidth * 3, ymin, ymax, F4);
    tick[4] = new FrequencyTick(scene, xmin + tickWidth * 4, ymin, ymax, F5);
    tick[5] = new FrequencyTick(scene, xmin + tickWidth * 5, ymin, ymax, F6);
    tick[6] = new FrequencyTick(scene, xmin + tickWidth * 6, ymin, ymax, F7);
    tick[7] = new FrequencyTick(scene, xmin + tickWidth * 7, ymin, ymax, F8);
    tick[8] = new FrequencyTick(scene, xmin + tickWidth * 8, ymin, ymax, F9);
    tick[9] = new FrequencyTick(scene, xmax,                 ymin, ymax, F10);
}

qreal FrequencyTickBuilder::lerpTick(qreal x)
{
    FrequencyTick *tp, *tq;
    for (int i = 1; i < NUM_TICKS; i++) {
        tp = tick[i-1];
        tq = tick[i];
        if (x >= tp->getX() && x <= tq->getX())
            break;
    }
    return LINEAR_REMAP(x, tp->getX(), tq->getX(), tp->getFrequency(), tq->getFrequency());
}

qreal FrequencyTickBuilder::unlerpTick(qreal f)
{
    FrequencyTick *tp, *tq;
    for (int i = 1; i < NUM_TICKS; i++) {
        tp = tick[i-1];
        tq = tick[i];
        if (f >= tp->getFrequency() && f <= tq->getFrequency())
            break;
    }
    return LINEAR_REMAP(f, tp->getFrequency(), tq->getFrequency(), tp->getX(), tq->getX());
}

FrequencyTickBuilder::~FrequencyTickBuilder()
{
    for (int i = 0; i < NUM_TICKS; i++)
        delete tick[i];
}
