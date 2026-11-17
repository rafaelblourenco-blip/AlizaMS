#ifndef A_GRAPHICSWIDGET_H
#define A_GRAPHICSWIDGET_H

#include "graphicsview.h"
#include "structures.h"
#include "toolbox2D.h"
#include "sliderwidget.h"
#include <QWidget>
#include <QLabel>
#include <QList>
#include <QVector>
#include <QMap>
#include <QLineEdit>
#include <QThread>
#include <QPointF>
#include <QPen>
#include <QTimer>
#include <QMouseEvent>
#include <QEvent>
#include <QCloseEvent>
#include <QProgressDialog>
#include <QCache>
#include <memory>

class QGraphicsPathItem;
class Aliza;
class SlabScheduler;
struct MprSharedState;

class GraphicsWidget : public QWidget
{
Q_OBJECT
	friend class Aliza;
	friend class GraphicsView;
	friend class RadiantInputDispatcher;
	friend class RadiantShortcutsManager;
public:
	GraphicsWidget(
		short      /*axis*/,
		bool       /*OpenGL*/,
		QLabel*    /*top*/,
		QLabel*    /*left*/,
		QLabel*    /*measurement*/,
		QLineEdit* /*value*/,
		QWidget*   /*sinle frame*/,
		QWidget*   /*multi frame*/);
	~GraphicsWidget();
	void set_axis(int); // 0 x, 1 y, 2 z
	void set_top_label(QLabel*);
	void set_left_label(QLabel*);
	void set_measure_label(QLabel*);
	void set_info_line(QLineEdit*);
	void set_aliza(Aliza*);
	Aliza * get_aliza() const { return aliza; }
	int  get_axis() const { return axis; }
	void start_animation();
	void stop_animation();
	GraphicsView * graphicsview;
	ToolBox2D    * toolbox2D{};
	SliderWidget * slider_m{};
	void set_slice_2D(
		ImageVariant*,
		const short /*fit*/,
		const bool  /*alw usregions*/,
		const bool = false /*frame level, to avoid check map twice*/);
	void set_toolbox2D_widget(ToolBox2D*);
	void set_sliderwidget(SliderWidget*);
	void update_image(
		const short /*fit*/,
		const bool  /*redraw_contours*/,
		const bool = false /*frame level, to avoid check map twice*/);
	void clear_();
	void update_frames();
	void set_mpr_state(const std::shared_ptr<MprSharedState> & state) { m_mpr_state = state; }
	const std::shared_ptr<MprSharedState> & mpr_state() const { return m_mpr_state; }
	void set_reference_lines_visible(bool);
	void clear_slice_pixmap_cache();
	QCache<QString, QPixmap> m_slice_pixmap_cache;
	ImageContainer image_container;
	float get_offset_x();
	float get_offset_y();
	QString labels_to_contours(
		QProgressDialog*,
		int,int,
		const QMap< int, QVector<int> > &,
		const QMap< int, QString> &);
	void  set_bb(bool);
	bool  get_bb() const;
	bool  run__{};
	void  update_selection_rectangle();
	void  update_pr_area();
	void  update_selection_item();
	void  set_top_label_text(const QString&);
	void  set_left_label_text(const QString&);
	void  set_info_line_text(const QString&);
	void  set_measure_text(const QString&);
	void  update_reconstructed_geometry(ImageVariant*);
	void  update_background_color();
	void  set_main();
	bool  is_main() const;
	void  set_multiview(bool);
	bool  is_multiview() const;
	void  set_smooth(bool);
	bool  get_smooth() const;
	void  update_measurement(double, double, double, double);
	void  set_mouse_modus(short,bool);
	short get_mouse_modus() const;
	void  set_show_cursor(bool);
	bool  get_show_cursor() const;
	void  update_pixel_value(double, double);
	void  update_pixel_value2(double, double);
	void  set_alt_mode(bool);
	bool  get_alt_mode() const;
	int   get_frametime_2D() const;
	QString contours_from_selected_paths(ImageVariant*, ROI*);
	double get_contours_width() const;
	bool get_enable_shutter() const;
	bool get_enable_overlays() const;
	void set_enable_shutter(bool);
	void set_enable_overlays(bool);
	void set_oblique_slice_2D(
		ImageVariant * v,
		double angle,
		short target_plane,
		short source_plane,
		bool fastDraftMode = false);
	void reset_oblique_slice();
	void recover_mpr_failure();
	bool handle_mpr_slider(int);
	bool handle_mpr_point(double, double);
	bool set_slab_projection(int mode, double thickness_mm);
	const Image2DTypeUC::Pointer & slab_mask() const { return m_slab_mask; }
	int slab_mode() const { return m_slab_mode; }
	double slab_thickness() const { return m_slab_thickness_mm; }
	bool slab_pending() const { return m_slab_pending; }
	QString slab_error() const { return m_slab_error; }
	QString mpr_backend() const { return m_mpr_backend; }
	bool is_oblique() const { return m_is_oblique_active; }
	double get_oblique_angle() const { return m_oblique_angle; }

public slots:
	void set_frame_time_unit(bool);
	void get_screen();
	void set_frametime_2D(int);
	void show_image_info();
	void set_contours_width(double);

private slots:
	void animate_();

signals:
	void slice_changed(int);

protected:
	void closeEvent(QCloseEvent*) override;
	void leaveEvent(QEvent*) override;

private:
	std::shared_ptr<MprSharedState> m_mpr_state;
	short  axis;
	bool   main{};
	bool   multi{};
	bool   bb{};
	bool   smooth_{true};
	bool   gl{};
	short  mouse_modus{};
	bool   enable_shutter{true};
	bool   enable_overlays{true};
	bool   reference_lines_visible{false};
	int    frame_time_unit{}; // 0 - ms; 1 - s
	int    frametime_2D{120};
	double contours_width{};
	QTimer    * anim2D_timer;
	QLabel    * top_label;
	QLabel    * left_label;
	QLabel    * measure_label;
	QLineEdit * info_line;
	Aliza     * aliza{};
	QWidget * single_frame_ptr;
	QWidget * multi_frame_ptr;
	bool alt_mode{};
	bool show_cursor{};
	bool m_is_oblique_active{false};
	double m_oblique_angle{0.0};
	short m_oblique_source_plane{2};
	int m_slab_mode{0};
	double m_slab_thickness_mm{0.0};
	Image2DTypeUC::Pointer m_slab_mask;
	SlabScheduler *m_slab_scheduler{nullptr};
	bool m_slab_pending{false};
	QString m_slab_error;
	QString m_mpr_backend{QStringLiteral("CPU")};
	void cancel_slab_workers();
	void update_mpr_state(ImageVariant*);
	bool update_projected_pixel_value(double, double);
};

#endif

