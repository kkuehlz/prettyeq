#ifndef FILTERCURVE_H
#define FILTERCURVE_H

#include "prettyshim.h"

#include <QGraphicsItem>
#include <QObject>
#include <QPen>
#include <QBrush>

static QPen defaultPen = Qt::NoPen;
static QBrush defaultBrush = QBrush(QColor(73, 137, 196, 255));

typedef enum ColorState {
    DefaultState = 0,
    PrettyState,
} ColorState;

class FilterCurve : public QGraphicsItem
{
public:
    explicit FilterCurve(QPen togglePen, QBrush toggleBrush, bool guiOnly = false);
    virtual ~FilterCurve();
    virtual QPointF controlPoint() const = 0;
    void setColorState(ColorState state);

signals:

protected:
    QPen getActivePen() const;
    QBrush getActiveBrush() const;

protected:
    ShimFilterPtr filter;

private:
    QPen pens[2];
    QBrush brushes[2];
    int colorState;
};

#endif // FILTERCURVE_H
