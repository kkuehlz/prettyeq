#ifndef PRETTYSHIM_H
#define PRETTYSHIM_H

#include <QObject>
#include <QString>
#include <QDebug>

#include "pretty.h"

typedef PrettyFilter* ShimFilterPtr;

/*  The shim is for making C callbacks idiomatic with Qt signals/slots  */

class PrettyShim : public QObject
{
    Q_OBJECT

public:
    static PrettyShim& getInstance() {
        static PrettyShim instance;
        return instance;
    }

private:
    explicit PrettyShim(QObject *parent = nullptr) {}
    static PrettyShim *instance;

public:
    PrettyShim(PrettyShim const&) 	  = delete;
    void operator=(PrettyShim const&) = delete;

    void init() {
        qRegisterMetaType<uint32_t>("uint32_t");
        int r = pretty_init();
        Q_ASSERT(r == 0);
    }

    void exit() {
        pretty_exit();
    }

    void new_filter(ShimFilterPtr *filter) {
        int r = pretty_new_filter(filter);
        Q_ASSERT(*filter && r >= 0);
    }

    void set_peaking_eq(ShimFilterPtr filter, float f0, float bandwidth, float db_gain) {
        pretty_set_peaking_eq(filter, f0, bandwidth, db_gain);
    }

    void set_low_shelf(PrettyFilter *filter, float f0, float S, float db_gain) {
        pretty_set_low_shelf(filter, f0, S, db_gain);
    }

    void set_high_shelf(PrettyFilter *filter, float f0, float S, float db_gain) {
        pretty_set_high_shelf(filter, f0, S, db_gain);
    }

};

#endif // PRETTYSHIM_H
