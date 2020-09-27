#include "collisionmanager.h"
#include "curvepoint.h"
#include "eqhoverer.h"
#include "frequencytick.h"
#include "frequencytickbuilder.h"
#include "gui.h"
#include "highshelfcurve.h"
#include "lowshelfcurve.h"
#include "macro.h"
#include "peakingcurve.h"
#include "prettygraphicsscene.h"
#include "prettyshim.h"
#include "spectrumanalyzer.h"
#include "ui_gui.h"

#include <QBrush>
#include <QComboBox>
#include <QDebug>
#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include <QPainter>
#include <QPen>
#include <QRadialGradient>
#include <QResizeEvent>
#include <QScrollBar>
#include <QSystemTrayIcon>
#include <QtMath>

#define WIDTH 2000
#define HEIGHT 1000
#define XMIN -1000
#define YMIN -500
#define XMAX (XMIN + WIDTH)
#define YMAX (YMIN + HEIGHT)

static QPen GreenFilterPen = QPen(QColor(138, 237, 152), 3);
static QBrush GreenFilterBrush = QBrush(QColor(31, 204, 57, 127));
static QBrush GreenInnerRadiusBrush = QBrush(QColor(31, 204, 57));
static QBrush GreenOuterRadiusBrush = QBrush(QColor(31, 204, 57, 50));

static QPen RedFilterPen = QPen(QColor(231, 123, 131), 3);
static QBrush RedFilterBrush = QBrush(QColor(236, 85, 95, 127));
static QBrush RedInnerRadiusBrush = QBrush(QColor(223, 59, 70));
static QBrush RedOuterRadiusBRush = QBrush(QColor(233, 59, 70, 50));

static QPen YellowFilterPen = QPen(QColor(231, 229, 123), 3);
static QBrush YellowFilterBrush = QBrush(QColor(236, 225, 85, 127));
static QBrush YellowInnerRadiusBrush = QBrush(QColor(236, 225, 85));
static QBrush YellowOuterRadiusBrush = QBrush(QColor(236, 225, 85, 50));

static QPen PinkFilterPen = QPen(QColor(231, 123, 217), 3);
static QBrush PinkFilterBrush = QBrush(QColor(236, 85, 216, 127));
static QBrush PinkInnerRadiusBrush = QBrush(QColor(236, 85, 216));
static QBrush PinkOuterRadiusBrush = QBrush(QColor(236, 85, 216, 50));

static QPen BlueFilterPen = QPen(QColor(123, 231, 191), 3);
static QBrush BlueFilterBrush = QBrush(QColor(85, 236, 193, 127));
static QBrush BlueInnerRadiusBrush = QBrush(QColor(85, 236, 193));
static QBrush BlueOuterRadiusBrush = QBrush(QColor(85, 236, 193, 50));

static QPen OrangeFilterPen = QPen(QColor(231, 175, 123), 3);
static QBrush OrangeFilterBrush = QBrush(QColor(236, 151, 85, 127));
static QBrush OrangeInnerRadiusBrush = QBrush(QColor(236, 151, 85));
static QBrush OrangeOuterRadiusBrush = QBrush(QColor(236, 151, 85, 50));

static QPen PurpleFilterPen = QPen(QColor(177, 123, 231), 3);
static QBrush PurpleFilterBrush = QBrush(QColor(159, 85, 236, 127));
static QBrush PurpleInnerRadiusBrush = QBrush(QColor(159, 85, 236));
static QBrush PurpleOuterRadiusBrush = QBrush(QColor(159, 85, 236, 50));

Gui::Gui(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Gui)
{

    PrettyShim::getInstance().init();
    PrettyShim::getInstance().setup_sink_io();
    ui->setupUi(this);

    QRadialGradient backgroundGradient(QPoint(0, 0), WIDTH);
    backgroundGradient.setSpread(QGradient::ReflectSpread);
    backgroundGradient.setColorAt(0.15, QColor(4, 38, 69));
    backgroundGradient.setColorAt(0.85, QColor(6, 17, 43));
    scene = new PrettyGraphicsScene(ui->graphicsView);
    scene->setSceneRect(XMIN, YMIN, WIDTH, HEIGHT);
    scene->setBackgroundBrush(QBrush(backgroundGradient));

    ui->graphicsView->setRenderHint(QPainter::Antialiasing);
    ui->graphicsView->setRenderHint(QPainter::TextAntialiasing);
    ui->graphicsView->setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
    ui->graphicsView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->graphicsView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->graphicsView->setResizeAnchor(QGraphicsView::AnchorViewCenter);
    ui->graphicsView->setScene(scene);

    /* x-axis (boost/cut) */
    auto xaxis = new QGraphicsLineItem(XMIN, scene->sceneRect().center().y(), XMAX, scene->sceneRect().center().y());
    xaxis->setPen(QPen(Qt::white));
    xaxis->setOpacity(0.3);
    scene->addItem(xaxis);

    /* x-axis frequency markers */
    xTickBuilder = new FrequencyTickBuilder(scene, WIDTH, XMIN, XMAX, YMIN, YMAX);
    addSpectrumAnalyzer();

    collisionMgr = new CollisionManager();

    addLowShelf (GreenFilterPen,  GreenFilterBrush,  GreenInnerRadiusBrush,  GreenOuterRadiusBrush);
    addHighShelf(OrangeFilterPen, OrangeFilterBrush, OrangeInnerRadiusBrush, OrangeOuterRadiusBrush);

    addPeakingEq(150,  RedFilterPen, 	RedFilterBrush, 	RedInnerRadiusBrush, 	RedOuterRadiusBRush);
    addPeakingEq(350,  YellowFilterPen, YellowFilterBrush, 	YellowInnerRadiusBrush, YellowOuterRadiusBrush);
    addPeakingEq(750,  PinkFilterPen, 	PinkFilterBrush, 	PinkInnerRadiusBrush, 	PinkOuterRadiusBrush);
    addPeakingEq(1500, BlueFilterPen, 	BlueFilterBrush, 	BlueInnerRadiusBrush, 	BlueOuterRadiusBrush);
    addPeakingEq(3500, PurpleFilterPen, PurpleFilterBrush, 	PurpleInnerRadiusBrush, PurpleOuterRadiusBrush);

    connectBypassButton();
    maybeShowInSystemTray();
}

//=============================================================================

void Gui::addFilterItem(QGraphicsItemGroup *group, FilterCurve *curve, CurvePoint *point, EqHoverer *hover)
{
    Q_ASSERT(itemCount < NUM_FILTERS);
    items[itemCount].group = group;
    items[itemCount].curve = curve;
    items[itemCount].point = point;
    items[itemCount].hover = hover;
    itemCount++;
}

void Gui::addPeakingEq(int frequency, QPen curvePen, QBrush filterBrush, QBrush innerRadiusBrush, QBrush outerRadiusBrush) {
    Q_ASSERT(collisionMgr);
    PeakingCurve *curve = new PeakingCurve(curvePen, filterBrush);
    CurvePoint *point = new CurvePoint(innerRadiusBrush, outerRadiusBrush);
    EqHoverer *hover = new EqHoverer(collisionMgr, curve, point);
    collisionMgr->addEqHoverer(hover);

    /* point signals */
    QObject::connect(point,
                  SIGNAL(pointPositionChanged(CurvePoint*)),
                  curve,
                  SLOT(pointPositionChanged(CurvePoint*)));
    QObject::connect(point,
                  SIGNAL(pointSlopeChanged(int)),
                  curve,
                  SLOT(pointSlopeChanged(int)));

    /* curve signals */
    QObject::connect(curve,
                  SIGNAL(resync(FilterCurve*)),
                  hover,
                  SLOT(resync(FilterCurve*)));
    QObject::connect(curve,
                     SIGNAL(filterParamsChanged(ShimFilterPtr, PeakingCurve*)),
                     this,
                     SLOT(peakingFilterParamsChanged(ShimFilterPtr, PeakingCurve*)));

    QGraphicsItemGroup *group = new QGraphicsItemGroup();
    group->addToGroup(curve);
    group->addToGroup(hover);
    group->setHandlesChildEvents(false);
    group->setPos(xTickBuilder->unlerpTick(frequency) - group->boundingRect().width() / 2, 0);
    point->setResetPos(curve->controlPoint());
    scene->addItem(group);
    scene->addItem(point);
    addFilterItem(group, curve, point, hover);
}

void Gui::addLowShelf(QPen curvePen, QBrush filterBrush, QBrush innerRadiusBrush, QBrush outerRadiusBrush)
{
    Q_ASSERT(collisionMgr);
    ShelfCurve *curve = new LowShelfCurve(curvePen, filterBrush);
    CurvePoint *point = new CurvePoint(innerRadiusBrush, outerRadiusBrush);
    EqHoverer *hover = new EqHoverer(collisionMgr, curve, point);
    collisionMgr->addEqHoverer(hover);

    /* point signals */
    QObject::connect(point,
                     SIGNAL(pointPositionChanged(CurvePoint*)),
                     curve,
                     SLOT(pointPositionChanged(CurvePoint*)));
    QObject::connect(point,
                     SIGNAL(pointSlopeChanged(int)),
                     curve,
                     SLOT(pointSlopeChanged(int)));

    /* curve signals */
    QObject::connect(curve,
                  SIGNAL(resync(FilterCurve*)),
                  hover,
                  SLOT(resync(FilterCurve*)));
    QObject::connect(curve,
                     SIGNAL(filterParamsChanged(ShimFilterPtr, ShelfCurve*)),
                     this,
                     SLOT(lowshelfFilterParamsChanged(ShimFilterPtr, ShelfCurve*)));

    QGraphicsItemGroup *group = new QGraphicsItemGroup();
    group->setHandlesChildEvents(false);
    group->addToGroup(curve);
    group->addToGroup(hover);
    group->setPos(XMIN, 0);
    point->setResetPos(curve->controlPoint());
    scene->addItem(group);
    scene->addItem(point);
    addFilterItem(group, curve, point, hover);
}

void Gui::addHighShelf(QPen curvePen, QBrush filterBrush, QBrush innerRadiusBrush, QBrush outerRadiusBrush)
{
    Q_ASSERT(collisionMgr);
    ShelfCurve *curve = new HighShelfCurve(curvePen, filterBrush);
    CurvePoint *point = new CurvePoint(innerRadiusBrush, outerRadiusBrush);
    EqHoverer *hover = new EqHoverer(collisionMgr, curve, point);
    collisionMgr->addEqHoverer(hover);

    /* point signals */
    QObject::connect(point,
                     SIGNAL(pointPositionChanged(CurvePoint*)),
                     curve,
                     SLOT(pointPositionChanged(CurvePoint*)));
    QObject::connect(point,
                     SIGNAL(pointSlopeChanged(int)),
                     curve,
                     SLOT(pointSlopeChanged(int)));

    /* curve signals */
    QObject::connect(curve,
                  SIGNAL(resync(FilterCurve*)),
                  hover,
                  SLOT(resync(FilterCurve*)));
    QObject::connect(curve,
                     SIGNAL(filterParamsChanged(ShimFilterPtr, ShelfCurve*)),
                     this,
                     SLOT(highshelfFilterParamsChanged(ShimFilterPtr, ShelfCurve*)));

    QGraphicsItemGroup *group = new QGraphicsItemGroup();
    group->setHandlesChildEvents(false);
    group->addToGroup(curve);
    group->addToGroup(hover);
    group->setPos(XMAX, 0);
    point->setResetPos(curve->controlPoint());
    scene->addItem(group);
    scene->addItem(point);
    addFilterItem(group, curve, point, hover);
}

void Gui::peakingFilterParamsChanged(ShimFilterPtr filter, PeakingCurve *curve)
{
    QPointF c = curve->controlPoint();
    QRectF r = curve->sceneBoundingRect();
    qreal f0 = xTickBuilder->lerpTick(c.x());
    qreal bw = xTickBuilder->lerpTick(r.bottomRight().x()) / xTickBuilder->lerpTick(r.bottomLeft().x()) / 2;
    qreal db_gain = LINEAR_REMAP(
                c.y(),
                scene->sceneRect().y(), scene->sceneRect().y() + scene->sceneRect().height(),
                DB_GAIN_MAX, -DB_GAIN_MAX);
    PrettyShim::getInstance().set_peaking_eq(filter, f0, bw, db_gain);
}

void Gui::lowshelfFilterParamsChanged(ShimFilterPtr filter, ShelfCurve *curve)
{
    QPointF c = curve->controlPoint();
    qreal f0 = xTickBuilder->lerpTick(c.x());
    qreal S =  curve->slope();
    qreal db_gain = LINEAR_REMAP(
                c.y(),
                scene->sceneRect().y(), scene->sceneRect().y() + scene->sceneRect().height(),
                DB_GAIN_MAX, -DB_GAIN_MAX);
    PrettyShim::getInstance().set_low_shelf(filter, f0, S, db_gain);
}

void Gui::highshelfFilterParamsChanged(ShimFilterPtr filter, ShelfCurve *curve)
{

    QPointF c = curve->controlPoint();
    qreal f0 = xTickBuilder->lerpTick(c.x());
    qreal S =  curve->slope();
    qreal db_gain = LINEAR_REMAP(
                c.y(),
                scene->sceneRect().y(), scene->sceneRect().y() + scene->sceneRect().height(),
                DB_GAIN_MAX, -DB_GAIN_MAX);
    PrettyShim::getInstance().set_high_shelf(filter, f0, S, db_gain);
}

void Gui::connectBypassButton()
{
    QObject::connect(ui->pushButton, &QAbstractButton::clicked, this, [&](bool checked){
        PrettyShim::getInstance().enable_bypass(checked);
    });
}

void Gui::addSpectrumAnalyzer()
{
    Q_ASSERT(xTickBuilder);
    spectrumAnalyzer = new SpectrumAnalyzer(xTickBuilder);
    spectrumAnalyzer->setPos(-scene->sceneRect().width() / 2, -scene->sceneRect().height() / 8);
    scene->addItem(spectrumAnalyzer);
    spectrumUpdateTimer = new QTimer(this);
    spectrumUpdateTimer->setInterval(1000 / 60);
    QObject::connect(spectrumUpdateTimer, &QTimer::timeout, [&]() {
        spectrumAnalyzer->updateFrameDelta();
        spectrumAnalyzer->update();
    });
    spectrumUpdateTimer->start();
}

//=============================================================================

void Gui::maybeShowInSystemTray()
{
    if (!QSystemTrayIcon::isSystemTrayAvailable())
        return;

    trayMenu = new QMenu();
    quitAct = new QAction("Quit");
    trayMenu->addAction(quitAct);
    QObject::connect(quitAct, &QAction::triggered, qApp, QCoreApplication::quit);

    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setContextMenu(trayMenu);
    trayIcon->setIcon(QIcon(":/images/images/prettyeq.png"));
    trayIcon->setContextMenu(trayMenu);
    QObject::connect(trayIcon,
                     SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
                     this,
                     SLOT(trayActivated(QSystemTrayIcon::ActivationReason)));

    trayIcon->show();
}

//=============================================================================

void Gui::resizeEvent(QResizeEvent *event) {
    QDialog::resizeEvent(event);
    qDebug() << this->size();
    if (isVisible())
        ui->graphicsView->fitInView(scene->sceneRect(), Qt::KeepAspectRatio);
    //ui->graphicsView->centerOn(0, 0);
}

void Gui::showEvent(QShowEvent *event)
{
    ui->graphicsView->fitInView(scene->sceneRect(), Qt::KeepAspectRatio);
}

void Gui::on_actionQuit_triggered()
{
    QCoreApplication::exit();
}

void Gui::trayActivated(QSystemTrayIcon::ActivationReason reason)
{
    this->show();
    this->raise();
    this->activateWindow();
}

//=============================================================================

Gui::~Gui()
{
    delete collisionMgr;
    for (int i = 0; i < NUM_FILTERS; i++) {
        delete items[i].curve;
        delete items[i].hover;
        delete items[i].point;
        delete items[i].group;
    }

    if (QSystemTrayIcon::isSystemTrayAvailable()) {
        delete trayIcon;
        delete trayMenu;
        delete quitAct;
    }
    spectrumUpdateTimer->stop();
    delete spectrumUpdateTimer;
    delete spectrumAnalyzer;
    delete ui;
}

void Gui::cleanup()
{
    PrettyShim::getInstance().exit();
}
