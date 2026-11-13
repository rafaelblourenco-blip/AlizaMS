#include "radiantshortcutsmanager.h"
#include "mainwindow.h"
#include "aliza.h"
#include "graphicswidget.h"
#include "graphicsview.h"
#include "studygraphicswidget.h"
#include "studygraphicsview.h"
#include "studyviewwidget.h"
#include "studyframewidget.h"
#include "imagesbox.h"
#include "sliderwidget.h"
#include "animwidget.h"

#include <QScrollBar>
#include <QApplication>
#include <QMenuBar>
#include <QStatusBar>
#include <QDockWidget>
#include <QTabWidget>
#include <algorithm>

RadiantShortcutsManager::RadiantShortcutsManager(MainWindow * mainWindow, QObject * parent)
    : QObject(parent)
    , mw(mainWindow)
{
    connect(RadiantInputDispatcher::instance(), SIGNAL(tool_changed(RadiantTool)),
            this, SLOT(on_dispatcher_tool_changed(RadiantTool)));
}

void RadiantShortcutsManager::setup_shortcut(const QKeySequence & seq, const char * slot, bool autoRepeat)
{
    if (!mw) return;
    QShortcut * sc = new QShortcut(seq, mw);
    sc->setContext(Qt::ApplicationShortcut);
    sc->setAutoRepeat(autoRepeat);
    connect(sc, SIGNAL(activated()), this, slot);
}

void RadiantShortcutsManager::initialize_shortcuts()
{
    if (!mw) return;

    // -------------------------------------------------------------
    // Create Tool Actions & Action Group
    // -------------------------------------------------------------
    tool_action_group = new QActionGroup(this);
    tool_action_group->setExclusive(true);

    action_browse = new QAction(QIcon(":/bitmaps/align.svg"), QString("Browse Series (B)"), this);
    action_browse->setCheckable(true);
    action_browse->setChecked(true);
    tool_action_group->addAction(action_browse);
    connect(action_browse, SIGNAL(triggered()), this, SLOT(select_tool_browse()));

    action_wl = new QAction(QIcon(":/bitmaps/circle1.svg"), QString("Window/Level (W)"), this);
    action_wl->setCheckable(true);
    tool_action_group->addAction(action_wl);
    connect(action_wl, SIGNAL(triggered()), this, SLOT(select_tool_wl()));

    action_pan = new QAction(QIcon(":/bitmaps/cursor.svg"), QString("Pan Image (M)"), this);
    action_pan->setCheckable(true);
    tool_action_group->addAction(action_pan);
    connect(action_pan, SIGNAL(triggered()), this, SLOT(select_tool_pan()));

    action_zoom = new QAction(QIcon(":/bitmaps/zoomin.svg"), QString("Zoom Image (Z)"), this);
    action_zoom->setCheckable(true);
    tool_action_group->addAction(action_zoom);
    connect(action_zoom, SIGNAL(triggered()), this, SLOT(select_tool_zoom()));

    action_length = new QAction(QIcon(":/bitmaps/distance.svg"), QString("Length Ruler (L)"), this);
    action_length->setCheckable(true);
    tool_action_group->addAction(action_length);
    connect(action_length, SIGNAL(triggered()), this, SLOT(select_tool_length()));

    action_ellipse = new QAction(QIcon(":/bitmaps/circle1.svg"), QString("Ellipse ROI (E)"), this);
    action_ellipse->setCheckable(true);
    tool_action_group->addAction(action_ellipse);
    connect(action_ellipse, SIGNAL(triggered()), this, SLOT(select_tool_ellipse()));

    action_closed_poly = new QAction(QIcon(":/bitmaps/group.svg"), QString("Closed Polygon (G)"), this);
    action_closed_poly->setCheckable(true);
    tool_action_group->addAction(action_closed_poly);
    connect(action_closed_poly, SIGNAL(triggered()), this, SLOT(select_tool_closed_poly()));

    action_open_poly = new QAction(QIcon(":/bitmaps/group.svg"), QString("Open Polygon (O)"), this);
    action_open_poly->setCheckable(true);
    tool_action_group->addAction(action_open_poly);
    connect(action_open_poly, SIGNAL(triggered()), this, SLOT(select_tool_open_poly()));

    action_angle = new QAction(QIcon(":/bitmaps/cross.svg"), QString("Angle (A)"), this);
    action_angle->setCheckable(true);
    tool_action_group->addAction(action_angle);
    connect(action_angle, SIGNAL(triggered()), this, SLOT(select_tool_angle()));

    action_cobb = new QAction(QIcon(":/bitmaps/cross.svg"), QString("Cobb Angle (C)"), this);
    action_cobb->setCheckable(true);
    tool_action_group->addAction(action_cobb);
    connect(action_cobb, SIGNAL(triggered()), this, SLOT(select_tool_cobb()));

    action_deviation = new QAction(QIcon(":/bitmaps/chart0.svg"), QString("Deviation (D)"), this);
    action_deviation->setCheckable(true);
    tool_action_group->addAction(action_deviation);
    connect(action_deviation, SIGNAL(triggered()), this, SLOT(select_tool_deviation()));

    action_arrow = new QAction(QIcon(":/bitmaps/right0.svg"), QString("Arrow (Y)"), this);
    action_arrow->setCheckable(true);
    tool_action_group->addAction(action_arrow);
    connect(action_arrow, SIGNAL(triggered()), this, SLOT(select_tool_arrow()));

    action_pen = new QAction(QIcon(":/bitmaps/tool.svg"), QString("Pen (P)"), this);
    action_pen->setCheckable(true);
    tool_action_group->addAction(action_pen);
    connect(action_pen, SIGNAL(triggered()), this, SLOT(select_tool_pen()));

    // -------------------------------------------------------------
    // Additional Direct Toolbar Actions
    // -------------------------------------------------------------
    action_open_file = new QAction(QIcon(":/bitmaps/file.svg"), QString("Open File (Ctrl+O)"), this);
    connect(action_open_file, SIGNAL(triggered()), this, SLOT(trigger_open_file()));

    action_open_dir = new QAction(QIcon(":/bitmaps/folder.svg"), QString("Scan Folder (Ctrl+Shift+O)"), this);
    connect(action_open_dir, SIGNAL(triggered()), this, SLOT(trigger_scan_folder()));

    action_preset_0 = new QAction(QString("Default (0)"), this);
    connect(action_preset_0, SIGNAL(triggered()), this, SLOT(apply_preset_0()));

    action_preset_1 = new QAction(QString("Full (1)"), this);
    connect(action_preset_1, SIGNAL(triggered()), this, SLOT(apply_preset_1()));

    action_preset_2 = new QAction(QString("Abdomen (2)"), this);
    connect(action_preset_2, SIGNAL(triggered()), this, SLOT(apply_preset_2()));

    action_preset_3 = new QAction(QString("Angio (3)"), this);
    connect(action_preset_3, SIGNAL(triggered()), this, SLOT(apply_preset_3()));

    action_preset_4 = new QAction(QString("Bone (4)"), this);
    connect(action_preset_4, SIGNAL(triggered()), this, SLOT(apply_preset_4()));

    action_preset_5 = new QAction(QString("Brain (5)"), this);
    connect(action_preset_5, SIGNAL(triggered()), this, SLOT(apply_preset_5()));

    action_preset_6 = new QAction(QString("Mediastinum (6)"), this);
    connect(action_preset_6, SIGNAL(triggered()), this, SLOT(apply_preset_6()));

    action_preset_7 = new QAction(QString("Lung (7)"), this);
    connect(action_preset_7, SIGNAL(triggered()), this, SLOT(apply_preset_7()));

    action_invert_lut = new QAction(QIcon(":/bitmaps/reload.svg"), QString("Invert LUT (F11)"), this);
    connect(action_invert_lut, SIGNAL(triggered()), this, SLOT(toggle_invert_lut()));

    action_sync = new QAction(QIcon(":/bitmaps/anchor.svg"), QString("Sync Series (F5)"), this);
    connect(action_sync, SIGNAL(triggered()), this, SLOT(toggle_series_sync()));

    action_cine = new QAction(QIcon(":/bitmaps/play.svg"), QString("Cine Play/Stop (Space)"), this);
    connect(action_cine, SIGNAL(triggered()), this, SLOT(toggle_cine()));

    action_split = new QAction(QIcon(":/bitmaps/grid.svg"), QString("Split Screen (F4)"), this);
    connect(action_split, SIGNAL(triggered()), this, SLOT(toggle_split_screen()));

    action_mpr_axial = new QAction(QIcon(":/bitmaps/2dz2.svg"), QString("MPR Axial (Ctrl+Alt+3)"), this);
    connect(action_mpr_axial, SIGNAL(triggered()), this, SLOT(trigger_mpr_axial()));

    action_3d_vr = new QAction(QIcon(":/bitmaps/3d.svg"), QString("3D VR (F3)"), this);
    connect(action_3d_vr, SIGNAL(triggered()), this, SLOT(trigger_3d_vr()));

    // -------------------------------------------------------------
    // Register Global Application Shortcuts
    // -------------------------------------------------------------
    // A. Tool Selection
    setup_shortcut(QKeySequence(Qt::Key_B), SLOT(select_tool_browse()));
    setup_shortcut(QKeySequence(Qt::Key_W), SLOT(select_tool_wl()));
    setup_shortcut(QKeySequence(Qt::Key_M), SLOT(select_tool_pan()));
    setup_shortcut(QKeySequence(Qt::Key_Z), SLOT(select_tool_zoom()));
    setup_shortcut(QKeySequence(Qt::Key_L), SLOT(select_tool_length()));
    setup_shortcut(QKeySequence(Qt::Key_E), SLOT(select_tool_ellipse()));
    setup_shortcut(QKeySequence(Qt::Key_G), SLOT(select_tool_closed_poly()));
    setup_shortcut(QKeySequence(Qt::Key_O), SLOT(select_tool_open_poly()));
    setup_shortcut(QKeySequence(Qt::Key_A), SLOT(select_tool_angle()));
    setup_shortcut(QKeySequence(Qt::Key_C), SLOT(select_tool_cobb()));
    setup_shortcut(QKeySequence(Qt::Key_D), SLOT(select_tool_deviation()));
    setup_shortcut(QKeySequence(Qt::Key_Y), SLOT(select_tool_arrow()));
    setup_shortcut(QKeySequence(Qt::Key_P), SLOT(select_tool_pen()));

    // B. Presets & Window/Level
    setup_shortcut(QKeySequence(Qt::Key_0), SLOT(apply_preset_0()));
    setup_shortcut(QKeySequence(Qt::Key_1), SLOT(apply_preset_1()));
    setup_shortcut(QKeySequence(Qt::Key_2), SLOT(apply_preset_2()));
    setup_shortcut(QKeySequence(Qt::Key_3), SLOT(apply_preset_3()));
    setup_shortcut(QKeySequence(Qt::Key_4), SLOT(apply_preset_4()));
    setup_shortcut(QKeySequence(Qt::Key_5), SLOT(apply_preset_5()));
    setup_shortcut(QKeySequence(Qt::Key_6), SLOT(apply_preset_6()));
    setup_shortcut(QKeySequence(Qt::Key_7), SLOT(apply_preset_7()));
    setup_shortcut(QKeySequence(Qt::Key_8), SLOT(apply_preset_8()));
    setup_shortcut(QKeySequence(Qt::Key_9), SLOT(apply_preset_9()));
    setup_shortcut(QKeySequence(Qt::Key_F11), SLOT(toggle_invert_lut()), false);
    setup_shortcut(QKeySequence(Qt::CTRL + Qt::Key_F11), SLOT(show_custom_wl_dialog()), false);

    // C. Navigation & Cine
    setup_shortcut(QKeySequence(Qt::Key_Up), SLOT(prev_image()));
    setup_shortcut(QKeySequence(Qt::Key_Down), SLOT(next_image()));
    setup_shortcut(QKeySequence(Qt::Key_PageUp), SLOT(prev_10_images()));
    setup_shortcut(QKeySequence(Qt::Key_PageDown), SLOT(next_10_images()));
    setup_shortcut(QKeySequence(Qt::Key_Left), SLOT(prev_series()));
    setup_shortcut(QKeySequence(Qt::Key_Right), SLOT(next_series()));
    setup_shortcut(QKeySequence(Qt::Key_Home), SLOT(first_image_or_series()));
    setup_shortcut(QKeySequence(Qt::Key_End), SLOT(last_image_or_series()));
    setup_shortcut(QKeySequence(Qt::Key_Tab), SLOT(next_panel()));
    setup_shortcut(QKeySequence(Qt::SHIFT + Qt::Key_Backtab), SLOT(prev_panel()));
    setup_shortcut(QKeySequence(Qt::SHIFT + Qt::Key_Tab), SLOT(prev_panel()));
    setup_shortcut(QKeySequence(Qt::Key_Space), SLOT(toggle_cine()), false);

    // C. Zoom & Pan
    setup_shortcut(QKeySequence(Qt::CTRL + Qt::Key_0), SLOT(zoom_fit()));
    setup_shortcut(QKeySequence(Qt::CTRL + Qt::Key_1), SLOT(zoom_100()));
    setup_shortcut(QKeySequence(Qt::CTRL + Qt::Key_2), SLOT(zoom_200()));
    setup_shortcut(QKeySequence(Qt::CTRL + Qt::Key_3), SLOT(zoom_400()));
    setup_shortcut(QKeySequence(Qt::CTRL + Qt::Key_4), SLOT(zoom_800()));
    setup_shortcut(QKeySequence(Qt::CTRL + Qt::Key_Plus), SLOT(zoom_in()));
    setup_shortcut(QKeySequence(Qt::CTRL + Qt::Key_Equal), SLOT(zoom_in()));
    setup_shortcut(QKeySequence(Qt::CTRL + Qt::Key_Minus), SLOT(zoom_out()));
    setup_shortcut(QKeySequence(Qt::CTRL + Qt::Key_Up), SLOT(pan_up()));
    setup_shortcut(QKeySequence(Qt::CTRL + Qt::Key_Down), SLOT(pan_down()));
    setup_shortcut(QKeySequence(Qt::CTRL + Qt::Key_Left), SLOT(pan_left()));
    setup_shortcut(QKeySequence(Qt::CTRL + Qt::Key_Right), SLOT(pan_right()));

    // C. Transformations & Filters
    setup_shortcut(QKeySequence(Qt::CTRL + Qt::Key_BracketLeft), SLOT(rotate_90_ccw()), false);
    setup_shortcut(QKeySequence(Qt::CTRL + Qt::Key_BracketRight), SLOT(rotate_90_cw()), false);
    setup_shortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_BracketLeft), SLOT(flip_horizontal()), false);
    setup_shortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_BracketRight), SLOT(flip_vertical()), false);
    setup_shortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Backslash), SLOT(clear_geometric_transforms()), false);
    setup_shortcut(QKeySequence(Qt::Key_I), SLOT(toggle_sharpen()), false);
    setup_shortcut(QKeySequence(Qt::SHIFT + Qt::Key_I), SLOT(disable_sharpen()), false);

    // D. Interface & Overlays
    setup_shortcut(QKeySequence(Qt::Key_F12), SLOT(toggle_hud()), false);
    setup_shortcut(QKeySequence(Qt::SHIFT + Qt::Key_F12), SLOT(toggle_patient_id_hud()), false);
    setup_shortcut(QKeySequence(Qt::CTRL + Qt::Key_F12), SLOT(toggle_cross_reference()), false);
    setup_shortcut(QKeySequence(Qt::ALT + Qt::Key_F12), SLOT(toggle_measurements()), false);
    setup_shortcut(QKeySequence(Qt::Key_F5), SLOT(toggle_series_sync()), false);
    setup_shortcut(QKeySequence(Qt::Key_F4), SLOT(toggle_split_screen()), false);
    setup_shortcut(QKeySequence(Qt::SHIFT + Qt::Key_F4), SLOT(close_split_screen()), false);
    setup_shortcut(QKeySequence(Qt::CTRL + Qt::Key_M), SLOT(maximize_active_panel()), false);
    setup_shortcut(QKeySequence(Qt::Key_F), SLOT(toggle_full_screen()), false);
    setup_shortcut(QKeySequence(Qt::SHIFT + Qt::ALT + Qt::Key_Return), SLOT(toggle_distraction_free()), false);
    setup_shortcut(QKeySequence(Qt::SHIFT + Qt::ALT + Qt::Key_Enter), SLOT(toggle_distraction_free()), false);
    setup_shortcut(QKeySequence(Qt::CTRL + Qt::ALT + Qt::Key_F2), SLOT(toggle_thumbnail_strip()), false);
    setup_shortcut(QKeySequence(Qt::CTRL + Qt::ALT + Qt::Key_F3), SLOT(toggle_top_toolbar()), false);
    setup_shortcut(QKeySequence(Qt::CTRL + Qt::ALT + Qt::Key_1), SLOT(trigger_mpr_coronal()), false);
    setup_shortcut(QKeySequence(Qt::CTRL + Qt::ALT + Qt::Key_2), SLOT(trigger_mpr_sagittal()), false);
    setup_shortcut(QKeySequence(Qt::CTRL + Qt::ALT + Qt::Key_3), SLOT(trigger_mpr_axial()), false);
    setup_shortcut(QKeySequence(Qt::Key_F3), SLOT(trigger_3d_vr()), false);
    setup_shortcut(QKeySequence(Qt::CTRL + Qt::ALT + Qt::Key_F), SLOT(trigger_fusion()), false);
    setup_shortcut(QKeySequence(Qt::CTRL + Qt::ALT + Qt::Key_T), SLOT(trigger_dicom_tags()), false);
    setup_shortcut(QKeySequence(Qt::CTRL + Qt::Key_O), SLOT(trigger_open_file()), false);
    setup_shortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_O), SLOT(trigger_scan_folder()), false);

    // D. ROIs deletion
    setup_shortcut(QKeySequence(Qt::Key_Delete), SLOT(delete_selected_roi()), false);
    setup_shortcut(QKeySequence(Qt::SHIFT + Qt::Key_Delete), SLOT(delete_all_rois_image()), false);
    setup_shortcut(QKeySequence(Qt::CTRL + Qt::Key_Delete), SLOT(delete_all_rois_series()), false);
    setup_shortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Delete), SLOT(delete_all_rois_all()), false);

    // D. MPR 3D Mode Shortcuts
    setup_shortcut(QKeySequence(Qt::Key_F2), SLOT(mpr3d_toggle_mode()), false);
    setup_shortcut(QKeySequence(Qt::CTRL + Qt::Key_F3), SLOT(mpr3d_center_axes()), false);
    setup_shortcut(QKeySequence(Qt::Key_H), SLOT(mpr3d_slab_thickness()), false);
    setup_shortcut(QKeySequence(Qt::Key_Plus), SLOT(mpr3d_increase_slab()));
    setup_shortcut(QKeySequence(Qt::Key_Minus), SLOT(mpr3d_decrease_slab()));
    setup_shortcut(QKeySequence(Qt::Key_R), SLOT(mpr3d_rotate()), false);
    setup_shortcut(QKeySequence(Qt::Key_T), SLOT(mpr3d_roll()), false);
    setup_shortcut(QKeySequence(Qt::Key_Return), SLOT(mpr3d_maximize()), false);
    setup_shortcut(QKeySequence(Qt::Key_Enter), SLOT(mpr3d_maximize()), false);
    setup_shortcut(QKeySequence(Qt::CTRL + Qt::Key_R), SLOT(mpr3d_reset()), false);
}

void RadiantShortcutsManager::create_radiant_toolbar(QToolBar * parentToolbar)
{
    if (!mw) return;

    if (!parentToolbar)
    {
        radiant_toolbar = mw->addToolBar(QString("RadiAnt Tools"));
        radiant_toolbar->setObjectName(QString("RadiAntToolBar"));
    }
    else
    {
        radiant_toolbar = parentToolbar;
    }

    radiant_toolbar->setIconSize(QSize(22, 22));

    // File Operations
    radiant_toolbar->addAction(action_open_dir);
    radiant_toolbar->addAction(action_open_file);
    radiant_toolbar->addSeparator();

    // Primary Mouse Tools
    radiant_toolbar->addAction(action_browse);
    radiant_toolbar->addAction(action_wl);
    radiant_toolbar->addAction(action_pan);
    radiant_toolbar->addAction(action_zoom);
    radiant_toolbar->addAction(action_length);
    radiant_toolbar->addAction(action_ellipse);
    radiant_toolbar->addAction(action_angle);
    radiant_toolbar->addAction(action_cobb);
    radiant_toolbar->addSeparator();

    // Presets
    radiant_toolbar->addAction(action_preset_0);
    radiant_toolbar->addAction(action_preset_1);
    radiant_toolbar->addAction(action_preset_2);
    radiant_toolbar->addAction(action_preset_3);
    radiant_toolbar->addAction(action_preset_4);
    radiant_toolbar->addAction(action_preset_5);
    radiant_toolbar->addAction(action_preset_6);
    radiant_toolbar->addAction(action_preset_7);
    radiant_toolbar->addSeparator();

    // Utility & View Modes
    radiant_toolbar->addAction(action_invert_lut);
    radiant_toolbar->addAction(action_sync);
    radiant_toolbar->addAction(action_cine);
    radiant_toolbar->addAction(action_split);
    radiant_toolbar->addAction(action_mpr_axial);
    radiant_toolbar->addAction(action_3d_vr);
}

void RadiantShortcutsManager::on_dispatcher_tool_changed(RadiantTool tool)
{
    switch (tool)
    {
    case RadiantTool::Browse:
        if (action_browse) action_browse->setChecked(true);
        break;
    case RadiantTool::WindowLevel:
        if (action_wl) action_wl->setChecked(true);
        break;
    case RadiantTool::Pan:
        if (action_pan) action_pan->setChecked(true);
        break;
    case RadiantTool::Zoom:
        if (action_zoom) action_zoom->setChecked(true);
        break;
    case RadiantTool::Length:
        if (action_length) action_length->setChecked(true);
        break;
    case RadiantTool::Ellipse:
        if (action_ellipse) action_ellipse->setChecked(true);
        break;
    case RadiantTool::ClosedPolygon:
        if (action_closed_poly) action_closed_poly->setChecked(true);
        break;
    case RadiantTool::OpenPolygon:
        if (action_open_poly) action_open_poly->setChecked(true);
        break;
    case RadiantTool::Angle:
        if (action_angle) action_angle->setChecked(true);
        break;
    case RadiantTool::CobbAngle:
        if (action_cobb) action_cobb->setChecked(true);
        break;
    case RadiantTool::Deviation:
        if (action_deviation) action_deviation->setChecked(true);
        break;
    case RadiantTool::Arrow:
        if (action_arrow) action_arrow->setChecked(true);
        break;
    case RadiantTool::Pen:
        if (action_pen) action_pen->setChecked(true);
        break;
    default:
        break;
    }
}

// -------------------------------------------------------------
// A. Tool Selection Slots
// -------------------------------------------------------------
void RadiantShortcutsManager::select_tool_browse()
{
    RadiantInputDispatcher::instance()->set_tool(RadiantTool::Browse);
    if (mw && mw->graphicswidget_m && mw->graphicswidget_m->graphicsview)
        mw->graphicswidget_m->graphicsview->setCursor(Qt::ArrowCursor);
}

void RadiantShortcutsManager::select_tool_wl()
{
    RadiantInputDispatcher::instance()->set_tool(RadiantTool::WindowLevel);
    if (mw && mw->graphicswidget_m && mw->graphicswidget_m->graphicsview)
        mw->graphicswidget_m->graphicsview->setCursor(Qt::SizeAllCursor);
}

void RadiantShortcutsManager::select_tool_pan()
{
    RadiantInputDispatcher::instance()->set_tool(RadiantTool::Pan);
    if (mw && mw->graphicswidget_m && mw->graphicswidget_m->graphicsview)
        mw->graphicswidget_m->graphicsview->setCursor(Qt::OpenHandCursor);
}

void RadiantShortcutsManager::select_tool_zoom()
{
    RadiantInputDispatcher::instance()->set_tool(RadiantTool::Zoom);
    if (mw && mw->graphicswidget_m && mw->graphicswidget_m->graphicsview)
        mw->graphicswidget_m->graphicsview->setCursor(Qt::SizeVerCursor);
}

void RadiantShortcutsManager::select_tool_length()
{
    RadiantInputDispatcher::instance()->set_tool(RadiantTool::Length);
    if (mw && mw->graphicswidget_m && mw->graphicswidget_m->graphicsview)
        mw->graphicswidget_m->graphicsview->setCursor(Qt::CrossCursor);
}

void RadiantShortcutsManager::select_tool_ellipse()
{
    RadiantInputDispatcher::instance()->set_tool(RadiantTool::Ellipse);
    if (mw && mw->graphicswidget_m && mw->graphicswidget_m->graphicsview)
        mw->graphicswidget_m->graphicsview->setCursor(Qt::CrossCursor);
}

void RadiantShortcutsManager::select_tool_closed_poly()
{
    RadiantInputDispatcher::instance()->set_tool(RadiantTool::ClosedPolygon);
    if (mw && mw->graphicswidget_m && mw->graphicswidget_m->graphicsview)
        mw->graphicswidget_m->graphicsview->setCursor(Qt::CrossCursor);
}

void RadiantShortcutsManager::select_tool_open_poly()
{
    RadiantInputDispatcher::instance()->set_tool(RadiantTool::OpenPolygon);
    if (mw && mw->graphicswidget_m && mw->graphicswidget_m->graphicsview)
        mw->graphicswidget_m->graphicsview->setCursor(Qt::CrossCursor);
}

void RadiantShortcutsManager::select_tool_angle()
{
    RadiantInputDispatcher::instance()->set_tool(RadiantTool::Angle);
    if (mw && mw->graphicswidget_m && mw->graphicswidget_m->graphicsview)
        mw->graphicswidget_m->graphicsview->setCursor(Qt::CrossCursor);
}

void RadiantShortcutsManager::select_tool_cobb()
{
    RadiantInputDispatcher::instance()->set_tool(RadiantTool::CobbAngle);
    if (mw && mw->graphicswidget_m && mw->graphicswidget_m->graphicsview)
        mw->graphicswidget_m->graphicsview->setCursor(Qt::CrossCursor);
}

void RadiantShortcutsManager::select_tool_deviation()
{
    RadiantInputDispatcher::instance()->set_tool(RadiantTool::Deviation);
    if (mw && mw->graphicswidget_m && mw->graphicswidget_m->graphicsview)
        mw->graphicswidget_m->graphicsview->setCursor(Qt::CrossCursor);
}

void RadiantShortcutsManager::select_tool_arrow()
{
    RadiantInputDispatcher::instance()->set_tool(RadiantTool::Arrow);
    if (mw && mw->graphicswidget_m && mw->graphicswidget_m->graphicsview)
        mw->graphicswidget_m->graphicsview->setCursor(Qt::UpArrowCursor);
}

void RadiantShortcutsManager::select_tool_pen()
{
    RadiantInputDispatcher::instance()->set_tool(RadiantTool::Pen);
    if (mw && mw->graphicswidget_m && mw->graphicswidget_m->graphicsview)
        mw->graphicswidget_m->graphicsview->setCursor(Qt::CrossCursor);
}

// -------------------------------------------------------------
// B. Presets & Window/Level Slots
// -------------------------------------------------------------
void RadiantShortcutsManager::apply_preset_0() { if (mw && mw->aliza) mw->aliza->apply_wl_preset(0); }
void RadiantShortcutsManager::apply_preset_1() { if (mw && mw->aliza) mw->aliza->apply_wl_preset(1); }
void RadiantShortcutsManager::apply_preset_2() { if (mw && mw->aliza) mw->aliza->apply_wl_preset(2); }
void RadiantShortcutsManager::apply_preset_3() { if (mw && mw->aliza) mw->aliza->apply_wl_preset(3); }
void RadiantShortcutsManager::apply_preset_4() { if (mw && mw->aliza) mw->aliza->apply_wl_preset(4); }
void RadiantShortcutsManager::apply_preset_5() { if (mw && mw->aliza) mw->aliza->apply_wl_preset(5); }
void RadiantShortcutsManager::apply_preset_6() { if (mw && mw->aliza) mw->aliza->apply_wl_preset(6); }
void RadiantShortcutsManager::apply_preset_7() { if (mw && mw->aliza) mw->aliza->apply_wl_preset(7); }
void RadiantShortcutsManager::apply_preset_8() { if (mw && mw->aliza) mw->aliza->apply_wl_preset(8); }
void RadiantShortcutsManager::apply_preset_9() { if (mw && mw->aliza) mw->aliza->apply_wl_preset(9); }

void RadiantShortcutsManager::toggle_invert_lut()
{
    if (mw && mw->aliza) mw->aliza->toggle_invert_lut();
}

void RadiantShortcutsManager::show_custom_wl_dialog()
{
    if (mw && mw->aliza) mw->aliza->show_custom_wl_dialog(mw);
}

// -------------------------------------------------------------
// C. Navigation & Cine Slots
// -------------------------------------------------------------
void RadiantShortcutsManager::prev_image()
{
    if (!mw) return;
    if (mw->multiview_tab && mw->tabWidget->currentIndex() == 1 && mw->studyview)
    {
        const int active_id = mw->studyview->get_active_id();
        for (int i = 0; i < mw->studyview->widgets.size(); ++i)
        {
            StudyFrameWidget * sfw = mw->studyview->widgets.at(i);
            if (sfw && sfw->graphicswidget && sfw->graphicswidget->get_id() == active_id)
            {
                QSlider * sl = sfw->graphicswidget->get_slider();
                if (sl) sl->setValue(sl->value() - 1);
                return;
            }
        }
    }
    if (mw->slider_m && mw->slider_m->slices_slider)
    {
        mw->slider_m->slices_slider->setValue(mw->slider_m->slices_slider->value() - 1);
    }
}

void RadiantShortcutsManager::next_image()
{
    if (!mw) return;
    if (mw->multiview_tab && mw->tabWidget->currentIndex() == 1 && mw->studyview)
    {
        const int active_id = mw->studyview->get_active_id();
        for (int i = 0; i < mw->studyview->widgets.size(); ++i)
        {
            StudyFrameWidget * sfw = mw->studyview->widgets.at(i);
            if (sfw && sfw->graphicswidget && sfw->graphicswidget->get_id() == active_id)
            {
                QSlider * sl = sfw->graphicswidget->get_slider();
                if (sl) sl->setValue(sl->value() + 1);
                return;
            }
        }
    }
    if (mw->slider_m && mw->slider_m->slices_slider)
    {
        mw->slider_m->slices_slider->setValue(mw->slider_m->slices_slider->value() + 1);
    }
}

void RadiantShortcutsManager::prev_10_images()
{
    if (!mw) return;
    if (mw->multiview_tab && mw->tabWidget->currentIndex() == 1 && mw->studyview)
    {
        const int active_id = mw->studyview->get_active_id();
        for (int i = 0; i < mw->studyview->widgets.size(); ++i)
        {
            StudyFrameWidget * sfw = mw->studyview->widgets.at(i);
            if (sfw && sfw->graphicswidget && sfw->graphicswidget->get_id() == active_id)
            {
                QSlider * sl = sfw->graphicswidget->get_slider();
                if (sl) sl->setValue(sl->value() - 10);
                return;
            }
        }
    }
    if (mw->slider_m && mw->slider_m->slices_slider)
    {
        mw->slider_m->slices_slider->setValue(mw->slider_m->slices_slider->value() - 10);
    }
}

void RadiantShortcutsManager::next_10_images()
{
    if (!mw) return;
    if (mw->multiview_tab && mw->tabWidget->currentIndex() == 1 && mw->studyview)
    {
        const int active_id = mw->studyview->get_active_id();
        for (int i = 0; i < mw->studyview->widgets.size(); ++i)
        {
            StudyFrameWidget * sfw = mw->studyview->widgets.at(i);
            if (sfw && sfw->graphicswidget && sfw->graphicswidget->get_id() == active_id)
            {
                QSlider * sl = sfw->graphicswidget->get_slider();
                if (sl) sl->setValue(sl->value() + 10);
                return;
            }
        }
    }
    if (mw->slider_m && mw->slider_m->slices_slider)
    {
        mw->slider_m->slices_slider->setValue(mw->slider_m->slices_slider->value() + 10);
    }
}

void RadiantShortcutsManager::prev_series()
{
    if (mw && mw->imagesbox && mw->imagesbox->listWidget)
    {
        const int row = mw->imagesbox->listWidget->currentRow();
        const int count = mw->imagesbox->listWidget->count();
        if (count > 0)
        {
            const int new_row = std::max(0, row - 1);
            mw->imagesbox->listWidget->setCurrentRow(new_row);
        }
    }
}

void RadiantShortcutsManager::next_series()
{
    if (mw && mw->imagesbox && mw->imagesbox->listWidget)
    {
        const int row = mw->imagesbox->listWidget->currentRow();
        const int count = mw->imagesbox->listWidget->count();
        if (count > 0)
        {
            const int new_row = std::min(count - 1, row + 1);
            mw->imagesbox->listWidget->setCurrentRow(new_row);
        }
    }
}

void RadiantShortcutsManager::first_image_or_series()
{
    if (!mw) return;
    if (mw->slider_m && mw->slider_m->slices_slider && mw->slider_m->slices_slider->value() > 0)
    {
        mw->slider_m->slices_slider->setValue(0);
    }
    else if (mw->imagesbox && mw->imagesbox->listWidget)
    {
        mw->imagesbox->listWidget->setCurrentRow(0);
    }
}

void RadiantShortcutsManager::last_image_or_series()
{
    if (!mw) return;
    if (mw->slider_m && mw->slider_m->slices_slider &&
        mw->slider_m->slices_slider->value() < mw->slider_m->slices_slider->maximum())
    {
        mw->slider_m->slices_slider->setValue(mw->slider_m->slices_slider->maximum());
    }
    else if (mw->imagesbox && mw->imagesbox->listWidget && mw->imagesbox->listWidget->count() > 0)
    {
        mw->imagesbox->listWidget->setCurrentRow(mw->imagesbox->listWidget->count() - 1);
    }
}

void RadiantShortcutsManager::next_panel()
{
    if (!mw || !mw->studyview || mw->studyview->widgets.isEmpty()) return;
    const int cur_id = mw->studyview->get_active_id();
    int cur_idx = -1;
    for (int i = 0; i < mw->studyview->widgets.size(); ++i)
    {
        if (mw->studyview->widgets.at(i)->graphicswidget->get_id() == cur_id)
        {
            cur_idx = i;
            break;
        }
    }
    const int count = mw->studyview->widgets.size();
    const int next_idx = (cur_idx + 1) % count;
    mw->studyview->set_active_image(mw->studyview->widgets.at(next_idx)->graphicswidget->get_id());
}

void RadiantShortcutsManager::prev_panel()
{
    if (!mw || !mw->studyview || mw->studyview->widgets.isEmpty()) return;
    const int cur_id = mw->studyview->get_active_id();
    int cur_idx = -1;
    for (int i = 0; i < mw->studyview->widgets.size(); ++i)
    {
        if (mw->studyview->widgets.at(i)->graphicswidget->get_id() == cur_id)
        {
            cur_idx = i;
            break;
        }
    }
    const int count = mw->studyview->widgets.size();
    const int prev_idx = (cur_idx - 1 + count) % count;
    mw->studyview->set_active_image(mw->studyview->widgets.at(prev_idx)->graphicswidget->get_id());
}

void RadiantShortcutsManager::toggle_cine()
{
    if (!mw || !mw->anim2Dwidget || !mw->anim2Dwidget->start_pushButton) return;
    if (mw->anim2Dwidget->start_pushButton->isEnabled())
        mw->start_anim();
    else
        mw->stop_anim();
}

// -------------------------------------------------------------
// C. Zoom & Pan Slots
// -------------------------------------------------------------
void RadiantShortcutsManager::zoom_fit()
{
    if (!mw) return;
    if (mw->resetRectAct2) mw->resetRectAct2->trigger();
}

void RadiantShortcutsManager::zoom_100()
{
    if (!mw || !mw->graphicswidget_m || !mw->graphicswidget_m->graphicsview) return;
    mw->graphicswidget_m->graphicsview->setTransform(QTransform());
}

void RadiantShortcutsManager::zoom_200()
{
    if (!mw || !mw->graphicswidget_m || !mw->graphicswidget_m->graphicsview) return;
    mw->graphicswidget_m->graphicsview->setTransform(QTransform().scale(2.0, 2.0));
}

void RadiantShortcutsManager::zoom_400()
{
    if (!mw || !mw->graphicswidget_m || !mw->graphicswidget_m->graphicsview) return;
    mw->graphicswidget_m->graphicsview->setTransform(QTransform().scale(4.0, 4.0));
}

void RadiantShortcutsManager::zoom_800()
{
    if (!mw || !mw->graphicswidget_m || !mw->graphicswidget_m->graphicsview) return;
    mw->graphicswidget_m->graphicsview->setTransform(QTransform().scale(8.0, 8.0));
}

void RadiantShortcutsManager::zoom_in()
{
    if (!mw || !mw->graphicswidget_m || !mw->graphicswidget_m->graphicsview) return;
    mw->graphicswidget_m->graphicsview->scale(1.2, 1.2);
}

void RadiantShortcutsManager::zoom_out()
{
    if (!mw || !mw->graphicswidget_m || !mw->graphicswidget_m->graphicsview) return;
    mw->graphicswidget_m->graphicsview->scale(1.0 / 1.2, 1.0 / 1.2);
}

void RadiantShortcutsManager::pan_up()
{
    if (!mw || !mw->graphicswidget_m || !mw->graphicswidget_m->graphicsview) return;
    GraphicsView * gv = mw->graphicswidget_m->graphicsview;
    if (gv->verticalScrollBar())
        gv->verticalScrollBar()->setValue(gv->verticalScrollBar()->value() - 40);
}

void RadiantShortcutsManager::pan_down()
{
    if (!mw || !mw->graphicswidget_m || !mw->graphicswidget_m->graphicsview) return;
    GraphicsView * gv = mw->graphicswidget_m->graphicsview;
    if (gv->verticalScrollBar())
        gv->verticalScrollBar()->setValue(gv->verticalScrollBar()->value() + 40);
}

void RadiantShortcutsManager::pan_left()
{
    if (!mw || !mw->graphicswidget_m || !mw->graphicswidget_m->graphicsview) return;
    GraphicsView * gv = mw->graphicswidget_m->graphicsview;
    if (gv->horizontalScrollBar())
        gv->horizontalScrollBar()->setValue(gv->horizontalScrollBar()->value() - 40);
}

void RadiantShortcutsManager::pan_right()
{
    if (!mw || !mw->graphicswidget_m || !mw->graphicswidget_m->graphicsview) return;
    GraphicsView * gv = mw->graphicswidget_m->graphicsview;
    if (gv->horizontalScrollBar())
        gv->horizontalScrollBar()->setValue(gv->horizontalScrollBar()->value() + 40);
}

// -------------------------------------------------------------
// C. Transformations & Filters Slots
// -------------------------------------------------------------
void RadiantShortcutsManager::rotate_90_ccw()
{
    if (!mw || !mw->aliza) return;
    mw->aliza->rotate_ccw();
}

void RadiantShortcutsManager::rotate_90_cw()
{
    if (!mw || !mw->aliza) return;
    mw->aliza->rotate_cw();
}

void RadiantShortcutsManager::flip_horizontal()
{
    if (!mw) return;
    mw->flipX();
}

void RadiantShortcutsManager::flip_vertical()
{
    if (!mw) return;
    mw->flipY();
}

void RadiantShortcutsManager::clear_geometric_transforms()
{
    if (!mw || !mw->aliza) return;
    mw->aliza->reset_geometric_transformations();
}

void RadiantShortcutsManager::toggle_sharpen()
{
    if (!mw || !mw->graphicswidget_m) return;
    mw->graphicswidget_m->set_smooth(false);
    mw->graphicswidget_m->update_image(0, false);
}

void RadiantShortcutsManager::disable_sharpen()
{
    if (!mw || !mw->graphicswidget_m) return;
    mw->graphicswidget_m->set_smooth(true);
    mw->graphicswidget_m->update_image(0, false);
}

// -------------------------------------------------------------
// D. Interface, Overlays & MPR Slots
// -------------------------------------------------------------
void RadiantShortcutsManager::toggle_hud()
{
    GraphicsView::toggle_hud_display();
    if (mw)
    {
        if (mw->graphicswidget_m && mw->graphicswidget_m->graphicsview)
            mw->graphicswidget_m->graphicsview->viewport()->update();
        if (mw->graphicswidget_y && mw->graphicswidget_y->graphicsview)
            mw->graphicswidget_y->graphicsview->viewport()->update();
        if (mw->graphicswidget_x && mw->graphicswidget_x->graphicsview)
            mw->graphicswidget_x->graphicsview->viewport()->update();
        if (mw->studyview)
            mw->studyview->update();
    }
}

void RadiantShortcutsManager::toggle_patient_id_hud()
{
    GraphicsView::toggle_anonymous_display();
    if (mw)
    {
        if (mw->graphicswidget_m && mw->graphicswidget_m->graphicsview)
            mw->graphicswidget_m->graphicsview->viewport()->update();
        if (mw->graphicswidget_y && mw->graphicswidget_y->graphicsview)
            mw->graphicswidget_y->graphicsview->viewport()->update();
        if (mw->graphicswidget_x && mw->graphicswidget_x->graphicsview)
            mw->graphicswidget_x->graphicsview->viewport()->update();
        if (mw->studyview)
            mw->studyview->update();
    }
}

void RadiantShortcutsManager::toggle_cross_reference()
{
    if (mw && mw->collisionAct) mw->collisionAct->trigger();
}

void RadiantShortcutsManager::toggle_measurements()
{
    if (!mw || !mw->graphicswidget_m || !mw->graphicswidget_m->graphicsview) return;
    GraphicsView * gv = mw->graphicswidget_m->graphicsview;
    if (gv->measurment_line)
        gv->measurment_line->setVisible(!gv->measurment_line->isVisible());
}

void RadiantShortcutsManager::toggle_series_sync()
{
    if (mw && mw->zlockAct) mw->zlockAct->trigger();
}

void RadiantShortcutsManager::toggle_split_screen()
{
    if (!mw || !mw->multiview_tab) return;
    const int cur = mw->tabWidget->currentIndex();
    mw->tabWidget->setCurrentIndex(cur == 0 ? 1 : 0);
}

void RadiantShortcutsManager::close_split_screen()
{
    if (!mw) return;
    mw->tabWidget->setCurrentIndex(0);
}

void RadiantShortcutsManager::maximize_active_panel()
{
    if (!mw) return;
    if (!is_panel_maximized)
    {
        cached_3d_frame_visible = mw->gl_frame && mw->gl_frame->isVisible();
        if (cached_3d_frame_visible && mw->gl_frame)
            mw->gl_frame->hide();
        is_panel_maximized = true;
    }
    else
    {
        if (cached_3d_frame_visible && mw->gl_frame)
            mw->gl_frame->show();
        is_panel_maximized = false;
    }
}

void RadiantShortcutsManager::toggle_full_screen()
{
    if (!mw) return;
    if (mw->isFullScreen())
        mw->showNormal();
    else
        mw->showFullScreen();
}

void RadiantShortcutsManager::toggle_distraction_free()
{
    if (!mw) return;
    if (!is_distraction_free)
    {
        cached_menu_visible = mw->menuBar()->isVisible();
        cached_dock_visible = mw->dockWidget1 && mw->dockWidget1->isVisible();
        cached_toolbar2_visible = mw->toolbar2D_frame && mw->toolbar2D_frame->isVisible();
        cached_toolbar4_visible = mw->view2d_frame && mw->view2d_frame->isVisible();
        cached_toolbar5_visible = mw->view3d_frame && mw->view3d_frame->isVisible();
        cached_status_visible = mw->statusBar()->isVisible();

        mw->menuBar()->hide();
        if (mw->dockWidget1) mw->dockWidget1->hide();
        if (mw->toolbar2D_frame) mw->toolbar2D_frame->hide();
        if (mw->view2d_frame) mw->view2d_frame->hide();
        if (mw->view3d_frame) mw->view3d_frame->hide();
        if (radiant_toolbar) radiant_toolbar->hide();
        mw->statusBar()->hide();

        mw->showFullScreen();
        is_distraction_free = true;
    }
    else
    {
        if (cached_menu_visible) mw->menuBar()->show();
        if (cached_dock_visible && mw->dockWidget1) mw->dockWidget1->show();
        if (cached_toolbar2_visible && mw->toolbar2D_frame) mw->toolbar2D_frame->show();
        if (cached_toolbar4_visible && mw->view2d_frame) mw->view2d_frame->show();
        if (cached_toolbar5_visible && mw->view3d_frame) mw->view3d_frame->show();
        if (radiant_toolbar) radiant_toolbar->show();
        if (cached_status_visible) mw->statusBar()->show();

        mw->showNormal();
        is_distraction_free = false;
    }
}

void RadiantShortcutsManager::toggle_thumbnail_strip()
{
    if (mw && mw->dockWidget1)
    {
        mw->dockWidget1->setVisible(!mw->dockWidget1->isVisible());
    }
}

void RadiantShortcutsManager::toggle_top_toolbar()
{
    if (radiant_toolbar)
    {
        radiant_toolbar->setVisible(!radiant_toolbar->isVisible());
    }
    else if (mw && mw->toolbar2D_frame)
    {
        mw->toolbar2D_frame->setVisible(!mw->toolbar2D_frame->isVisible());
    }
}

void RadiantShortcutsManager::trigger_mpr_coronal()
{
    if (mw && mw->graphicsAct_Y)
    {
        mw->graphicsAct_Y->setChecked(true);
        mw->toggle_graphicswidget_m_y(true);
    }
}

void RadiantShortcutsManager::trigger_mpr_sagittal()
{
    if (mw && mw->graphicsAct_X)
    {
        mw->graphicsAct_X->setChecked(true);
        mw->toggle_graphicswidget_m_x(true);
    }
}

void RadiantShortcutsManager::trigger_mpr_axial()
{
    if (mw && mw->graphicsAct_Z)
    {
        mw->graphicsAct_Z->setChecked(true);
        mw->toggle_graphicswidget_m_z(true);
    }
}

void RadiantShortcutsManager::trigger_3d_vr()
{
    if (mw && mw->show3DAct)
    {
        mw->show3DAct->setChecked(!mw->show3DAct->isChecked());
        mw->toggle_showgl(mw->show3DAct->isChecked());
    }
}

void RadiantShortcutsManager::trigger_fusion()
{
    if (mw && mw->imagesbox && mw->imagesbox->actionColor)
    {
        mw->imagesbox->actionColor->trigger();
    }
}

void RadiantShortcutsManager::trigger_dicom_tags()
{
    if (mw && mw->imagesbox && mw->imagesbox->actionDICOMMeta)
    {
        mw->imagesbox->actionDICOMMeta->trigger();
    }
}

void RadiantShortcutsManager::trigger_open_file()
{
    if (mw && mw->openanyAct)
    {
        mw->openanyAct->trigger();
    }
}

void RadiantShortcutsManager::trigger_scan_folder()
{
    if (mw && mw->browser_open_dir_act)
    {
        mw->browser_open_dir_act->trigger();
    }
}

// -------------------------------------------------------------
// D. ROIs deletion Slots
// -------------------------------------------------------------
void RadiantShortcutsManager::delete_selected_roi()
{
    if (mw && mw->graphicswidget_m && mw->graphicswidget_m->graphicsview)
    {
        mw->graphicswidget_m->graphicsview->set_empty_distance();
        mw->graphicswidget_m->set_measure_text(QString(""));
    }
}

void RadiantShortcutsManager::delete_all_rois_image()
{
    delete_selected_roi();
}

void RadiantShortcutsManager::delete_all_rois_series()
{
    if (mw && mw->imagesbox && mw->imagesbox->actionClearChecked)
    {
        mw->imagesbox->actionClearChecked->trigger();
    }
}

void RadiantShortcutsManager::delete_all_rois_all()
{
    if (mw && mw->imagesbox && mw->imagesbox->actionClearAll)
    {
        mw->imagesbox->actionClearAll->trigger();
    }
}

// -------------------------------------------------------------
// D. MPR 3D Mode Slots
// -------------------------------------------------------------
void RadiantShortcutsManager::mpr3d_toggle_mode()
{
    if (!mw) return;
    if (mw->slicesAct && mw->slicesAct->isChecked())
    {
        if (mw->raycastAct) mw->raycastAct->setChecked(true);
    }
    else if (mw->raycastAct && mw->raycastAct->isChecked())
    {
        if (mw->mipAct) mw->mipAct->setChecked(true);
    }
    else if (mw->mipAct)
    {
        if (mw->slicesAct) mw->slicesAct->setChecked(true);
    }
}

void RadiantShortcutsManager::mpr3d_toggle_axes()
{
    if (mw && mw->frames3DAct) mw->frames3DAct->trigger();
}

void RadiantShortcutsManager::mpr3d_center_axes()
{
    if (mw && mw->reset3DAct) mw->reset3DAct->trigger();
}

void RadiantShortcutsManager::mpr3d_layout()
{
    if (mw && mw->zyxAct)
    {
        mw->zyxAct->setChecked(true);
        mw->toggle_graphicswidget_zyx(true);
    }
}

void RadiantShortcutsManager::mpr3d_slab_thickness()
{
    if (mw && mw->zrangeAct) mw->zrangeAct->trigger();
}

void RadiantShortcutsManager::mpr3d_increase_slab()
{
    // Increase slab thickness
}

void RadiantShortcutsManager::mpr3d_decrease_slab()
{
    // Decrease slab thickness
}

void RadiantShortcutsManager::mpr3d_rotate()
{
    // 3D rotation focus
}

void RadiantShortcutsManager::mpr3d_roll()
{
    // Roll focus
}

void RadiantShortcutsManager::mpr3d_maximize()
{
    maximize_active_panel();
}

void RadiantShortcutsManager::mpr3d_reset()
{
    if (mw && mw->resetRectAct2) mw->resetRectAct2->trigger();
    if (mw && mw->reset3DAct) mw->reset3DAct->trigger();
}
