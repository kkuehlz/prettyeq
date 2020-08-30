#ifndef MACRO_H
#define MACRO_H

#include <QPointF>
#define F1   20
#define F2   50
#define F3   100
#define F4   200
#define F5   500
#define F6   1000
#define F7   2000
#define F8   5000
#define F9   10000
#define F10  20000
#define FMIN F1
#define FMAX F10

#define UNLERP( v, min, max ) ( ( (v) - (min) ) / ( (max) - (min) ) )

#define LERP( n, min, max ) ( (min) + (n) * ( (max) - (min) ) )

#define LINEAR_REMAP( i, imin, imax, omin, omax ) ( LERP( UNLERP( i, imin, imax ), omin, omax ) )

static inline QPointF cubic_bezier(qreal t, QPointF p0, QPointF p1, QPointF p2, QPointF p3) {
    return (1-t)*(1-t)*(1-t)*p0 +
            3*(1-t)*(1-t)*t*p1 +
            3*(1-t)*t*t*p2 +
            t*t*t*p3;
}

#define CUBIC_BEZIER cubic_bezier

#endif // MACRO_H
