#ifndef LOWSHELFCURVE_H
#define LOWSHELFCURVE_H

#include "shelfcurve.h"
#include <QBrush>
#include <QPen>

class LowShelfCurve : public ShelfCurve
{
public:
    explicit LowShelfCurve(QPen pen, QBrush brush, bool guiOnly = false, QObject *parent = nullptr);
    ~LowShelfCurve();

protected:
    QPointF clampP2() const override;

public slots:
    void pointSlopeChanged(int delta) override;

    // FilterCurve interface
public:
    void reset() override;
};

#endif // LOWSHELFCURVE_H
