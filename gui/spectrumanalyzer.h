#ifndef SPECTRUMANALYZER_H
#define SPECTRUMANALYZER_H

#include <QGraphicsItem>
#include "ringbuffer.h"

#define MOVING_AVG_PERIOD 128

class SpectrumAnalyzer : public QGraphicsItem
{
public:
    SpectrumAnalyzer();

public:
    QRectF boundingRect() const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

private:
    RingBuffer<qreal, MOVING_AVG_PERIOD> max_psds;
};

#endif // SPECTRUMANALYZER_H
