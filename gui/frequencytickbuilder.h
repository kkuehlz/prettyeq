#ifndef FREQUENCYTICKBUILDER_H
#define FREQUENCYTICKBUILDER_H

#include <QtCore>
#include <QGraphicsScene>
#define NUM_TICKS 10

class FrequencyTick;

class FrequencyTickBuilder
{
public:
    FrequencyTickBuilder(QGraphicsScene *scene, int width, int xmin, int xmax, int ymin, int ymax);
    ~FrequencyTickBuilder();
    qreal lerpTick(qreal x);
    qreal unlerpTick(qreal f);
private:
    FrequencyTick *tick[NUM_TICKS];
};

#endif // FREQUENCYTICKBUILDER_H
