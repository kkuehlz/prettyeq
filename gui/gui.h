#ifndef GUI_H
#define GUI_H

#include "prettyshim.h"
#include <QDialog>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QMenu>
#include <QMetaType>
#include <QSystemTrayIcon>

#define DB_GAIN_MAX 12
#define NUM_FILTERS 7

QT_BEGIN_NAMESPACE
namespace Ui { class Gui; }
QT_END_NAMESPACE

class CollisionManager;
class CurvePoint;
class EqHoverer;
class FilterCurve;
class FrequencyTick;
class FrequencyTickBuilder;
class PeakingCurve;
class ShelfCurve;
class SpectrumAnalyzer;

typedef struct FilterItem {
    QGraphicsItemGroup *group;
    FilterCurve *curve;
    CurvePoint *point;
    EqHoverer *hover;
} FilterItem;

class Gui : public QDialog
{
    Q_OBJECT

public:
    Gui(QWidget *parent = nullptr);
    ~Gui();

/* Equalizer slots */
public slots:
    void cleanup();
    void peakingFilterParamsChanged(ShimFilterPtr filter, PeakingCurve *curve);
    void lowshelfFilterParamsChanged(ShimFilterPtr filter, ShelfCurve *curve);
    void highshelfFilterParamsChanged(ShimFilterPtr filter, ShelfCurve *curve);

/* Menu Bar slots */
private slots:
    void on_actionQuit_triggered();

/* System Tray slots */
private slots:
    void trayActivated(QSystemTrayIcon::ActivationReason reason);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;


private:
    void addFilterItem(QGraphicsItemGroup *group, FilterCurve *curve, CurvePoint *point, EqHoverer *hover);
    qreal lerpTick(qreal x);
    qreal unlerpTick(qreal f);
    void addLowShelf(QPen curvePen, QBrush filterBrush, QBrush innerRadiusBrush, QBrush outerRadiusBrush);
    void addHighShelf(QPen curvePen, QBrush filterBrush, QBrush innerRadiusBrush, QBrush outerRadiusBrush);
    void addPeakingEq(int frequency, QPen curvePen, QBrush filterBrush, QBrush innerRadiusBrush, QBrush outerRadiusBrush);
    void connectBypassButton();
    void addSpectrumAnalyzer();

    void maybeShowInSystemTray();

private:
    Ui::Gui *ui;
    QGraphicsScene *scene;
    CollisionManager *collisionMgr = nullptr;;
    FilterItem items[NUM_FILTERS];
    SpectrumAnalyzer *spectrumAnalyzer = nullptr;
    FrequencyTickBuilder *xTickBuilder = nullptr;
    QTimer *spectrumUpdateTimer = nullptr;
    int itemCount = 0;

    /* System tray stuff. */
    QMenu *trayMenu;
    QAction *quitAct;
    QSystemTrayIcon *trayIcon;
};
#endif // GUI_H
