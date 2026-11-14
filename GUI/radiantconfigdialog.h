#ifndef RADIANTCONFIGDIALOG_H
#define RADIANTCONFIGDIALOG_H

#include <QDialog>
#include <QTabWidget>
#include <QCheckBox>
#include <QRadioButton>
#include <QButtonGroup>
#include <QPushButton>
#include <QTableWidget>
#include <QLabel>

class MainWindow;
class Aliza;

struct RadiantHudConfig {
    bool show_patient_name{true};
    bool show_patient_id{true};
    bool show_study_date{true};
    bool show_series_desc{true};
    bool show_institution{true};
    bool show_modality{true};
    bool show_tech_params{true};
    bool show_thickness_loc{true};
    bool show_zoom{true};
    bool show_hu_probe{true};
};

class RadiantConfigDialog : public QDialog
{
    Q_OBJECT
public:
    explicit RadiantConfigDialog(MainWindow * parentWindow, QWidget * parent = nullptr);
    ~RadiantConfigDialog() override = default;

    static RadiantHudConfig & get_hud_config();

private slots:
    void on_apply_clicked();
    void on_save_clicked();

private:
    MainWindow * mw{nullptr};

    // HUD Checkboxes
    QCheckBox * chk_pat_name{nullptr};
    QCheckBox * chk_pat_id{nullptr};
    QCheckBox * chk_study_date{nullptr};
    QCheckBox * chk_series_desc{nullptr};
    QCheckBox * chk_institution{nullptr};
    QCheckBox * chk_modality{nullptr};
    QCheckBox * chk_tech_params{nullptr};
    QCheckBox * chk_thickness_loc{nullptr};
    QCheckBox * chk_zoom{nullptr};
    QCheckBox * chk_hu_probe{nullptr};

    // Thumbnail strip radio buttons
    QRadioButton * radio_dock_left{nullptr};
    QRadioButton * radio_dock_right{nullptr};
    QRadioButton * radio_dock_top{nullptr};
    QRadioButton * radio_dock_bottom{nullptr};
    QCheckBox    * chk_dock_visible{nullptr};

    QPushButton * btn_save{nullptr};
    QPushButton * btn_cancel{nullptr};
    QPushButton * btn_apply{nullptr};
};

#endif // RADIANTCONFIGDIALOG_H
