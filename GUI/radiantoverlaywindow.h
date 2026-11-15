#ifndef RADIANTOVERLAYWINDOW_H
#define RADIANTOVERLAYWINDOW_H

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QCloseEvent>
#include <QToolBar>
#include <QToolButton>
#include <QAction>
#include <QActionGroup>
#include <QMenu>
#include "common/mprlayout.h"

class MainWindow;

class RadiantOverlayWindow : public QWidget
{
    Q_OBJECT
public:
    enum class OverlayType {
        Generic,
        MPR,
        VR_3D
    };

    explicit RadiantOverlayWindow(
        const QString & title,
        QWidget * contentWidget,
        MainWindow * parentWindow,
        OverlayType type = OverlayType::Generic);
    ~RadiantOverlayWindow() override;

    void set_content(QWidget * contentWidget);
    QWidget * take_content();

    QToolBar * get_toolbar() const { return toolbar; }
    OverlayType get_overlay_type() const { return overlay_type; }

protected:
    void closeEvent(QCloseEvent * event) override;
    bool eventFilter(QObject * watched, QEvent * event) override;

signals:
    void window_closed();

public slots:
    // MPR Dedicated Toolbar Actions
    void on_mpr_wl_preset(int preset);
    void on_mpr_pan_triggered();
    void on_mpr_zoom_triggered();
    void on_mpr_zoom_fit();
    void on_mpr_zoom_factor(double factor);
    void on_mpr_ref_lines_toggled(bool checked);
    void on_mpr_slab_mip_thickness(int mode, double thickness);
    void on_mpr_reset_planes();
    void on_mpr_layout_switched(int layoutIndex);
    void on_mpr_export();

    // 3D VR Dedicated Toolbar Actions
    void on_3d_preset_triggered(int preset);
    void on_3d_trackball_triggered();
    void on_3d_pan_triggered();
    void on_3d_zoom_triggered();
    void on_3d_scalpel_toggled(bool checked);
    void on_3d_anatomical_view(int viewIndex);
    void on_3d_reset_orientation();
    void on_3d_snapshot_export();

private:
    void setup_toolbar();
    void setup_mpr_toolbar();
    void apply_mpr_layout();
    void setup_3d_toolbar();
    QToolButton * create_menu_button(QAction * act, QMenu * menu, bool instantPopup = false);

    MainWindow   * mw{nullptr};
    QWidget      * content{nullptr};
    QVBoxLayout  * main_layout{nullptr};
    QToolBar     * toolbar{nullptr};
    OverlayType    overlay_type{OverlayType::Generic};
    mpr_layout::State mpr_layout_state;
    QActionGroup * tool_action_group{nullptr};

    // MPR Actions
    QAction * act_mpr_wl{nullptr};
    QAction * act_mpr_pan{nullptr};
    QAction * act_mpr_zoom{nullptr};
    QAction * act_mpr_ref_lines{nullptr};
    QAction * act_mpr_slab_mip{nullptr};
    QAction * act_mpr_reset_planes{nullptr};
    QAction * act_mpr_layout{nullptr};
    QAction * act_mpr_export{nullptr};

    // 3D Actions
    QAction * act_3d_preset{nullptr};
    QAction * act_3d_trackball{nullptr};
    QAction * act_3d_pan{nullptr};
    QAction * act_3d_zoom{nullptr};
    QAction * act_3d_scalpel{nullptr};
    QAction * act_3d_views{nullptr};
    QAction * act_3d_reset_orientation{nullptr};
    QAction * act_3d_snapshot{nullptr};
};

#endif // RADIANTOVERLAYWINDOW_H
