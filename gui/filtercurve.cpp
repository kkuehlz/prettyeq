#include "filtercurve.h"
#include <QGraphicsScene>

FilterCurve::FilterCurve(QPen togglePen, QBrush toggleBrush, bool guiOnly) : filter(nullptr)
{
    colorState = 0;
    pens[0] = defaultPen;
    pens[1] = togglePen;
    brushes[0] = defaultBrush;
    brushes[1] = toggleBrush;

    if (!guiOnly) {
        /* initialize the filter */
        PrettyShim::getInstance().new_filter(&filter);
        PrettyShim::getInstance().set_peaking_eq(filter, 100, 100, 0);
    }
}

FilterCurve::~FilterCurve()
{

}

void FilterCurve::setColorState(ColorState state)
{
    colorState = state;
    this->update();
}

QPen FilterCurve::getActivePen() const
{
    return pens[colorState];
}

QBrush FilterCurve::getActiveBrush() const
{
    return brushes[colorState];
}
