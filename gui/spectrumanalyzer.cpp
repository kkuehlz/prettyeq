#include "frequencytickbuilder.h"
#include "macro.h"
#include "prettyshim.h"
#include "spectrumanalyzer.h"

#include <QDebug>
#include <QGraphicsScene>
#include <QPainter>
#include <QPainterPath>
#include <complex>
#include <string.h>
#include <QtCore>

#define FFT_SAMPLE_TO_FREQ(NUM_SAMPLES, SAMPLE_INDEX) (44100*(SAMPLE_INDEX)/(NUM_SAMPLES))
#define FFT_FREQ_TO_SAMPLE(NUM_SAMPLES, FREQ) ((int)roundf((FREQ)*(NUM_SAMPLES)/44100))
#define FFT_BUCKET_WIDTH(NUM_SAMPLES) (44100/(NUM_SAMPLES))

SpectrumAnalyzer::SpectrumAnalyzer(FrequencyTickBuilder *xTickBuilder) : xTickBuilder(xTickBuilder)
{
    setZValue(-1);
    for (unsigned int i = 0; i < 4096; i++)
        last_psds[i] = 0.0;
    path.reserve(4096);
}

QRectF SpectrumAnalyzer::boundingRect() const
{
    auto x = scene()->sceneRect().width();
    auto y = scene()->sceneRect().height();
    return QRectF(0, 0, x, y / 4);
}

inline QPointF SpectrumAnalyzer::pointForSample(qreal frequency, qreal max_psd, qreal max_psd_moving_avg) {
    qreal sceneX = xTickBuilder->unlerpTick(frequency);
    qreal startX = mapFromScene(sceneX, 0xbeefcafe).x();
    qreal startY = qMax(boundingRect().top(), LINEAR_REMAP(max_psd, 0, max_psd_moving_avg, boundingRect().bottom(), boundingRect().top()));
    return QPointF(startX, qRound(startY));
}

void SpectrumAnalyzer::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    path.clear();
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setRenderHint(QPainter::SmoothPixmapTransform, true);
#if 0
    QPen rpen(Qt::red);
    rpen.setWidth(5);
    painter->setPen(rpen);
    painter->drawPoint(boundingRect().center());
    painter->drawRect(boundingRect());
#else
    qreal max_psd_moving_avg = 0.0;
    auto pen = QPen(QColor(127, 153, 176));
    pen.setWidth(3);
    painter->setPen(pen);
    {
        auto &buf = max_psds.buffer();
        for (auto it = buf.begin(); it != buf.end(); it++)
            max_psd_moving_avg += *it;
        max_psd_moving_avg /= MOVING_AVG_PERIOD;
    }

    {
        unsigned int N;
        auto data = PrettyShim::getInstance().get_audio_data(&N);

        int start_sample = FFT_FREQ_TO_SAMPLE(N, FMIN);
        qreal max_psd = (std::conj(data[start_sample]) * data[start_sample]).real() / N;
        QPointF start_point = pointForSample(FMIN, max_psd, max_psd_moving_avg);
        path.moveTo(start_point);

        for (unsigned int i = start_sample + 1; i < N / 2; i++) {
            qreal psd = (qreal) std::abs(data[i]);
            max_psd = qMax(max_psd, psd);
            qreal frequency = FFT_SAMPLE_TO_FREQ(N, (qreal) i);
            QPointF next_point = pointForSample(frequency, psd, max_psd_moving_avg);
            path.lineTo(next_point);
            path.moveTo(next_point);
        }

        if (max_psd_moving_avg > 0.01) {
            path.closeSubpath();
            painter->drawPath(path);
        }

        PrettyShim::getInstance().release_audio_data();
        max_psds.append(max_psd);
    }
#endif
}
