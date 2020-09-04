#ifndef HIGHSHELFCURVE_H
#define HIGHSHELFCURVE_H

#include "shelfcurve.h"

class HighShelfCurve : public ShelfCurve
{
public:
    explicit HighShelfCurve(QPen pen, QBrush brush, bool guiOnly = false, QObject *parent = nullptr);
    ~HighShelfCurve();

// ShelfCurve interface
protected:
    QPointF clampP2() const override;

    // ShelfCurve interface
public slots:
    void pointSlopeChanged(int delta) override;

    // FilterCurve interface
public:
    void reset() override;
};

#endif // HIGHSHELFCURVE_H
