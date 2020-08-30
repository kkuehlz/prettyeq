#ifndef FREQUENCYTICK_H
#define FREQUENCYTICK_H

#include <QGraphicsScene>
#include <QGraphicsLineItem>

class FrequencyTick
{
public:
    FrequencyTick(QGraphicsScene *scene, int x, int y0, int y1, int frequency);
    ~FrequencyTick();
    QString toQString();
    qreal getFrequency() const;
    qreal getX() const;

private:
    QGraphicsScene *scene;
    int x, y0, y1;
    int frequency;
    QGraphicsLineItem *line;
    QGraphicsTextItem *text;
};

#endif // FREQUENCYTICK_H
