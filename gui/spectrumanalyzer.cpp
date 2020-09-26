#include "frequencytickbuilder.h"
#include "macro.h"
#include "prettyshim.h"
#include "spectrumanalyzer.h"

#include <QDebug>
#include <QGraphicsScene>
#include <QPainter>
#include <QPainterPath>
#include <QtCore>
#include <complex>
#include <string.h>

#define FFT_SAMPLE_TO_FREQ(NUM_SAMPLES, SAMPLE_INDEX) (44100*(SAMPLE_INDEX)/(NUM_SAMPLES))
#define FFT_FREQ_TO_SAMPLE(NUM_SAMPLES, FREQ) ((int)roundf((FREQ)*(NUM_SAMPLES)/44100))
#define FFT_BUCKET_WIDTH(NUM_SAMPLES) (44100/(NUM_SAMPLES))

static inline qreal dampen(qreal start, qreal end, qreal smoothing_factor, qint64 dt) {
    return LERP(1 - qPow(smoothing_factor, dt), start, end);
}

SpectrumAnalyzer::SpectrumAnalyzer(FrequencyTickBuilder *xTickBuilder) : xTickBuilder(xTickBuilder), last_frame_time(0)
{
    setZValue(-1);
    for (unsigned int i = 0; i < MAX_SAMPLES; i++)
        last_psds[i] = 0.0;
}

QRectF SpectrumAnalyzer::boundingRect() const
{
    auto x = scene()->sceneRect().width();
    auto y = scene()->sceneRect().height();
    return QRectF(0, 0, x, y / 4);
}

inline QLineF SpectrumAnalyzer::pointForSample(qreal frequency, qreal max_psd, qreal max_psd_moving_avg) {
    qreal sceneX = xTickBuilder->unlerpTick(frequency);
    qreal startX = mapFromScene(sceneX, 0xbeefcafe).x();
    qreal startY = qMax(boundingRect().top(), LINEAR_REMAP(max_psd, 0, max_psd_moving_avg, 0, boundingRect().height() / 2));
    QLineF line(QPointF(startX, boundingRect().center().y()), QPointF(startX, boundingRect().center().y() - startY));
    return line;
}

qint64 SpectrumAnalyzer::frame_dt()
{
    return QDateTime::currentMSecsSinceEpoch() - last_frame_time;
}

void SpectrumAnalyzer::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    painter->setRenderHint(QPainter::Antialiasing, true);
#if 0
    QPen rpen(Qt::red);
    rpen.setWidth(5);
    QPen bpen(Qt::blue);
    bpen.setWidth(5);
    painter->setPen(rpen);
    painter->drawPoint(boundingRect().center());
    painter->drawRect(boundingRect());
    painter->drawPoint(boundingRect().topLeft());
    auto line = QLineF(QPointF(100, 0), QPointF(100, boundingRect().height() / 2));
    painter->drawLine(line);
    line.translate(0, boundingRect().height() / 2);
    painter->setPen(bpen);
    painter->drawLine(line);
#else
    qreal max_psd_moving_avg = 0.0;
    static auto brush = QBrush(QColor(127, 153, 176, 128));
    static auto pen = QPen(brush, 3);
    painter->setPen(pen);
    {
        auto &buf = max_psds.buffer();
        for (auto it = buf.begin(); it != buf.end(); it++)
            max_psd_moving_avg += *it;
        max_psd_moving_avg /= MOVING_AVG_PERIOD;
    }

    {
        qint64 delta = frame_dt();
        unsigned int N;
        auto data = PrettyShim::getInstance().get_audio_data(&N);
        Q_ASSERT(N < MAX_SAMPLES);
        qreal max_psd = 0.0;
        for (unsigned int i = FFT_FREQ_TO_SAMPLE(N, FMIN); i < N / 2; i++) {
            qreal raw_psd = (qreal) std::abs(data[i]);
            qreal min = qMin(raw_psd, last_psds[i]);
            qreal max = qMax(raw_psd, last_psds[i]);
            qreal dampening_factor = 1 - min/max;
            dampening_factor = qMax(0.5, dampening_factor);
            dampening_factor = qMin(0.95, dampening_factor);
            qreal smoothed_psd = dampen(raw_psd, last_psds[i], dampening_factor, delta);
            last_psds[i] = smoothed_psd;
            max_psd = qMax(max_psd, smoothed_psd);
            qreal frequency = FFT_SAMPLE_TO_FREQ(N, (qreal) i);
            qreal sceneX = xTickBuilder->unlerpTick(frequency);
            qreal startX = mapFromScene(sceneX, 0xbeefcafe).x();
            qreal startY = LINEAR_REMAP(smoothed_psd, 0, max_psd_moving_avg, 0, boundingRect().height() / 2);
            startY = qMin(startY, boundingRect().height() / 2);
            QPointF p1 = QPointF(startX, boundingRect().center().y() - startY);
            QPointF p2 = QPointF(startX, boundingRect().center().y() + startY);
            lines[i].setP1(p1);
            lines[i].setP2(p2);
        }

        if (max_psd_moving_avg > 0.01)
            painter->drawLines(lines, N / 2);

        PrettyShim::getInstance().release_audio_data();
        max_psds.append(max_psd);
    }
#endif
}

void SpectrumAnalyzer::updateFrameDelta()
{
    last_frame_time = QDateTime::currentMSecsSinceEpoch();
}
