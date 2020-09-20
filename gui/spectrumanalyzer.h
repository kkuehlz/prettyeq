#ifndef SPECTRUMANALYZER_H
#define SPECTRUMANALYZER_H

#include <QGraphicsItem>
#include "ringbuffer.h"

#define MOVING_AVG_PERIOD 128

class FrequencyTickBuilder;

class SpectrumAnalyzer : public QGraphicsItem
{
public:
    SpectrumAnalyzer(FrequencyTickBuilder *xTickBuilder);

public:
    QRectF boundingRect() const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
private:
    QPointF pointForSample(qreal frequency, qreal max_psd, qreal max_psd_moving_avg);

private:
    FrequencyTickBuilder *xTickBuilder;
    RingBuffer<qreal, MOVING_AVG_PERIOD> max_psds;
    QPainterPath path;
    qreal last_psds[4096];
};

#endif // SPECTRUMANALYZER_H
