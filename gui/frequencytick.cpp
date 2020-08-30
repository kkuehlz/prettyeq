#include "frequencytick.h"
#include "macro.h"

#include <QDebug>
#include <QFont>
#include <QFontDatabase>
#include <QPainter>

FrequencyTick::FrequencyTick(QGraphicsScene *scene, int x, int y0, int y1, int frequency) :
    scene(scene), x(x), y0(y0), y1(y1), frequency(frequency) {
    QFont ff = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    ff.setPixelSize(20);

    /* Setup line */
    line = new QGraphicsLineItem(x, y0, x, y1);
    line->setPen(QPen(Qt::white));
    line->setOpacity(0.3);

    /* Setup text */
    text = new QGraphicsTextItem(toQString());
    text->setDefaultTextColor(Qt::white);
    text->setFont(ff);
    auto fm = QFontMetrics(text->font());
    if (frequency == FMAX) {
        int offset = fm.horizontalAdvance(QLatin1Char('K')) * 4;
        text->setPos(x - offset, 30);
    } else if (frequency == FMIN) {
        text->setPos(x, 30);
    } else {
        int offset = fm.horizontalAdvance(QLatin1Char('K')) * toQString().length() + 7;
        text->setPos(x - offset / 2, 30);
    }
    text->setZValue(1000);

    scene->addItem(text);
    scene->addItem(line);
}

FrequencyTick::~FrequencyTick()
{
    delete line;
    delete text;
}

QString FrequencyTick::toQString()
{
    if (frequency >= 1000)
        return QString::number(frequency / 1000) + QString("K");
    else
        return QString::number(frequency);
}

qreal FrequencyTick::getFrequency() const
{
    return static_cast<qreal>(frequency);
}

qreal FrequencyTick::getX() const
{
    return x;
}
