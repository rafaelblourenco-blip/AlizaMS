#ifndef RADIANTSHORTCUTSMANAGER_H
#define RADIANTSHORTCUTSMANAGER_H

#include <QObject>
#include <QAction>
#include <QActionGroup>
#include <QShortcut>
#include <QToolBar>
#include <QKeySequence>
#include "radiantinputdispatcher.h"

class MainWindow;
class Aliza;

class RadiantShortcutsManager : public QObject
{
    Q_OBJECT
public:
    explicit RadiantShortcutsManager(MainWindow * mainWindow, QObject * parent = nullptr);
    ~RadiantShortcutsManager() override = default;

    void initialize_shortcuts();
    void create_radiant_toolbar(QToolBar * parentToolbar = nullptr);

    // Getters for tool actions
    QAction * get_action_browse() const { return action_browse; }
    QAction * get_action_wl() const { return action_wl; }
    QAction * get_action_pan() const { return action_pan; }
    QAction * get_action_zoom() const { return action_zoom; }
    QAction * get_action_length() const { return action_length; }
    QAction * get_action_ellipse() const { return action_ellipse; }
    QAction * get_action_closed_poly() const { return action_closed_poly; }
    QAction * get_action_open_poly() const { return action_open_poly; }
    QAction * get_action_angle() const { return action_angle; }
    QAction * get_action_cobb() const { return action_cobb; }
    QAction * get_action_deviation() const { return action_deviation; }
    QAction * get_action_arrow() const { return action_arrow; }
    QAction * get_action_pen() const { return action_pen; }

    QToolBar * get_toolbar() const { return radiant_toolbar; }

public slots:
    // A. Tool Selection
    void select_tool_browse();
    void select_tool_wl();
    void select_tool_pan();
    void select_tool_zoom();
    void select_tool_length();
    void select_tool_ellipse();
    void select_tool_closed_poly();
    void select_tool_open_poly();
    void select_tool_angle();
    void select_tool_cobb();
    void select_tool_deviation();
    void select_tool_arrow();
    void select_tool_pen();

    // B. Presets & Window/Level
    void apply_preset_0();
    void apply_preset_1();
    void apply_preset_2();
    void apply_preset_3();
    void apply_preset_4();
    void apply_preset_5();
    void apply_preset_6();
    void apply_preset_7();
    void apply_preset_8();
    void apply_preset_9();
    void toggle_invert_lut();
    void show_custom_wl_dialog();

    // C. Navigation & Cine
    void prev_image();
    void next_image();
    void prev_10_images();
    void next_10_images();
    void prev_series();
    void next_series();
    void first_image_or_series();
    void last_image_or_series();
    void next_panel();
    void prev_panel();
    void toggle_cine();

    // C. Zoom & Pan
    void zoom_fit();
    void zoom_100();
    void zoom_200();
    void zoom_400();
    void zoom_800();
    void zoom_in();
    void zoom_out();
    void pan_up();
    void pan_down();
    void pan_left();
    void pan_right();

    // C. Transformations & Filters
    void rotate_90_ccw();
    void rotate_90_cw();
    void flip_horizontal();
    void flip_vertical();
    void clear_geometric_transforms();
    void toggle_sharpen();
    void disable_sharpen();

    // D. Interface & Overlays
    void toggle_hud();
    void toggle_patient_id_hud();
    void toggle_cross_reference();
    void toggle_measurements();
    void toggle_series_sync();
    void toggle_split_screen();
    void close_split_screen();
    void maximize_active_panel();
    void toggle_full_screen();
    void toggle_distraction_free();
    void toggle_thumbnail_strip();
    void toggle_top_toolbar();
    void trigger_mpr_coronal();
    void trigger_mpr_sagittal();
    void trigger_mpr_axial();
    void trigger_3d_vr();
    void trigger_fusion();
    void trigger_dicom_tags();
    void trigger_open_file();
    void trigger_scan_folder();

    // D. ROIs deletion
    void delete_selected_roi();
    void delete_all_rois_image();
    void delete_all_rois_series();
    void delete_all_rois_all();

    // D. MPR 3D Mode Shortcuts
    void mpr3d_toggle_mode();
    void mpr3d_toggle_axes();
    void mpr3d_center_axes();
    void mpr3d_layout();
    void mpr3d_slab_thickness();
    void mpr3d_increase_slab();
    void mpr3d_decrease_slab();
    void mpr3d_rotate();
    void mpr3d_roll();
    void mpr3d_maximize();
    void mpr3d_reset();

private slots:
    void on_dispatcher_tool_changed(RadiantTool tool);

private:
    void setup_shortcut(const QKeySequence & seq, const char * slot, bool autoRepeat = true);

    MainWindow   * mw{nullptr};
    QToolBar     * radiant_toolbar{nullptr};
    QActionGroup * tool_action_group{nullptr};

    // Tool actions
    QAction * action_browse{nullptr};
    QAction * action_wl{nullptr};
    QAction * action_pan{nullptr};
    QAction * action_zoom{nullptr};
    QAction * action_length{nullptr};
    QAction * action_ellipse{nullptr};
    QAction * action_closed_poly{nullptr};
    QAction * action_open_poly{nullptr};
    QAction * action_angle{nullptr};
    QAction * action_cobb{nullptr};
    QAction * action_deviation{nullptr};
    QAction * action_arrow{nullptr};
    QAction * action_pen{nullptr};

    // Direct toolbar buttons for quick access
    QAction * action_open_file{nullptr};
    QAction * action_open_dir{nullptr};
    QAction * action_preset_0{nullptr};
    QAction * action_preset_1{nullptr};
    QAction * action_preset_2{nullptr};
    QAction * action_preset_3{nullptr};
    QAction * action_preset_4{nullptr};
    QAction * action_preset_5{nullptr};
    QAction * action_preset_6{nullptr};
    QAction * action_preset_7{nullptr};
    QAction * action_invert_lut{nullptr};
    QAction * action_sync{nullptr};
    QAction * action_cine{nullptr};
    QAction * action_split{nullptr};
    QAction * action_mpr_axial{nullptr};
    QAction * action_3d_vr{nullptr};

    // State cache
    bool is_distraction_free{false};
    bool cached_menu_visible{true};
    bool cached_dock_visible{true};
    bool cached_toolbar2_visible{true};
    bool cached_toolbar4_visible{true};
    bool cached_toolbar5_visible{true};
    bool cached_status_visible{true};
    bool cached_3d_frame_visible{false};
    bool is_panel_maximized{false};
};

#endif // RADIANTSHORTCUTSMANAGER_H
