#ifndef A_STUDYVIEWWIDGET_H
#define A_STUDYVIEWWIDGET_H

#include "ui_studyviewwidget.h"
#include <QList>
#include <QIcon>
#include <QToolButton>
#include <QCloseEvent>
#include <QSettings>
#include <QShortcut>
#include <QTimer>
#include <QDateTime>

class StudyFrameWidget;
class StudyGraphicsWidget;
class ImageVariant;
class MatrixButton;
class ImageContainer;
class LUTWidget;
class Aliza;
class QMenu;

class StudyViewWidget : public QWidget, public Ui::StudyViewWidget
{

Q_OBJECT
	friend class RadiantShortcutsManager;

public:
	enum SeriesSyncMode {
		SyncModeOff = 0,
		SyncModeAuto = 1,
		SyncModeManual = 2
	};
	enum PlaneType {
		PlaneUnknown = 0,
		PlaneSagittal = 1,
		PlaneCoronal = 2,
		PlaneAxial = 3
	};
	static PlaneType get_plane_type(const ImageVariant * iv);

	StudyViewWidget(bool, bool, float);
	~StudyViewWidget();
	void init_(Aliza*);
	void clear_();
	void invalidate_reference_lines();
	bool get_in_tab() const;
	void calculate_grid(int);
	int  get_active_id() const;
	void set_active_id(int);
	bool get_scouts() const;
	void set_active_image(int);
	void update_full(ImageContainer*);
	void update_level(ImageContainer*);
	void connect_tools();
	void disconnect_tools();
	void block_signals(bool);
	void update_null();
	void update_scouts();
	void update_scouts_now();
	bool get_anchored_sliders() const;
	void update_all_sliders(int, int, int);
	void set_single(int);
	void restore_multi();
	bool is_single_mode() const { return saved_r != -1 && saved_c != -1; }
	void toggle_single_mode(int wid);
	SeriesSyncMode get_sync_mode() const { return sync_mode; }
	void set_sync_mode(SeriesSyncMode mode);
	void cycle_sync_mode();
	bool get_sync_slice_position() const { return sync_slice_position; }
	void set_sync_slice_position(bool s) { sync_slice_position = s; }
	bool get_sync_zoom_pan() const { return sync_zoom_pan; }
	void set_sync_zoom_pan(bool s) { sync_zoom_pan = s; }
	bool get_sync_window_settings() const { return sync_window_settings; }
	void set_sync_window_settings(bool s) { sync_window_settings = s; }
	void sync_zoom_pan_from(int wid, double scale, const QPointF & center);
	void sync_window_from(int wid, double center, double width);
	void writeSettings(QSettings&);
	void update_background_color();
	QMenu * create_toolbar_menu(QWidget * parent);
	QAction * fitall_Action;
	QAction * scouts_Action;
	QAction * measure_Action;
	QAction * anchor_Action;
	QList<StudyFrameWidget *> widgets;

public slots:
	void update_grid(int, int);
	void load_series_split(int series_id);
	void close_viewport(StudyFrameWidget * frame);
	void update_grid2();
	void set_next_panel();
	void set_prev_panel();
	void toggle_sync_series();

protected:
	void closeEvent(QCloseEvent*) override;

private slots:
	void set_center_slider(int);
	void set_width_slider(int);
	void set_center_spinbox(double);
	void set_width_spinbox(double);
	void set_lut_function(int);
	void set_lut(int);
	void toggle_lock_window(bool);
	void reset_level();
	void all_to_fit();
	void all_to_original();
	void toggle_scouts(bool);
	void toggle_measure(bool);
	void check_close();

signals:
	void update_scouts_required();

private:
	void update_locked_window(bool);
	void update_lut_function(int);
	void update_spinbox_center_d(double);
	void update_spinbox_width_d(double);
	void update_window_upper(double);
	void update_window_lower(double);
	void update_max_width(double);
	void update_locked_center(double);
	void update_locked_width(double);
	void readSettings();
	const bool horizontal;
	const bool in_tab;
	MatrixButton * mbutton;
	QAction * close_Action;
	LUTWidget * lutwidget;
	Aliza * aliza{};
	QIcon lockon;
	QIcon lockoff;
	int active_id{-1};
	int saved_r{-1};
	int saved_c{-1};
	SeriesSyncMode sync_mode{SyncModeAuto};
	bool sync_slice_position{true};
	bool sync_zoom_pan{true};
	bool sync_window_settings{false};
	int prev_slice[25]{};
	QTimer * m_scouts_debounce_timer{nullptr};
	qint64 m_scouts_last_update_ms{0};
	QShortcut * close_sc;
#ifdef __APPLE__
	QShortcut * minimaze_sc;
	QShortcut * fullsceen_sc;
	QShortcut * normal_sc;
#endif
};

#endif

