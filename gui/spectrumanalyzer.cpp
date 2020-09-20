#include "macro.h"
#include "prettyshim.h"
#include "spectrumanalyzer.h"

#include <QDebug>
#include <QPainter>
#include <QGraphicsScene>
#include <complex>

#define FFT_SAMPLE_TO_FREQ(NUM_SAMPLES, SAMPLE_INDEX) (44100*(SAMPLE_INDEX)/(NUM_SAMPLES))
#define FFT_MAGIC_MAX 10000

SpectrumAnalyzer::SpectrumAnalyzer()
{
    setCacheMode(QGraphicsItem::NoCache);
    setZValue(-1);
}

QRectF SpectrumAnalyzer::boundingRect() const
{
    auto x = scene()->sceneRect().width();
    auto y = scene()->sceneRect().height();
    return QRectF(0, 0, x, y / 2);
}

void SpectrumAnalyzer::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    painter->setRenderHint(QPainter::Antialiasing, true);
    auto gpen = QPen(Qt::red);
    gpen.setWidth(5);
    painter->setPen(gpen);
    {
        qreal max_psd_moving_avg = 0.0;
        auto &buf = max_psds.buffer();
        for (auto it = buf.begin(); it != buf.end(); it++)
            max_psd_moving_avg += *it;
        max_psd_moving_avg /= MOVING_AVG_PERIOD;

        unsigned int N;

        auto data = PrettyShim::getInstance().get_audio_data(&N);
        qreal max_psd = 0;
        for (unsigned int i = 0; i < N / 2; i++) {
            qreal psd = (qreal) std::norm(data[i]);
            if (psd > max_psd)
                max_psd = psd;
            qreal y = LINEAR_REMAP(psd, 0, max_psd_moving_avg, boundingRect().bottom(), boundingRect().top());
            painter->drawPoint(200, y);
        }
        max_psds.append(max_psd);

        PrettyShim::getInstance().release_audio_data();
    }
}
