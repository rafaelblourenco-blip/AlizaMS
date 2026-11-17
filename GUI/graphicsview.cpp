#define _USE_MATH_DEFINES
#include <cmath>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#include "radiantconfigdialog.h"
#include "graphicsview.h"
#include "common/mprgeometry.h"
#include "common/mprmeasurementgeometry.h"
#include "mprstate.h"
#include "common/mprslabitk.h"
#include "graphicswidget.h"
#include "aliza.h"
#include "radiantinputdispatcher.h"
#include <QScrollBar>
#include <QtGlobal>
#include <QColor>
#include <QRectF>
#include <QStyleOptionButton>
#include <QTimer>
#include <QGraphicsSceneMouseEvent>
#include <QApplication>
#include <QClipboard>
#include <QList>
#include <QGraphicsDropShadowEffect>
#include "contourutils.h"
#include "colorspace/colorspace.h"

GraphicsView::GraphicsView(GraphicsWidget * p) : parent(p)
{
	setAcceptDrops(false);
	setMouseTracking(true);
	setFrameStyle(QFrame::Plain);
	setFrameShape(QFrame::NoFrame);
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	QGraphicsScene * scene_ = new QGraphicsScene(this);
	scene_->setItemIndexMethod(QGraphicsScene::NoIndex);
	const QRectF rectf(0, 0, 100, 100);
	scene_->setSceneRect(rectf);
	setScene(scene_);
	QColor bc(Qt::black);
	setBackgroundBrush(QBrush(bc));
	shutter_color = std::move(bc);
	//setCacheMode(CacheNone);
	setRenderHints(QPainter::Antialiasing);//QPainter::SmoothPixmapTransform);
#ifdef DELETE_GRAPHICSIMAGEITEM
	image_item = nullptr;
#else
	image_item = new QGraphicsPixmapItem();
	image_item->setShapeMode(QGraphicsPixmapItem::BoundingRectShape);
	if (parent && !parent->get_smooth())
	{
		image_item->setTransformationMode(Qt::FastTransformation);
	}
	else
	{
		image_item->setTransformationMode(Qt::SmoothTransformation);
	}
	image_item->setCacheMode(QGraphicsItem::NoCache);
	image_item->setPos(0, 0);
	image_item->setZValue(-1.0);
	scene_->addItem(image_item);
#endif
	setViewportUpdateMode(FullViewportUpdate);
	setTransformationAnchor(AnchorUnderMouse);
	setDragMode(QGraphicsView::ScrollHandDrag);
	//
	handle_rect = new RectItem(0, 0, 100, 100);
	topHandle = new HandleItem(this, handle_rect, HandleItem::TopHandle);
	topHandle->setParentItem(handle_rect);
	rightHandle = new HandleItem(this, handle_rect, HandleItem::RightHandle);
	rightHandle->setParentItem(handle_rect);
	leftHandle = new HandleItem(this, handle_rect, HandleItem::LeftHandle);
	leftHandle->setParentItem(handle_rect);
	bottomHandle = new HandleItem(this, handle_rect, HandleItem::BottomHandle);
	bottomHandle->setParentItem(handle_rect);
	handle_rect->setZValue(1e+19 - 5);
	handle_rect->hide();
	scene_->addItem(handle_rect);
	//
	selection_item = new RectItem(0, 0, 100, 100);
	selection_item->set_pen2(1.0);
	selection_item->setZValue(1e+19 - 5);
	scene_->addItem(selection_item);
	selection_item->hide();
	//
	QPen xpen;
	xpen.setBrush(QBrush(QColor(255, 0, 0, 255)));
	xpen.setWidth(0);
	xpen.setStyle(Qt::DashLine);
	line_x = new QGraphicsPathItem();
	line_x->setPen(xpen);
	line_x->setZValue(1e+19 - 4);
	line_x->hide();
	scene_->addItem(line_x);
	//
	QPen ypen;
	ypen.setBrush(QBrush(QColor(0, 255, 0, 255)));
	ypen.setWidth(0);
	ypen.setStyle(Qt::DashLine);
	line_y = new QGraphicsPathItem();
	line_y->setPen(ypen);
	line_y->setZValue(1e+19 - 3);
	line_y->hide();
	scene_->addItem(line_y);
	//
	QPen zpen;
	zpen.setBrush(QBrush(QColor(0, 0, 255, 255)));
	zpen.setWidth(0);
	zpen.setStyle(Qt::DashLine);
	line_z = new QGraphicsPathItem();
	line_z->setPen(zpen);
	line_z->setZValue(1e+19 - 2);
	line_z->hide();
	scene_->addItem(line_z);
	//
	measurment_line = new QGraphicsPathItem();
	measurment_line->setZValue(1e+19 - 1);
	scene_->addItem(measurment_line);
	//
	pr_area = new QGraphicsRectItem(0, 0, 100, 100);
	pr_area->setFlag(QGraphicsItem::ItemIsMovable, false);
	pr_area->setFlag(QGraphicsItem::ItemIsFocusable, false);
	pr_area->setAcceptHoverEvents(false);
#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
	pr_area->setAcceptedMouseButtons(Qt::NoButton);
#else
	pr_area->setAcceptedMouseButtons(0);
#endif
	pr_area->setZValue(1e+19 - 6);
	pr_area->hide();
	QBrush pbrush(QColor(146, 90, 248));
	QPen ppen;
	ppen.setBrush(pbrush);
	ppen.setWidth(0);
	ppen.setStyle(Qt::SolidLine);
	pr_area->setPen(ppen);
	scene_->addItem(pr_area);
	//
	paint_brush = new QGraphicsPathItem();
	paint_brush->setFlag(QGraphicsItem::ItemIsMovable, false);
	paint_brush->setFlag(QGraphicsItem::ItemIsFocusable, false);
	paint_brush->setAcceptHoverEvents(false);
#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
	paint_brush->setAcceptedMouseButtons(Qt::NoButton);
#else
	paint_brush->setAcceptedMouseButtons(0);
#endif
	QPen bpen;
	bpen.setBrush(QBrush(QColor(Qt::blue)));
	bpen.setWidth(0);
	bpen.setStyle(Qt::SolidLine);
	bpen.setCapStyle(Qt::RoundCap);
	bpen.setJoinStyle(Qt::RoundJoin);
	paint_brush->setPen(bpen);
	paint_brush->setZValue(1e+19);
	paint_brush->hide();
	scene_->addItem(paint_brush);
}

GraphicsView::~GraphicsView()
{
	clear_paths();
	clear_collision_paths();
	clear_us_regions();
	clear_prtexts_items();
	clear_prgraphicobjects_items();
	clear_shutters();
}

void GraphicsView::keyPressEvent(QKeyEvent * e)
{
	const bool ctrl = (e->modifiers() & Qt::ControlModifier);

	if (ctrl)
	{
		switch (e->key())
		{
		case Qt::Key_C:
			{
				QPixmap pix = viewport()->grab();
				if (!pix.isNull())
				{
					QApplication::clipboard()->setPixmap(pix);
					QApplication::clipboard()->setImage(pix.toImage());
				}
				e->accept();
				return;
			}
		case Qt::Key_0:
			// Fit to panel (Ctrl+0)
			parent->update_image(1, false);
			update_selection_rect_width();
			return;
		case Qt::Key_1:
			// Zoom 100% (1:1) (Ctrl+1)
			m_scale = 1.0;
			setTransform(QTransform());
			update_selection_rect_width();
			return;
		case Qt::Key_2:
			// Zoom 200% (Ctrl+2)
			m_scale = 2.0;
			setTransform(QTransform().scale(2.0, 2.0));
			update_selection_rect_width();
			return;
		case Qt::Key_3:
			// Zoom 400% (Ctrl+3)
			m_scale = 4.0;
			setTransform(QTransform().scale(4.0, 4.0));
			update_selection_rect_width();
			return;
		case Qt::Key_4:
			// Zoom 800% (Ctrl+4)
			m_scale = 8.0;
			setTransform(QTransform().scale(8.0, 8.0));
			update_selection_rect_width();
			return;
		case Qt::Key_F11:
			if (parent && parent->get_aliza())
			{
				parent->get_aliza()->show_custom_wl_dialog(this);
			}
			return;
		default:
			break;
		}
	}
	else
	{
		switch (e->key())
		{
		case Qt::Key_0:
			if (parent && parent->get_aliza()) parent->get_aliza()->apply_wl_preset(0);
			return;
		case Qt::Key_1:
			if (parent && parent->get_aliza()) parent->get_aliza()->apply_wl_preset(1);
			return;
		case Qt::Key_2:
			if (parent && parent->get_aliza()) parent->get_aliza()->apply_wl_preset(2);
			return;
		case Qt::Key_3:
			if (parent && parent->get_aliza()) parent->get_aliza()->apply_wl_preset(3);
			return;
		case Qt::Key_4:
			if (parent && parent->get_aliza()) parent->get_aliza()->apply_wl_preset(4);
			return;
		case Qt::Key_5:
			if (parent && parent->get_aliza()) parent->get_aliza()->apply_wl_preset(5);
			return;
		case Qt::Key_6:
			if (parent && parent->get_aliza()) parent->get_aliza()->apply_wl_preset(6);
			return;
		case Qt::Key_7:
			if (parent && parent->get_aliza()) parent->get_aliza()->apply_wl_preset(7);
			return;
		case Qt::Key_8:
			if (parent && parent->get_aliza()) parent->get_aliza()->apply_wl_preset(8);
			return;
		case Qt::Key_9:
			if (parent && parent->get_aliza()) parent->get_aliza()->apply_wl_preset(9);
			return;
		case Qt::Key_F11:
			if (parent && parent->get_aliza()) parent->get_aliza()->toggle_invert_lut();
			return;
		default:
			break;
		}
	}

	switch (e->key())
	{
	case Qt::Key_Plus:
	case Qt::Key_Equal:
		zoom_in();
		break;
	case Qt::Key_Minus:
		zoom_out();
		break;
	case Qt::Key_X:
		flipX();
		break;
	case Qt::Key_Y:
		flipY();
		break;
	case Qt::Key_G:
		parent->get_screen();
		break;
	case Qt::Key_P:
		parent->show_image_info();
		break;
	case Qt::Key_S:
		{
			const bool t = parent->get_smooth();
			parent->set_smooth(!t);
		}
		break;
	default:
		QGraphicsView::keyPressEvent(e);
		break;
	}
}

void GraphicsView::keyReleaseEvent(QKeyEvent * e)
{
	switch (e->key())
	{
	case Qt::Key_Shift :
	default:
		break;
	}
	QGraphicsView::keyReleaseEvent(e);
}

void GraphicsView::wheelEvent(QWheelEvent * e)
{
	if (RadiantInputDispatcher::instance()->handle_wheel(this, e, parent))
	{
		return;
	}
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
	scale_view(pow(2.0, -e->angleDelta().y() / 480.0));
#else
	scale_view(pow(2.0, -e->delta() / 480.0));
#endif
}

void GraphicsView::drawBackground(QPainter * painter, const QRectF & rect)
{
	QGraphicsView::drawBackground(painter, rect);
}

bool GraphicsView::s_show_hud = true;
bool GraphicsView::s_anonymous_mode = false;

void GraphicsView::toggle_hud_display()
{
	s_show_hud = !s_show_hud;
}

void GraphicsView::toggle_anonymous_display()
{
	s_anonymous_mode = !s_anonymous_mode;
}

bool GraphicsView::is_hud_visible()
{
	return s_show_hud;
}

bool GraphicsView::is_anonymous_mode()
{
	return s_anonymous_mode;
}

void GraphicsView::drawForeground(QPainter * painter, const QRectF & rect)
{
	Q_UNUSED(rect);
	if (!parent || !parent->image_container.image3D) {
		if (!extra_points.isEmpty() || !saved_measure_paths.isEmpty()) clear_extra_measurements();
		return;
	}
	if ((!extra_points.isEmpty() || !saved_measure_paths.isEmpty()) && parent->image_container.image3D->di)
	{
		const auto *di = parent->image_container.image3D->di;
		const auto state=parent->mpr_state();
		const int axis=parent->get_axis();
		bool changed=extra_volume != parent->image_container.image3D || !state || !state->enabled || !state->initialized || axis<0 || axis>=3;
		if (!changed) {
			const auto &p=state->planes.planes[axis];
			const double now[]={p.center.x,p.center.y,p.center.z,p.u.x,p.u.y,p.u.z,p.v.x,p.v.y,p.v.z};
			for (int i=0;i<9;++i) if (std::abs(extra_plane[i]-now[i])>1e-9) changed=true;
		}
		if (changed || extra_slice[0] != di->selected_x_slice || extra_slice[1] != di->selected_y_slice ||
			extra_slice[2] != di->selected_z_slice || extra_ref_h != m_ref_angle_h ||
			extra_ref_v != m_ref_angle_v || extra_slab_mode != parent->slab_mode() ||
			extra_slab_thickness != parent->slab_thickness()) clear_extra_measurements();
	}
	if (parent->slab_mode() != 0 || parent->is_oblique() || !parent->slab_error().isEmpty())
	{
		painter->save();
		painter->resetTransform();
		// A previous or thin slice must never be presented as the requested projection.
		if (parent->slab_pending() || !parent->slab_error().isEmpty())
			painter->fillRect(viewport()->rect(), Qt::black);
		const QString mode = parent->slab_mode() == 1 ? QString("MIP") :
			(parent->slab_mode() == 2 ? QString("MinIP") :
			(parent->slab_mode() == 3 ? QString("Average") : QString("Obliquo")));
		const QString label = !parent->slab_error().isEmpty() ? parent->slab_error() :
			(parent->slab_mode() == 0 ? mode : QString("%1 %2 mm%3").arg(mode)
				.arg(parent->slab_thickness(), 0, 'f', 1)
				.arg(parent->slab_pending() ? QString(" - calculando") : QString()));
        const QString backendLabel = label + QString(" [%1]").arg(parent->mpr_backend());
		const QRect r = viewport()->rect();
		const QRect labelRect(r.left(), r.top() + 4, r.width(), 22);
		painter->setPen(Qt::black);
		painter->drawText(labelRect.translated(1, 1), Qt::AlignHCenter | Qt::AlignVCenter, backendLabel);
		painter->setPen(Qt::white);
		painter->drawText(labelRect, Qt::AlignHCenter | Qt::AlignVCenter, backendLabel);
		painter->restore();
		if (parent->slab_pending() || !parent->slab_error().isEmpty()) return;
	}
	const auto & shared = parent->mpr_state();
	if (shared && shared->enabled && shared->initialized && parent->get_mouse_modus() == 1 &&
		(line_x->isVisible() || line_y->isVisible() || line_z->isVisible()))
	{
		painter->save();
		painter->resetTransform();
		painter->setPen(QPen(Qt::white, 1.5));
		painter->setBrush(QColor(7, 23, 40, 220));
		const QPointF handles[4] = {m_handle_h1_scene,m_handle_h2_scene,m_handle_v1_scene,m_handle_v2_scene};
		for (const QPointF &handle : handles) painter->drawEllipse(QPointF(mapFromScene(handle)), 4.0, 4.0);
		painter->restore();
	}
	if (!s_show_hud) return;

	const ImageVariant * iv = parent->image_container.image3D;
	if (!iv || !iv->di) return;

	painter->save();
	painter->resetTransform();

	const QRect vr = viewport()->rect();
	const int margin = 12;
	const int lineHeight = 16;
	QFont font = painter->font();
	font.setFamily("Segoe UI");
	font.setPointSize(9);
	font.setBold(true);
	painter->setFont(font);

	auto drawShadow = [&](int x, int y, const QString & text) {
		if (text.isEmpty()) return;
		painter->setPen(QColor(0, 0, 0, 240));
		painter->drawText(x + 1, y + 1, text);
		painter->drawText(x - 1, y + 1, text);
		painter->drawText(x + 1, y - 1, text);
		painter->drawText(x - 1, y - 1, text);
		painter->setPen(QColor(230, 230, 230));
		painter->drawText(x, y, text);
	};

	auto drawShadowRight = [&](int y, const QString & text) {
		if (text.isEmpty()) return;
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
		const int textWidth = painter->fontMetrics().horizontalAdvance(text);
#else
		const int textWidth = painter->fontMetrics().width(text);
#endif
		drawShadow(vr.right() - margin - textWidth, y, text);
	};

	const RadiantHudConfig & cfg = RadiantConfigDialog::get_hud_config();

	// 1. TOP-LEFT: Patient Name / ID (ocultável via Shift+F12), Data, Série
	int tl_y = vr.top() + margin + lineHeight;
	QString patText = s_anonymous_mode ? QString("[ANÔNIMO]") : (iv->pat_name.isEmpty() ? iv->pat_id : iv->pat_name);
	if (cfg.show_patient_name && !patText.isEmpty())
	{
		drawShadow(vr.left() + margin, tl_y, patText);
		tl_y += lineHeight;
	}

	if (cfg.show_patient_id && !s_anonymous_mode && !iv->pat_id.isEmpty())
	{
		drawShadow(vr.left() + margin, tl_y, QString("ID: ") + iv->pat_id);
		tl_y += lineHeight;
	}

	if (cfg.show_study_date && !iv->study_date.isEmpty())
	{
		drawShadow(vr.left() + margin, tl_y, iv->study_date);
		tl_y += lineHeight;
	}

	if (cfg.show_series_desc && !iv->series_description.isEmpty())
	{
		drawShadow(vr.left() + margin, tl_y, iv->series_description);
		tl_y += lineHeight;
	}

	// 2. TOP-RIGHT: Instituição, Modalidade, Parâmetros de aquisição
	int tr_y = vr.top() + margin + lineHeight;
	if (cfg.show_institution && !iv->institution.isEmpty())
	{
		drawShadowRight(tr_y, iv->institution);
		tr_y += lineHeight;
	}

	if (cfg.show_modality && !iv->modality.isEmpty())
	{
		drawShadowRight(tr_y, iv->modality);
		tr_y += lineHeight;
	}

	if (cfg.show_tech_params)
	{
		if (!iv->hardware_info.isEmpty())
		{
			drawShadowRight(tr_y, iv->hardware_info);
			tr_y += lineHeight;
		}
		else if (!iv->mrtimes.isEmpty())
		{
			drawShadowRight(tr_y, iv->mrtimes);
			tr_y += lineHeight;
		}
	}

	// 3. BOTTOM-LEFT: Im: X/Y, Posição (Loc: Z mm), Espessura (Thk: X mm), Zoom
	int bl_y = vr.bottom() - margin;
	if (cfg.show_zoom)
	{
		const int zoomPercent = static_cast<int>(std::round(transform().m11() * 100.0));
		drawShadow(vr.left() + margin, bl_y, QString("Zoom: %1%").arg(zoomPercent));
		bl_y -= lineHeight;
	}

	if (cfg.show_thickness_loc && parent->slab_mode() == 0 && !parent->is_oblique())
	{
		if (iv->di->iz_spacing > 0.0)
		{
			drawShadow(vr.left() + margin, bl_y, QString::asprintf("Thk: %.1f mm", iv->di->iz_spacing));
			bl_y -= lineHeight;
		}

		const double sliceLoc = iv->di->iz_origin + (iv->di->selected_z_slice * iv->di->iz_spacing);
		drawShadow(vr.left() + margin, bl_y, QString::asprintf("Loc: %.1f mm", sliceLoc));
		bl_y -= lineHeight;

		const int curSlice = iv->di->selected_z_slice + 1;
		const int totalSlices = std::max(1, iv->di->idimz);
		drawShadow(vr.left() + margin, bl_y, QString("Im: %1/%2").arg(curSlice).arg(totalSlices));
	}

	// 4. BOTTOM-RIGHT: WL: X WW: Y e leitura dinâmica de HU sob o cursor
	int br_y = vr.bottom() - margin;
	if (cfg.show_hu_probe && parent->info_line && !parent->info_line->text().isEmpty())
	{
		drawShadowRight(br_y, parent->info_line->text());
		br_y -= lineHeight;
	}

	const int curWL = static_cast<int>(std::round(iv->di->us_window_center));
	const int curWW = static_cast<int>(std::round(iv->di->us_window_width));
	drawShadowRight(br_y, QString("WL: %1  WW: %2").arg(curWL).arg(curWW));

	// Direct Measurement Values Beside Annotations:
	// Screen-space styled text badge positioned directly adjacent to annotation midpoint offset along normal vector
	QString measure_text;
	if (parent && parent->measure_label && !parent->measure_label->text().isEmpty())
	{
		measure_text = parent->measure_label->text();
	}

	bool has_measurement = false;
	QPointF p0_scene;
	QPointF p1_scene;

	if (m0_set && m1_set)
	{
		p0_scene = QPointF(m0_win_pos_x, m0_win_pos_y);
		p1_scene = QPointF(m1_win_pos_x, m1_win_pos_y);
		has_measurement = true;
	}
	else if (measurment_line && !measurment_line->path().isEmpty() &&
	         measurment_line->path().elementCount() >= 2)
	{
		const QPainterPath::Element el0 = measurment_line->path().elementAt(0);
		const QPainterPath::Element el1 = measurment_line->path().elementAt(1);
		p0_scene = QPointF(el0.x, el0.y);
		p1_scene = QPointF(el1.x, el1.y);
		has_measurement = true;
	}

	if (has_measurement && !measure_text.isEmpty())
	{
		const QPointF p0_screen = mapFromScene(p0_scene);
		const QPointF p1_screen = mapFromScene(p1_scene);
		const QPointF mid_screen = (p0_screen + p1_screen) * 0.5;

		const double dx = p1_screen.x() - p0_screen.x();
		const double dy = p1_screen.y() - p0_screen.y();
		const double length = std::hypot(dx, dy);

		QPointF normal(0.0, -1.0);
		if (length > 1e-4)
		{
			normal = QPointF(-dy / length, dx / length);
			if (normal.y() > 0.0)
			{
				normal = -normal;
			}
		}

		const double normal_offset = 18.0;
		const QPointF badge_center = mid_screen + normal * normal_offset;

		QFont badge_font("Segoe UI", 9, QFont::Bold);
		painter->setFont(badge_font);
		const QFontMetrics fm(badge_font);
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
		const int text_w = fm.horizontalAdvance(measure_text);
#else
		const int text_w = fm.width(measure_text);
#endif
		const int text_h = fm.height();
		const int pad_x = 8;
		const int pad_y = 4;
		const int badge_w = text_w + 2 * pad_x;
		const int badge_h = text_h + 2 * pad_y;

		QRectF badge_rect(badge_center.x() - badge_w / 2.0, badge_center.y() - badge_h / 2.0, badge_w, badge_h);

		if (badge_rect.left() < vr.left() + 4) badge_rect.moveLeft(vr.left() + 4);
		if (badge_rect.right() > vr.right() - 4) badge_rect.moveRight(vr.right() - 4);
		if (badge_rect.top() < vr.top() + 4) badge_rect.moveTop(vr.top() + 4);
		if (badge_rect.bottom() > vr.bottom() - 4) badge_rect.moveBottom(vr.bottom() - 4);

		painter->setRenderHint(QPainter::Antialiasing, true);

		// Rounded translucent dark badge (#071728 with 80% opacity)
		const QColor badge_bg(7, 23, 40, 204);
		// Crisp cyan/white contrast border (#38BDF8)
		const QColor badge_border(56, 189, 248);
		const QPen border_pen(badge_border, 1.5);
		painter->setPen(border_pen);
		painter->setBrush(QBrush(badge_bg));
		painter->drawRoundedRect(badge_rect, 4.0, 4.0);

		// Crisp white Segoe UI measurement text
		painter->setPen(QColor(255, 255, 255));
		painter->drawText(badge_rect, Qt::AlignCenter, measure_text);
	}


	painter->restore();
}

void GraphicsView::scale_view(double scale_factor)
{
	m_scale *= scale_factor;
	setTransform(transform().scale(scale_factor, scale_factor));
	update_selection_rect_width();
}

void GraphicsView::update_selection_rect_width()
{
	if (handle_rect->get_current_pen() == 2)
		handle_rect->set_pen2(3.0 / m_scale);
	else
		handle_rect->set_pen1(2.0 / m_scale);
	selection_item->set_pen2(1.0 / m_scale);
}

void GraphicsView::zoom_in()
{
	scale_view(1.015);
}

void GraphicsView::zoom_out()
{
	scale_view(1.0 / 1.015);
}

void GraphicsView::clear_paths()
{
	if (paths.empty()) return;
	for (int i = 0; i < paths.size(); ++i)
	{
		if (paths.at(i))
		{
			scene()->removeItem(static_cast<QGraphicsItem*>(paths[i]));
			delete paths[i];
			paths[i] = nullptr;
		}
	}
	paths.clear();
}

void GraphicsView::clear_collision_paths()
{
	if (collision_paths.empty()) return;
	for (int i = 0; i < collision_paths.size(); ++i)
	{
		if (collision_paths.at(i))
		{
			scene()->removeItem(static_cast<QGraphicsItem*>(collision_paths[i]));
			delete collision_paths[i];
			collision_paths[i] = nullptr;
		}
	}
	collision_paths.clear();
}

void GraphicsView::clear_us_regions()
{
	if (us_regions.empty()) return;
	for (int i = 0; i < us_regions.size(); ++i)
	{
		if (us_regions.at(i))
		{
			scene()->removeItem(static_cast<QGraphicsItem*>(us_regions[i]));
			delete us_regions[i];
			us_regions[i] = nullptr;
		}
	}
	us_regions.clear();
}

void GraphicsView::clear_prtexts_items()
{
	if (!prtexts.empty())
	{
		for (int i = 0; i < prtexts.size(); ++i)
		{
			if (prtexts.at(i))
			{
				scene()->removeItem(static_cast<QGraphicsItem*>(prtexts[i]));
				delete prtexts[i];
				prtexts[i] = nullptr;
			}
		}
		prtexts.clear();
	}
	if (!prtextanchors.empty())
	{
		for (int i = 0; i < prtextanchors.size(); ++i)
		{
			if (prtextanchors.at(i))
			{
				scene()->removeItem(static_cast<QGraphicsItem*>(prtextanchors[i]));
				delete prtextanchors[i];
				prtextanchors[i] = nullptr;
			}
		}
		prtextanchors.clear();
	}
}

void GraphicsView::clear_prgraphicobjects_items()
{
	if (!prpoints.empty())
	{
		for (int i = 0; i < prpoints.size(); ++i)
		{
			if (prpoints.at(i))
			{
				scene()->removeItem(static_cast<QGraphicsItem*>(prpoints[i]));
				delete prpoints[i];
				prpoints[i] = nullptr;
			}
		}
		prpoints.clear();
	}
	if (!prpolylines.empty())
	{
		for (int i = 0; i < prpolylines.size(); ++i)
		{
			if (prpolylines.at(i))
			{
				scene()->removeItem(static_cast<QGraphicsItem*>(prpolylines[i]));
				delete prpolylines[i];
				prpolylines[i] = nullptr;
			}
		}
		prpolylines.clear();
	}
	if (!printerpolated.empty())
	{
		for (int i = 0; i < printerpolated.size(); ++i)
		{
			if (printerpolated.at(i))
			{
				scene()->removeItem(static_cast<QGraphicsItem*>(printerpolated[i]));
				delete printerpolated[i];
				printerpolated[i] = nullptr;
			}
		}
		printerpolated.clear();
	}
	if (!prcircles.empty())
	{
		for (int i = 0; i < prcircles.size(); ++i)
		{
			if (prcircles.at(i))
			{
				scene()->removeItem(static_cast<QGraphicsItem*>(prcircles[i]));
				delete prcircles[i];
				prcircles[i] = nullptr;
			}
		}
		prcircles.clear();
	}
	if (!prellipses.empty())
	{
		for (int i = 0; i < prellipses.size(); ++i)
		{
			if (prellipses.at(i))
			{
				scene()->removeItem(static_cast<QGraphicsItem*>(prellipses[i]));
				delete prellipses[i];
				prellipses[i] = nullptr;
			}
		}
		prellipses.clear();
	}
}

void GraphicsView::clear_shutters()
{
	if (display_shutters.empty()) return;
	for (int i = 0; i < display_shutters.size(); ++i)
	{
		if (display_shutters.at(i))
		{
			scene()->removeItem(static_cast<QGraphicsItem*>(display_shutters[i]));
			delete display_shutters[i];
			display_shutters[i] = nullptr;
		}
	}
	display_shutters.clear();
}

void GraphicsView::mousePressEvent(QMouseEvent * e)
{
	const auto tool = RadiantInputDispatcher::instance()->get_tool();
	if (e->button() == Qt::LeftButton && parent->mpr_state() && parent->mpr_state()->enabled &&
		(tool == RadiantTool::Length || tool == RadiantTool::Angle || tool == RadiantTool::Ellipse ||
		 tool == RadiantTool::ClosedPolygon || tool == RadiantTool::Arrow))
	{
		if (parent->slab_pending() || !parent->slab_error().isEmpty()) return;
		const QPointF p = mapToScene(e->pos());
		if (!extra_measurement_point_valid(p)) return;
		const int t = static_cast<int>(tool);
		if (extra_complete || extra_tool != t)
		{
			if (extra_complete && extra_measure_path) {
				saved_measure_paths.push_back(extra_measure_path); saved_measure_labels.push_back(extra_measure_label);
				extra_measure_path=nullptr; extra_measure_label=nullptr;
			} else if (extra_measure_path) {
				delete extra_measure_path; delete extra_measure_label;
				extra_measure_path=nullptr; extra_measure_label=nullptr;
			}
			extra_points.clear(); extra_drag=false; extra_complete=false; extra_tool = t;
			const auto state=parent->mpr_state(); const int axis=parent->get_axis();
			if (!state || !state->initialized || axis<0 || axis>=3) { extra_tool=-1; return; }
			const auto &plane=state->planes.planes[axis];
			const double snapshot[]={plane.center.x,plane.center.y,plane.center.z,plane.u.x,plane.u.y,plane.u.z,plane.v.x,plane.v.y,plane.v.z};
			for (int i=0;i<9;++i) extra_plane[i]=snapshot[i];
			extra_volume=parent->image_container.image3D;
			const auto *di = parent->image_container.image3D->di;
			extra_slice[0]=di->selected_x_slice; extra_slice[1]=di->selected_y_slice; extra_slice[2]=di->selected_z_slice;
			extra_ref_h=m_ref_angle_h; extra_ref_v=m_ref_angle_v;
			extra_slab_mode=parent->slab_mode(); extra_slab_thickness=parent->slab_thickness();
		}
		if (tool == RadiantTool::Length || tool == RadiantTool::Ellipse || tool == RadiantTool::Arrow)
		{ extra_points.clear(); extra_points.push_back(p); extra_drag = true; }
		else { extra_points.push_back(p); if (tool == RadiantTool::Angle && extra_points.size() == 3) extra_complete = true; }
		update_extra_measurement(p, extra_complete);
		return;
	}
	if (e->button() == Qt::LeftButton && (parent->slab_pending() || !parent->slab_error().isEmpty())) return;
	if (parent->get_mouse_modus() == 1 && e->button() == Qt::LeftButton && e->modifiers() == Qt::NoModifier && check_ref_line_rotation_handle_hover(e->pos()) > 0)
	{
		int handle = check_ref_line_rotation_handle_hover(e->pos());
		m_hover_ref_line = (handle <= 2) ? 1 : 2;
		m_hover_ref_end = (handle % 2 == 1) ? 1 : 2;
		m_is_rotating_ref_line = true;
		m_initial_mouse_angle = reference_line_angle(e->pos());
		m_applied_drag_angle = 0.0;
		m_start_rotation_angle = (m_hover_ref_line == 1) ? m_ref_angle_h : m_ref_angle_v;
		setCursor(Qt::PointingHandCursor);
		return;
	}


	if (parent->mpr_state() && parent->mpr_state()->enabled && parent->get_mouse_modus() == 1 &&
		e->button() == Qt::LeftButton && e->modifiers() == Qt::NoModifier)
	{
		lines_at(e->pos().x(),e->pos().y());
		return;
	}

	if (RadiantInputDispatcher::instance()->handle_mouse_press(this, e, parent))
	{
		return;
	}

	if (e->button() == Qt::ForwardButton ||
	    (RadiantInputDispatcher::instance()->get_tool() == RadiantTool::Length && e->button() == Qt::LeftButton) ||
	    parent->get_mouse_modus() == 2)
	{
		m0_set = true;
		m1_set = false;
		const QPoint p0 = e->pos();
		const QPointF p1 = mapToScene(p0);
		m0_win_pos_x = p1.x();
		m0_win_pos_y = p1.y();
		measure();
		return;
	}


	if (e->button() == Qt::LeftButton && parent->get_mouse_modus() == 1)
	{
		const QPoint p0 = e->pos();
		lines_at(p0.x(), p0.y());
		return;
	}

	QGraphicsView::mousePressEvent(e);
}

void GraphicsView::mouseDoubleClickEvent(QMouseEvent * e)
{
	if (e->button() == Qt::LeftButton && extra_tool == static_cast<int>(RadiantTool::ClosedPolygon) && !extra_complete)
	{
		if (extra_points.size() >= 3) { extra_complete = true; update_extra_measurement(mapToScene(e->pos()), true); }
		return;
	}
	QGraphicsView::mouseDoubleClickEvent(e);
}

void GraphicsView::mouseReleaseEvent(QMouseEvent * e)
{
	if (extra_drag && e->button() == Qt::LeftButton)
	{
		extra_drag = false;
		const QPointF p = mapToScene(e->pos());
		if (extra_measurement_point_valid(p)) { extra_complete = true; update_extra_measurement(p, true); }
		else { extra_points.clear(); extra_tool=-1; if (extra_measure_path) extra_measure_path->setPath(QPainterPath()); if (extra_measure_label) extra_measure_label->setText(QString()); }
		return;
	}
	if (!m_is_rotating_ref_line && RadiantInputDispatcher::instance()->handle_mouse_release(this, e, parent))
	{
		return;
	}

	if (m0_set && (e->button() == Qt::LeftButton || e->button() == Qt::ForwardButton))
	{
		m1_set = true;
		const QPoint p0 = e->pos();
		const QPointF p1 = mapToScene(p0);
		m1_win_pos_x = p1.x();
		m1_win_pos_y = p1.y();
		if (QApplication::keyboardModifiers() & Qt::ShiftModifier)
		{
			const double dx = m1_win_pos_x - m0_win_pos_x;
			const double dy = m1_win_pos_y - m0_win_pos_y;
			const double adx = std::abs(dx);
			const double ady = std::abs(dy);
			if (adx > 2.0 * ady)
				m1_win_pos_y = m0_win_pos_y;
			else if (ady > 2.0 * adx)
				m1_win_pos_x = m0_win_pos_x;
			else
			{
				const double dist = std::max(adx, ady);
				m1_win_pos_x = m0_win_pos_x + ((dx >= 0) ? dist : -dist);
				m1_win_pos_y = m0_win_pos_y + ((dy >= 0) ? dist : -dist);
			}
		}
		measure();
		m0_set = false;
		m1_set = false;
		return;
	}

	if (m_is_rotating_ref_line)
	{
		m_is_rotating_ref_line = false;
		GraphicsWidget * targetWidget = nullptr;
		short targetPlane = 0;
		double angleToApply = (m_hover_ref_line == 1) ? m_ref_angle_h : m_ref_angle_v;

		if (parent->get_axis() == 2)
		{
			targetWidget = (m_hover_ref_line == 1) ? widget_y : widget_x;
			targetPlane = (m_hover_ref_line == 1) ? 1 : 0;
		}
		else if (parent->get_axis() == 1)
		{
			targetWidget = (m_hover_ref_line == 1) ? widget_m : widget_x;
			targetPlane = (m_hover_ref_line == 1) ? 2 : 0;
		}
		else
		{
			targetWidget = (m_hover_ref_line == 1) ? widget_m : widget_y;
			targetPlane = (m_hover_ref_line == 1) ? 2 : 1;
		}

		if (targetWidget && parent->image_container.image3D)
		{
			targetWidget->set_oblique_slice_2D(
				parent->image_container.image3D,
				angleToApply,
				targetPlane,
				parent->get_axis(),
				false /* diagnostic Linear interpolation */);
		}
		m_hover_ref_line = 0;
		m_hover_ref_end = 0;
		unsetCursor();
		return;
	}

	if (e->button() == Qt::LeftButton && parent->get_mouse_modus() == 1)
	{
		const QPoint p0 = e->pos();
		lines_at(p0.x(), p0.y());
		return;
	}

	QGraphicsView::mouseReleaseEvent(e);
}

void GraphicsView::mouseMoveEvent(QMouseEvent * e)
{
	if (!extra_complete && !extra_points.isEmpty() &&
		static_cast<int>(RadiantInputDispatcher::instance()->get_tool()) == extra_tool)
	{
		const QPointF p = mapToScene(e->pos());
		if (extra_measurement_point_valid(p)) update_extra_measurement(p, false);
		return;
	}
	if ((e->buttons() & Qt::LeftButton) && (parent->slab_pending() || !parent->slab_error().isEmpty())) return;
	if (parent->get_show_cursor())
	{
		const QPoint p0 = e->pos();
		const QPointF p1 = mapToScene(p0);
		get_pixel_value(p1.x(), p1.y());
	}

	if (!m_is_rotating_ref_line && parent->mpr_state() && parent->mpr_state()->enabled &&
		parent->get_mouse_modus() == 1 && (e->buttons() & Qt::LeftButton) && e->modifiers() == Qt::NoModifier)
	{
		lines_at(e->pos().x(),e->pos().y());
		return;
	}

	if (!m_is_rotating_ref_line && RadiantInputDispatcher::instance()->handle_mouse_move(this, e, parent))
	{
		return;
	}

	if (m0_set && (e->buttons() & (Qt::LeftButton | Qt::ForwardButton)))
	{
		m1_set = true;
		const QPoint p0 = e->pos();
		const QPointF p1 = mapToScene(p0);
		m1_win_pos_x = p1.x();
		m1_win_pos_y = p1.y();
		if (QApplication::keyboardModifiers() & Qt::ShiftModifier)
		{
			const double dx = m1_win_pos_x - m0_win_pos_x;
			const double dy = m1_win_pos_y - m0_win_pos_y;
			const double adx = std::abs(dx);
			const double ady = std::abs(dy);
			if (adx > 2.0 * ady)
				m1_win_pos_y = m0_win_pos_y;
			else if (ady > 2.0 * adx)
				m1_win_pos_x = m0_win_pos_x;
			else
			{
				const double dist = std::max(adx, ady);
				m1_win_pos_x = m0_win_pos_x + ((dx >= 0) ? dist : -dist);
				m1_win_pos_y = m0_win_pos_y + ((dy >= 0) ? dist : -dist);
			}
		}
		measure();
		return;
	}

	if (!m_is_rotating_ref_line)
	{
		int handle = check_ref_line_rotation_handle_hover(e->pos());
		if (handle > 0)
		{
			m_hover_ref_line = (handle <= 2) ? 1 : 2;
			m_hover_ref_end = (handle % 2 == 1) ? 1 : 2;
			setCursor(Qt::PointingHandCursor);
		}
		else if (m_hover_ref_line > 0)
		{
			m_hover_ref_line = 0;
			m_hover_ref_end = 0;
			unsetCursor();
		}
	}

	if ((e->buttons() & Qt::LeftButton) && m_is_rotating_ref_line)
	{
		double curAngle = reference_line_angle(e->pos());
		double deltaAngle = curAngle - m_initial_mouse_angle;
		double newAngle = m_start_rotation_angle + deltaAngle;
		while (newAngle > M_PI) newAngle -= 2.0 * M_PI;
		while (newAngle < -M_PI) newAngle += 2.0 * M_PI;

		if (m_hover_ref_line == 1)
			m_ref_angle_h = newAngle;
		else
			m_ref_angle_v = newAngle;

		draw_frames(parent->get_axis());

		GraphicsWidget * targetWidget = nullptr;
		short targetPlane = 0;
		double angleToApply = (m_hover_ref_line == 1) ? m_ref_angle_h : m_ref_angle_v;

		if (parent->get_axis() == 2)
		{
			targetWidget = (m_hover_ref_line == 1) ? widget_y : widget_x;
			targetPlane = (m_hover_ref_line == 1) ? 1 : 0;
		}
		else if (parent->get_axis() == 1)
		{
			targetWidget = (m_hover_ref_line == 1) ? widget_m : widget_x;
			targetPlane = (m_hover_ref_line == 1) ? 2 : 0;
		}
		else
		{
			targetWidget = (m_hover_ref_line == 1) ? widget_m : widget_y;
			targetPlane = (m_hover_ref_line == 1) ? 2 : 1;
		}

		const auto & shared = parent->mpr_state();
		if (shared && shared->enabled && shared->initialized && targetWidget)
		{
			const double delta = deltaAngle - m_applied_drag_angle;
			if (!(shared->lock_orthogonal ? shared->planes.rotate_locked(parent->get_axis(), targetPlane, delta) :
                  shared->planes.rotate_target(parent->get_axis(), targetPlane, delta))) return;
			m_applied_drag_angle = deltaAngle;
			if (shared->lock_orthogonal) { sync_mpr_sliders(); refresh_mpr_views(); return; }
			sync_mpr_sliders();
			parent->update_frames();
			if (widget_m) widget_m->update_frames();
			if (widget_y) widget_y->update_frames();
			if (widget_x) widget_x->update_frames();
		}
		if (targetWidget && parent->image_container.image3D)
		{
			targetWidget->set_oblique_slice_2D(
				parent->image_container.image3D,
				angleToApply,
				targetPlane,
				parent->get_axis(),
				true /* fastDraftMode */);
		}
		return;
	}

	if ((e->buttons() & Qt::LeftButton) && parent->get_mouse_modus() == 1)
	{
		const QPoint p0 = e->pos();
		lines_at(p0.x(), p0.y());
		return;
	}

	QGraphicsView::mouseMoveEvent(e);
}

void GraphicsView::flip()
{
	m_angle = 0;
	animate_flip();
}

void GraphicsView::animate_flip()
{
	if (!image_item) return;
	if (m_angle >= 360) return;
	m_angle += 10;
	QRectF r = image_item->boundingRect();
	image_item->setTransform(QTransform()
		.translate(r.width() / 2.0, r.height() / 2.0)
		.rotate(m_angle, Qt::YAxis)
		.translate(-r.width() / 2.0, -r.height() / 2.0));
	QTimer::singleShot(25, this, SLOT(animate_flip()));
}

void GraphicsView::emit_bb_update()
{
	emit bb_changed();
}

void GraphicsView::set_win_old_position(int x, int y)
{
	old_win_pos_x = x;
	old_win_pos_y = y;
}

void GraphicsView::set_win_new_position(int x, int y)
{
	new_win_pos_x = x;
	new_win_pos_y = y;
}

void GraphicsView::set_win_last_position(int x, int y)
{
	last_win_pos_x = x;
	last_win_pos_y = y;
}

void GraphicsView::update_background_color()
{
	QColor bc(Qt::black);
	setBackgroundBrush(QBrush(bc));
	shutter_color = std::move(bc);
}

void GraphicsView::draw_frames(short a)
{
	if (!parent->image_container.image3D) return;
	const double W = parent->image_container.image2D ? static_cast<double>(parent->image_container.image2D->idimx) : 512.0;
	const double H = parent->image_container.image2D ? static_cast<double>(parent->image_container.image2D->idimy) : 512.0;
	const auto * di = parent->image_container.image3D->di;
	if (!di) return;
	const double su = std::abs(a == 0 ? di->iy_spacing : di->ix_spacing);
	const double sv = std::abs(a == 2 ? di->iy_spacing : di->iz_spacing);
	if (su <= 0.0 || sv <= 0.0) return;
	const double L = std::max(W * su, H * sv) * 2.0;
	const double D_handle = std::min(W * su, H * sv) * 0.45;
	const auto & shared = parent->mpr_state();
	if (shared && shared->enabled && shared->initialized && a >= 0 && a < 3 &&
		shared->volume == parent->image_container.image3D)
	{
		const double cx = shared->voxel[a == 0 ? 1 : 0] + 0.5;
		const double cy = shared->voxel[a == 2 ? 1 : 2] + 0.5;
		m_rotation_pivot_scene = QPointF(cx, cy);
		QGraphicsPathItem * items[3] = {line_x, line_y, line_z};
		for (int i = 0; i < 3; ++i)
		{
			QPainterPath path;
			mpr_geometry::Line2D line;
			if (i != a && mpr_geometry::reference_line(shared->planes.planes[a], shared->planes.planes[i], line))
			{
				path.moveTo(cx + (line.u - L * line.du) / su, cy + (line.v - L * line.dv) / sv);
				path.lineTo(cx + (line.u + L * line.du) / su, cy + (line.v + L * line.dv) / sv);
				const QPointF h1(cx + (line.u - D_handle * line.du) / su,
				                 cy + (line.v - D_handle * line.dv) / sv);
				const QPointF h2(cx + (line.u + D_handle * line.du) / su,
				                 cy + (line.v + D_handle * line.dv) / sv);
				const int horizontalTarget = a == 2 ? 1 : 2;
				if (i == horizontalTarget) { m_handle_h1_scene=h1; m_handle_h2_scene=h2; }
				else { m_handle_v1_scene=h1; m_handle_v2_scene=h2; }
			}
			items[i]->setPath(path);
		}
		return;
	}

	switch (a)
	{
	case 0:
		{
			const double Cx = parent->image_container.image3D->di->selected_y_slice + 0.5;
			const double Cy = parent->image_container.image3D->di->selected_z_slice + 0.5;
			m_rotation_pivot_scene = QPointF(Cx, Cy);

			const double cosV = std::cos(m_ref_angle_v);
			const double sinV = std::sin(m_ref_angle_v);
			QPainterPath path_y_line;
			path_y_line.moveTo(Cx + L * sinV / su, Cy - L * cosV / sv);
			path_y_line.lineTo(Cx - L * sinV / su, Cy + L * cosV / sv);
			m_handle_v1_scene = QPointF(Cx + D_handle * sinV / su, Cy - D_handle * cosV / sv);
			m_handle_v2_scene = QPointF(Cx - D_handle * sinV / su, Cy + D_handle * cosV / sv);
			line_y->setPath(path_y_line);

			const double cosH = std::cos(m_ref_angle_h);
			const double sinH = std::sin(m_ref_angle_h);
			QPainterPath path_z_line;
			path_z_line.moveTo(Cx - L * cosH / su, Cy - L * sinH / sv);
			path_z_line.lineTo(Cx + L * cosH / su, Cy + L * sinH / sv);
			m_handle_h1_scene = QPointF(Cx - D_handle * cosH / su, Cy - D_handle * sinH / sv);
			m_handle_h2_scene = QPointF(Cx + D_handle * cosH / su, Cy + D_handle * sinH / sv);
			line_z->setPath(path_z_line);

			QPainterPath empty_path;
			line_x->setPath(empty_path);
		}
		break;
	case 1:
		{
			const double Cx = parent->image_container.image3D->di->selected_x_slice + 0.5;
			const double Cy = parent->image_container.image3D->di->selected_z_slice + 0.5;
			m_rotation_pivot_scene = QPointF(Cx, Cy);

			const double cosV = std::cos(m_ref_angle_v);
			const double sinV = std::sin(m_ref_angle_v);
			QPainterPath path_x_line;
			path_x_line.moveTo(Cx + L * sinV / su, Cy - L * cosV / sv);
			path_x_line.lineTo(Cx - L * sinV / su, Cy + L * cosV / sv);
			m_handle_v1_scene = QPointF(Cx + D_handle * sinV / su, Cy - D_handle * cosV / sv);
			m_handle_v2_scene = QPointF(Cx - D_handle * sinV / su, Cy + D_handle * cosV / sv);
			line_x->setPath(path_x_line);

			const double cosH = std::cos(m_ref_angle_h);
			const double sinH = std::sin(m_ref_angle_h);
			QPainterPath path_z_line;
			path_z_line.moveTo(Cx - L * cosH / su, Cy - L * sinH / sv);
			path_z_line.lineTo(Cx + L * cosH / su, Cy + L * sinH / sv);
			m_handle_h1_scene = QPointF(Cx - D_handle * cosH / su, Cy - D_handle * sinH / sv);
			m_handle_h2_scene = QPointF(Cx + D_handle * cosH / su, Cy + D_handle * sinH / sv);
			line_z->setPath(path_z_line);

			QPainterPath empty_path;
			line_y->setPath(empty_path);
		}
		break;
	case 2:
		{
			const double Cx = parent->image_container.image3D->di->selected_x_slice + 0.5;
			const double Cy = parent->image_container.image3D->di->selected_y_slice + 0.5;
			m_rotation_pivot_scene = QPointF(Cx, Cy);

			const double cosV = std::cos(m_ref_angle_v);
			const double sinV = std::sin(m_ref_angle_v);
			QPainterPath path_x_line;
			path_x_line.moveTo(Cx + L * sinV / su, Cy - L * cosV / sv);
			path_x_line.lineTo(Cx - L * sinV / su, Cy + L * cosV / sv);
			m_handle_v1_scene = QPointF(Cx + D_handle * sinV / su, Cy - D_handle * cosV / sv);
			m_handle_v2_scene = QPointF(Cx - D_handle * sinV / su, Cy + D_handle * cosV / sv);
			line_x->setPath(path_x_line);

			const double cosH = std::cos(m_ref_angle_h);
			const double sinH = std::sin(m_ref_angle_h);
			QPainterPath path_y_line;
			path_y_line.moveTo(Cx - L * cosH / su, Cy - L * sinH / sv);
			path_y_line.lineTo(Cx + L * cosH / su, Cy + L * sinH / sv);
			m_handle_h1_scene = QPointF(Cx - D_handle * cosH / su, Cy - D_handle * sinH / sv);
			m_handle_h2_scene = QPointF(Cx + D_handle * cosH / su, Cy + D_handle * sinH / sv);
			line_y->setPath(path_y_line);

			QPainterPath empty_path;
			line_z->setPath(empty_path);
		}
		break;
	default:
		break;
	}
}

double GraphicsView::reference_line_angle(const QPoint & screen) const
{
	if (!parent || !parent->image_container.image3D || !parent->image_container.image3D->di) return 0.0;
	const auto * di = parent->image_container.image3D->di;
	const int a = parent->get_axis();
	const double su = std::abs(a == 0 ? di->iy_spacing : di->ix_spacing);
	const double sv = std::abs(a == 2 ? di->iy_spacing : di->iz_spacing);
	const QPointF p = mapToScene(screen) - m_rotation_pivot_scene;
	return mpr_geometry::scene_angle(p.x(), p.y(), su, sv);
}

int GraphicsView::check_ref_line_rotation_handle_hover(const QPoint & pos)
{
	const auto & shared = parent->mpr_state();
	if (!shared || !shared->enabled || !shared->initialized || parent->slab_pending() ||
		!parent->slab_error().isEmpty() ||
		(!line_x->isVisible() && !line_y->isVisible() && !line_z->isVisible())) return 0;
	const QPointF handles[4] = {m_handle_h1_scene,m_handle_h2_scene,m_handle_v1_scene,m_handle_v2_scene};
	for (int i=0; i<4; ++i)
	{
		const QPointF p = mapFromScene(handles[i]);
		if (std::hypot(pos.x()-p.x(),pos.y()-p.y()) <= 10.0) return i+1;
	}
	return 0;
}

void GraphicsView::refresh_mpr_views()
{
	const auto shared=parent->mpr_state();
	GraphicsWidget *views[4]={parent,widget_m,widget_y,widget_x};
	for (int i=0; i<4; ++i)
	{
		if (!shared || !shared->enabled) break;
		if (!views[i] || !views[i]->image_container.image3D) continue;
		bool duplicate=false;
		for (int j=0; j<i; ++j) if (views[j]==views[i]) duplicate=true;
		if (!duplicate) views[i]->set_slice_2D(views[i]->image_container.image3D,0,false);
	}
}

void GraphicsView::sync_mpr_sliders()
{
	const auto shared=parent->mpr_state();
	ImageVariant *v=parent->image_container.image3D;
	if (!shared || !shared->enabled || !v) return;
	GraphicsWidget *views[4]={parent,widget_m,widget_y,widget_x};
	for (int i=0; i<4; ++i)
	{
		GraphicsWidget *view=views[i];
		if (!view || !view->slider_m || !view->slider_m->slices_slider || view->get_axis()<0 || view->get_axis()>2) continue;
		const int a=view->get_axis();
		int minimum=0,maximum=0;
		bool valid=false;
		auto bounds=[&](const auto &input) {
			if (input.IsNotNull()) valid=mpr_slab::navigation_bounds_itk(shared->planes,a,
				shared->axisNormalSign[a],shared->sliderStep[a],input.GetPointer(),minimum,maximum);
		};
		switch (v->image_type) {
		case 0: bounds(v->pSS); break; case 1: bounds(v->pUS); break; case 2: bounds(v->pSI); break;
		case 3: bounds(v->pUI); break; case 4: bounds(v->pUC); break; case 5: bounds(v->pF); break;
		case 6: bounds(v->pD); break; case 7: bounds(v->pSLL); break; case 8: bounds(v->pULL); break; default: break;
		}
		if (!valid) continue;
		const bool blocked=view->slider_m->slices_slider->blockSignals(true);
		view->slider_m->slices_slider->setRange(minimum,maximum);
		view->slider_m->set_slice(shared->sliderStep[a]);
		view->slider_m->slices_slider->blockSignals(blocked);
	}
}

void GraphicsView::recover_mpr_views()
{
	const auto & shared=parent->mpr_state();
	if (shared) { shared->enabled=false; shared->reset_orientation(); }
	GraphicsWidget *views[4]={parent,widget_m,widget_y,widget_x};
	for (int i=0; i<4; ++i)
	{
		if (!views[i]) continue;
		bool duplicate=false;
		for (int j=0; j<i; ++j) if (views[j]==views[i]) duplicate=true;
		if (!duplicate) views[i]->recover_mpr_failure();
	}
}

void GraphicsView::reset_reference_line_angles()
{
	m_ref_angle_h = 0.0;
	m_ref_angle_v = 0.0;
	m_is_rotating_ref_line = false;
	m_hover_ref_line = 0;
	m_hover_ref_end = 0;
	unsetCursor();
	if (parent && parent->image_container.image3D)
	{
		draw_frames(parent->get_axis());
	}
}

void GraphicsView::draw_us_regions()
{
	clear_us_regions();
	if (parent->get_axis() != 2) return;
	if (!parent->image_container.image3D) return;
	for (int x = 0;
		x < parent->image_container.image3D->usregions.size();
		++x)
	{
		const UltrasoundRegionData & r =
			parent->image_container.image3D->usregions.at(x);
		const unsigned int minX0 = r.m_X0;
		const unsigned int minY0 = r.m_Y0;
		const unsigned int maxX1 = r.m_X1;
		const unsigned int maxY1 = r.m_Y1;
		const unsigned int upper_left_x  = minX0;
		const unsigned int upper_left_y  = minY0;
		const unsigned int upper_right_x = maxX1;
		const unsigned int upper_right_y = minY0;
		const unsigned int lower_left_x  = minX0;
		const unsigned int lower_left_y  = maxY1;
		const unsigned int lower_right_x = maxX1;
		const unsigned int lower_right_y = maxY1;
		const unsigned int priority =
			(r.m_FlagsBool && ((r.m_RegionFlags & 1) == 0)) ? 0 : 1;
		const unsigned short spatial = r.m_RegionSpatialFormat;
		QColor color(0xff, 0xff, 0xff);
		switch (spatial)
		{
		case 0x1:
			color = QColor(0x00, 0xff, 0x00); // 2D
			break;
		case 0x2:
			color = QColor(0xff, 0x00, 0x00); // M-Mode
			break;
		case 0x3:
			color = QColor(0x00, 0x00, 0xff); // Spectral
			break;
		case 0x4:
			color = QColor(0x00, 0xff, 0xff); // Wave form
			break;
		case 0x5:
			color = QColor(0xff, 0x00, 0xff); // Graphics
			break;
		case 0:
		default:
			break;
		}
		QGraphicsPathItem * i = new QGraphicsPathItem();
		QPen pen;
		pen.setBrush(QBrush(color));
		pen.setWidth(0);
		if (priority == 0) pen.setStyle(Qt::SolidLine);
		else               pen.setStyle(Qt::DotLine);
		pen.setCapStyle(Qt::RoundCap);
		pen.setJoinStyle(Qt::RoundJoin);
		i->setFlag(QGraphicsItem::ItemIsSelectable, false);
		i->setPen(pen);
		QPainterPath p;
		p.moveTo(lower_left_x,  lower_left_y);
		p.lineTo(upper_left_x,  upper_left_y);
		p.lineTo(upper_right_x, upper_right_y);
		p.lineTo(lower_right_x, lower_right_y);
		p.lineTo(lower_left_x,  lower_left_y);
		if (r.m_ReferencePixelBool)
			p.addRect(
				r.m_ReferencePixelX0,
				r.m_ReferencePixelY0,
				1.0,
				1.0);
		i->setPath(p);
		us_regions.push_back(i);
		if (scene()) scene()->addItem(i);
	}
}

void GraphicsView::set_empty_lines()
{
	QPainterPath path___;
	line_x->setPath(path___);
	line_y->setPath(path___);
	line_z->setPath(path___);
}

void GraphicsView::set_empty_distance()
{

	m0_set = false;
	m1_set = false;
	QPainterPath path___;
	measurment_line->setPath(path___);
	if (viewport())
		viewport()->update();
}

void GraphicsView::set_widget_m(GraphicsWidget * w)
{
	widget_m = w;
}

void GraphicsView::set_widget_y(GraphicsWidget * w)
{
	widget_y = w;
}

void GraphicsView::set_widget_x(GraphicsWidget * w)
{
	widget_x = w;
}

void GraphicsView::set_handleitems_cursors(bool t)
{
	if (t)
	{
		if (leftHandle)   leftHandle->setCursor(Qt::SizeHorCursor);
		if (rightHandle)  rightHandle->setCursor(Qt::SizeHorCursor);
		if (topHandle)    topHandle->setCursor(Qt::SizeVerCursor);
		if (bottomHandle) bottomHandle->setCursor(Qt::SizeVerCursor);
	}
	else
	{
		if (leftHandle)   leftHandle->unsetCursor();
		if (rightHandle)  rightHandle->unsetCursor();
		if (topHandle)    topHandle->unsetCursor();
		if (bottomHandle) bottomHandle->unsetCursor();
	}
}

void GraphicsView::lines_at(double x, double y)
{
	const QPoint p0(x, y);
	const QPointF p1 = mapToScene(p0);
	if (parent->handle_mpr_point(p1.x(),p1.y())) return;
	if (parent->image_container.image3D &&
		parent->image_container.image3D->equi)
	{
		int tmpa = static_cast<int>(p1.x());
		if (tmpa < 0) tmpa = 0;
		int tmpb = static_cast<int>(p1.y());
		if (tmpb < 0) tmpb = 0;
		if (parent->is_main())
		{
			switch (parent->get_axis())
			{
			case 0:
				{
					if (tmpa >= parent->image_container.image3D->di->idimy)
					{
						tmpa = parent->image_container.image3D->di->idimy - 1;
					}
					if (tmpb >= parent->image_container.image3D->di->idimz)
					{
						tmpb = parent->image_container.image3D->di->idimz - 1;
					}
					parent->image_container.image3D->di->selected_y_slice = tmpa;
					parent->image_container.image3D->di->selected_z_slice = tmpb;
				}
				break;
			case 1:
				{
					if (tmpa >= parent->image_container.image3D->di->idimx)
					{
						tmpa = parent->image_container.image3D->di->idimx - 1;
					}
					if (tmpb >= parent->image_container.image3D->di->idimz)
					{
						tmpb = parent->image_container.image3D->di->idimz - 1;
					}
					parent->image_container.image3D->di->selected_x_slice = tmpa;
					parent->image_container.image3D->di->selected_z_slice = tmpb;
				}
				break;
			case 2:
				{
					if (tmpa >= parent->image_container.image3D->di->idimx)
					{
						tmpa = parent->image_container.image3D->di->idimx - 1;
					}
					if (tmpb >= parent->image_container.image3D->di->idimy)
					{
						tmpb = parent->image_container.image3D->di->idimy - 1;
					}
					if (parent->is_multiview())
					{
						if (widget_x && !widget_x->run__ && widget_x->slider_m)
						{
							widget_x->slider_m->set_slice(tmpa);
						}
						if (widget_y && !widget_y->run__ && widget_y->slider_m)
						{
							widget_y->slider_m->set_slice(tmpb);
						}
					}
					else
					{
						parent->image_container.image3D->di->selected_x_slice = tmpa;
						parent->image_container.image3D->di->selected_y_slice = tmpb;
					}
				}
				break;
			default:
				break;
			}
		}
		else
		{
			switch (parent->get_axis())
			{
			case 0:
				{
					if (tmpa >= parent->image_container.image3D->di->idimy)
					{
						tmpa =parent->image_container.image3D->di->idimy - 1;
					}
					if (tmpb >= parent->image_container.image3D->di->idimz)
					{
						tmpb =parent->image_container.image3D->di->idimz - 1;
					}
					if (widget_y && widget_y->slider_m)
					{
						widget_y->slider_m->set_slice(tmpa);
					}
					if (widget_m && !widget_m->run__ && widget_m->slider_m)
					{
						widget_m->slider_m->set_slice(tmpb);
					}
				}
				break;
			case 1:
				{
					if (tmpa >= parent->image_container.image3D->di->idimx)
					{
						tmpa = parent->image_container.image3D->di->idimx - 1;
					}
					if (tmpb >= parent->image_container.image3D->di->idimz)
					{
						tmpb = parent->image_container.image3D->di->idimz - 1;
					}
					if (widget_x && widget_x->slider_m)
					{
						widget_x->slider_m->set_slice(tmpa);
					}
					if (widget_m && !widget_m->run__ && widget_m->slider_m)
					{
						widget_m->slider_m->set_slice(tmpb);
					}
				}
				break;
			default:
				break;
			}
		}
		draw_frames(parent->get_axis());
	}
}

bool GraphicsView::extra_measurement_point_valid(const QPointF & p) const
{
	if (!parent || !parent->image_container.image3D || !parent->image_container.image3D->di ||
		!std::isfinite(p.x()) || !std::isfinite(p.y()) || p.x() < 0 || p.y() < 0) return false;
	const auto *di = parent->image_container.image3D->di;
	const int axis = parent->get_axis();
	int width = axis == 0 ? di->idimy : di->idimx;
	int height = axis == 2 ? di->idimy : di->idimz;
	if ((parent->is_oblique() || parent->slab_mode() != 0) && parent->image_container.image2D &&
		parent->image_container.image2D->pD.IsNotNull())
	{
		const auto size=parent->image_container.image2D->pD->GetLargestPossibleRegion().GetSize();
		width=static_cast<int>(size[0]); height=static_cast<int>(size[1]);
	}
	if (p.x() >= width || p.y() >= height) return false;
	if (parent->is_oblique() || parent->slab_mode() != 0)
	{
		const auto &mask = parent->slab_mask();
		if (mask.IsNull()) return false;
		Image2DTypeUC::IndexType idx; idx[0] = static_cast<long>(p.x()); idx[1] = static_cast<long>(p.y());
		if (!mask->GetLargestPossibleRegion().IsInside(idx) || !mask->GetPixel(idx)) return false;
	}
	return true;
}

void GraphicsView::update_extra_measurement(const QPointF & cursor, bool complete)
{
	if (extra_points.isEmpty()) return;
	const auto *di = parent->image_container.image3D->di;
	const int axis = parent->get_axis();
	double su = std::abs(axis == 0 ? di->iy_spacing : di->ix_spacing);
	double sv = std::abs(axis == 2 ? di->iy_spacing : di->iz_spacing);
	if ((parent->is_oblique() || parent->slab_mode() != 0) && parent->image_container.image2D &&
		parent->image_container.image2D->pD.IsNotNull())
	{
		const auto spacing = parent->image_container.image2D->pD->GetSpacing();
		su = std::abs(spacing[0]); sv = std::abs(spacing[1]);
	}
	if (!std::isfinite(su) || !std::isfinite(sv) || su <= 0 || sv <= 0) return;
	if (!extra_measure_path)
	{
		extra_measure_path = scene()->addPath(QPainterPath(), QPen(Qt::cyan, 0));
		extra_measure_path->setZValue(1e10);
		extra_measure_label = scene()->addSimpleText(QString());
		extra_measure_label->setBrush(QBrush(Qt::cyan));
		extra_measure_label->setFlag(QGraphicsItem::ItemIgnoresTransformations);
		extra_measure_label->setZValue(1e10);
	}
	QVector<QPointF> points = extra_points;
	if (!complete && (points.isEmpty() || points.last() != cursor)) points.push_back(cursor);
	else if (complete && extra_drag == false && points.size() == 1) points.push_back(cursor);
	QPainterPath path;
	QString text;
	const auto tool = static_cast<RadiantTool>(extra_tool);
	if ((tool == RadiantTool::Length || tool == RadiantTool::Ellipse || tool == RadiantTool::Arrow) && points.size() >= 2)
	{
		const QPointF a=points.first(), b=points.last();
		if (tool == RadiantTool::Length)
		{
			path.moveTo(a); path.lineTo(b);
			if (complete && mpr_measurement::valid_segments(points,[this](const QPointF &p) { return extra_measurement_point_valid(p); }))
				text=QString::number(std::hypot((b.x()-a.x())*su,(b.y()-a.y())*sv),'f',3)+QString(" mm");
		}
		else if (tool == RadiantTool::Ellipse)
		{
			const QRectF rect = QRectF(a,b).normalized(); path.addEllipse(rect);
			if (complete) {
                const double area=mpr_measurement::ellipse_area(rect,su,sv);
                if (area>0) text = QString::number(area, 'f', 2) + QString::fromUtf8(" mm²");
            }
		}
		else
		{
			path.moveTo(a); path.lineTo(b);
			const QPointF d = b-a; const double len = std::hypot(d.x(),d.y());
			if (len > 0) { const QPointF u=d/len, n(-u.y(),u.x()); const double head=std::min(12.0, len/3.0); path.moveTo(b-u*head+n*head*0.45); path.lineTo(b); path.lineTo(b-u*head-n*head*0.45); }
		}
	}
	else if (tool == RadiantTool::Angle && points.size() >= 2)
	{
		path.moveTo(points[0]); for (int i=1;i<points.size();++i) path.lineTo(points[i]);
		if (complete && points.size() == 3)
		{
            const double angle=mpr_measurement::angle_degrees(points[0],points[1],points[2],su,sv);
            if (angle>=0 && mpr_measurement::valid_segments(points,[this](const QPointF &p) { return extra_measurement_point_valid(p); }))
                text=QString::number(angle,'f',1)+QString::fromUtf8("°");
		}
	}
	else if (tool == RadiantTool::ClosedPolygon && points.size() >= 2)
	{
		if (complete && points.size() >= 3)
		{
            path=mpr_measurement::rounded_polygon(points);
            double area=0;
            if (mpr_measurement::simple_area(path,su,sv,area))
                text=QString::number(area,'f',2)+QString::fromUtf8(" mm²");
		}
		else { path.moveTo(points[0]); for (int i=1;i<points.size();++i) path.lineTo(points[i]); }
	}
	// Do not report an area that includes pixels outside a reconstructed slab.
	if (complete && !text.isEmpty() && (tool == RadiantTool::Ellipse || tool == RadiantTool::ClosedPolygon) &&
		(parent->is_oblique() || parent->slab_mode() != 0))
	{
		const QRect bounds = path.boundingRect().toAlignedRect();
		for (int y=bounds.top();y<=bounds.bottom() && !text.isEmpty();++y)
			for (int x=bounds.left();x<=bounds.right();++x)
				if (path.contains(QPointF(x+0.5,y+0.5)) && !extra_measurement_point_valid(QPointF(x+0.5,y+0.5))) { text.clear(); break; }
	}
	extra_measure_path->setPath(path);
	extra_measure_label->setText(text);
	extra_measure_label->setPos(path.boundingRect().bottomRight());
}

void GraphicsView::measure()
{
	if (!parent->image_container.image3D) return;
	if (m0_set && m1_set)
	{
		parent->update_measurement(
			m0_win_pos_x,
			m0_win_pos_y,
			m1_win_pos_x,
			m1_win_pos_y);
	}
	else
	{
		parent->set_measure_text("");
	}
	if (viewport())
		viewport()->update();
}

void GraphicsView::get_pixel_value(double x, double y)
{
	if (!parent->image_container.image3D) return;
	parent->update_pixel_value(x, y);
}

void GraphicsView::get_pixel_value2(double x, double y)
{
	if (!parent->image_container.image3D) return;
	parent->update_pixel_value2(x, y);
}

void GraphicsView::flipX()
{
	global_flip_x = !global_flip_x;
	parent->update_image(0, false);
}

void GraphicsView::flipY()
{
	global_flip_y = !global_flip_y;
	parent->update_image(0, false);
}

void GraphicsView::rotate_cw()
{
	rotation_angle = (rotation_angle + 90) % 360;
	if (parent && !parent->run__)
		parent->update_image(0, false);
}

void GraphicsView::rotate_ccw()
{
	rotation_angle = (rotation_angle + 270) % 360;
	if (parent && !parent->run__)
		parent->update_image(0, false);
}

void GraphicsView::reset_transformations()
{
	rotation_angle = 0;
	global_flip_x = false;
	global_flip_y = false;
	setTransform(QTransform());
	if (parent && !parent->run__)
		parent->update_image(0, false);
}

void GraphicsView::draw_prtexts(const ImageVariant * ivariant)
{
#if 0
	{
		double L_, a_, b_;
		ColorSpace_::Rgb2Lab(
			&L_, &a_, &b_, 5.0 / 255.0, 220.0 / 255.0, 5.0 / 255.0);
		const unsigned short L = static_cast<unsigned short>((L_ / 100.0) * 65535.0);
		const unsigned short a =
			static_cast<unsigned short>(((a_ + 128.0) / 255.0) * 65535.0);
		const unsigned short b =
			static_cast<unsigned short>(((b_ + 128.0) / 255.0) * 65535.0);
		std::cout <<  L << " " << a << " " << b << std::endl;
	}
#endif
	clear_prtexts_items();
	if (parent->get_axis() != 2) return;
	if (!ivariant) return;
	if (ivariant->pr_text_annotations.empty()) return;
	int idx = -1;
	const int z = ivariant->di->selected_z_slice;
	if (ivariant->pr_text_annotations.contains(z))
	{
		idx = z;
	}
	else if (ivariant->pr_text_annotations.contains(-1))
	{
		;
	}
	else
	{
		return;
	}
	const QList<PRTextAnnotation> & l =
		ivariant->pr_text_annotations.value(idx);
	for (int x = 0; x < l.size(); ++x)
	{
		QGraphicsTextItem * i = new QGraphicsTextItem();
		i->setFlag(QGraphicsItem::ItemIsMovable, false);
		i->setFlag(QGraphicsItem::ItemIsFocusable, false);
		i->setAcceptHoverEvents(false);
		double posx = 0.0;
		double posy = 0.0;
		double r = 0.0;
		if (l.at(x).has_bb)
		{
			double bottom_right_posx = 0.0;
			double bottom_right_posy = 0.0;
			if (l.at(x).BoundingBoxAnnotationUnits == QString("DISPLAY"))
			{
				if (ivariant->pr_display_areas.contains(z))
				{
					int dimx =
						ivariant->pr_display_areas.value(z).bottom_right_x -
						ivariant->pr_display_areas.value(z).top_left_x;
					int dimy =
						ivariant->pr_display_areas.value(z).bottom_right_y -
						ivariant->pr_display_areas.value(z).top_left_y;
					if (dimx > 0 && dimy > 0)
					{
						posx =
							ivariant->pr_display_areas.value(z).top_left_x +
							dimx * l.at(x).bb_top_left_x;
						posy =
							ivariant->pr_display_areas.value(z).top_left_y +
							dimy * l.at(x).bb_top_left_y;
						bottom_right_posx =
							ivariant->pr_display_areas.value(z).top_left_x +
							dimx * l.at(x).bb_bottom_right_x;
						bottom_right_posy =
							ivariant->pr_display_areas.value(z).top_left_y +
							dimy * l.at(x).bb_bottom_right_y;
					}
					else
					{
						continue;
					}
				}
				else
				{
					posx = ivariant->di->idimx * l.at(x).bb_top_left_x;
					posy = ivariant->di->idimy * l.at(x).bb_top_left_y;
					bottom_right_posx = ivariant->di->idimx * l.at(x).bb_bottom_right_x;
					bottom_right_posy = ivariant->di->idimy * l.at(x).bb_bottom_right_y;
				}
			}
			else if (l.at(x).BoundingBoxAnnotationUnits == QString("PIXEL"))
			{
				posx = l.at(x).bb_top_left_x;
				posy = l.at(x).bb_top_left_y;
				bottom_right_posx = l.at(x).bb_bottom_right_x;
				bottom_right_posy = l.at(x).bb_bottom_right_y;
			}
			if (posx < bottom_right_posx &&
				posy < bottom_right_posy)
			{
				;
			}
			else if (posx > bottom_right_posx &&
				posy > bottom_right_posy)
			{
				r = 180.0;
			}
			else if (posx < bottom_right_posx &&
				posy > bottom_right_posy)
			{
				r = 270.0;
			}
			else if (posx > bottom_right_posx &&
				posy < bottom_right_posy)
			{
				r = 90.0;
			}
		}
		else if (l.at(x).has_anchor)
		{
			if (l.at(x).AnchorPointAnnotationUnits == QString("DISPLAY"))
			{
				if (ivariant->pr_display_areas.contains(z))
				{
					int dimx =
						ivariant->pr_display_areas.value(z).bottom_right_x -
						ivariant->pr_display_areas.value(z).top_left_x;
					int dimy =
						ivariant->pr_display_areas.value(z).bottom_right_y -
						ivariant->pr_display_areas.value(z).top_left_y;
					if (dimx > 0 && dimy > 0)
					{
						posx =
							ivariant->pr_display_areas.value(z).top_left_x +
							dimx * l.at(x).anchor_x;
						posy =
							ivariant->pr_display_areas.value(z).top_left_y +
							dimy * l.at(x).anchor_y;
					}
					else
					{
						continue;
					}
				}
				else
				{
					posx = ivariant->di->idimx * l.at(x).anchor_x;
					posy = ivariant->di->idimy * l.at(x).anchor_y;
				}
			}
			else if (l.at(x).AnchorPointAnnotationUnits == QString("PIXEL"))
			{
				posx = l.at(x).anchor_x;
				posy = l.at(x).anchor_y;
			}
		}
		const bool anchor_visible =
			l.at(x).AnchorPointVisibility == QString("Y")
			? true : false;
		if (l.at(x).has_anchor && anchor_visible)
		{
			double aposx = 0.0;
			double aposy = 0.0;
			if (l.at(x).AnchorPointAnnotationUnits == QString("DISPLAY"))
			{
				if (ivariant->pr_display_areas.contains(z))
				{
					int dimx =
						ivariant->pr_display_areas.value(z).bottom_right_x -
						ivariant->pr_display_areas.value(z).top_left_x;
					int dimy =
						ivariant->pr_display_areas.value(z).bottom_right_y -
						ivariant->pr_display_areas.value(z).top_left_y;
					if (dimx > 0 && dimy > 0)
					{
						aposx =
							ivariant->pr_display_areas.value(z).top_left_x +
							dimx * l.at(x).anchor_x;
						aposy =
							ivariant->pr_display_areas.value(z).top_left_y +
							dimy * l.at(x).anchor_y;
					}
					else
					{
						continue;
					}
				}
				else
				{
					aposx = ivariant->di->idimx * l.at(x).anchor_x;
					aposy = ivariant->di->idimy * l.at(x).anchor_y;
				}
			}
			else if (l.at(x).AnchorPointAnnotationUnits == QString("PIXEL"))
			{
				aposx = l.at(x).anchor_x;
				aposy = l.at(x).anchor_y;
			}
			QPen pen;
			pen.setBrush(QBrush(QColor(255, 127, 23)));
			QPainterPath p;
			p.moveTo(aposx,aposy - 3);
			p.lineTo(aposx,aposy + 3);
			p.moveTo(aposx - 3, aposy);
			p.lineTo(aposx + 3, aposy);
			GraphicsPathItem * a = new GraphicsPathItem();
			a->setFlag(QGraphicsItem::ItemIsMovable, false);
			a->setFlag(QGraphicsItem::ItemIsFocusable, false);
			a->setAcceptHoverEvents(false);
			a->setPath(p);
			a->setPen(pen);
			a->setZValue(1e+18);
			scene()->addItem(static_cast<QGraphicsItem*>(a));
			prtextanchors.push_back(a);
		}
		i->setRotation(r);
		i->setPos(posx, posy);
		if (l.at(x).has_textstyle)
		{
			QFont font = i->font();
			if (!l.at(x).FontName.isEmpty())
			{
				font.setFamily(l.at(x).FontName);
			}
			else if (!l.at(x).CSSFontName.isEmpty())
			{
				font.setFamily(l.at(x).CSSFontName);
			}
			if (!l.at(x).Bold.isEmpty())
			{
				if (l.at(x).Bold == QString("Y")) font.setBold(true);
			}
			if (!l.at(x).Italic.isEmpty())
			{
				if (l.at(x).Italic == QString("Y")) font.setItalic(true);
			}
			if (!l.at(x).Underlined.isEmpty())
			{
				if (l.at(x).Underlined == QString("Y")) font.setUnderline(true);
			}
			if (!(
				l.at(x).TextColorCIELabValue_L == -1 &&
				l.at(x).TextColorCIELabValue_a == -1 &&
				l.at(x).TextColorCIELabValue_b == -1))
			{
				double R, G, B;
				double _L = (static_cast<double>(l.at(x).TextColorCIELabValue_L) / 65535.0) * 100.0;
				double _a = ((static_cast<unsigned short>
					(l.at(x).TextColorCIELabValue_a) - 0x8080) / 65535.0) * 255.0;
				double _b = ((static_cast<unsigned short>
					(l.at(x).TextColorCIELabValue_b) - 0x8080) / 65535.0) * 255.0;
				ColorSpace_::Lab2Rgb(&R, &G, &B, _L, _a, _b);
				i->setDefaultTextColor(QColor(
					static_cast<unsigned int>(R * 255.0),
					static_cast<unsigned int>(G * 255.0),
					static_cast<unsigned int>(B * 255.0),
					255));
			}
			else
			{
				i->setDefaultTextColor(QColor(150, 0, 150, 255));
			}
			i->setFont(font);
			if (l.at(x).ShadowStyle == QString("NORMAL"))
			{
				QGraphicsDropShadowEffect * shadow = new QGraphicsDropShadowEffect(i);
				shadow->setXOffset(1.5); // not supported
				shadow->setYOffset(1.5); // not supported
				double opacity = 255.0;
				if (l.at(x).ShadowOpacity > 0 && l.at(x).ShadowOpacity < 1)
				{
					opacity *= l.at(x).ShadowOpacity;
				}
				if (!(
					l.at(x).ShadowColorCIELabValue_L == -1 &&
					l.at(x).ShadowColorCIELabValue_a == -1 &&
					l.at(x).ShadowColorCIELabValue_b == -1))
				{
					double R, G, B;
					double _L = (static_cast<double>(l.at(x).ShadowColorCIELabValue_L) / 65535.0) * 100.0;
					double _a = ((static_cast<unsigned short>
						(l.at(x).ShadowColorCIELabValue_a) - 0x8080) / 65535.0) * 255.0;
					double _b = ((static_cast<unsigned short>
						(l.at(x).ShadowColorCIELabValue_b) - 0x8080) / 65535.0) * 255.0;
					ColorSpace_::Lab2Rgb(&R, &G, &B, _L, _a, _b);
					shadow->setColor(
						QColor(
							static_cast<unsigned int>(R * 255.0),
							static_cast<unsigned int>(G * 255.0),
							static_cast<unsigned int>(B * 255.0),
							static_cast<unsigned int>(opacity)));
				}
				else
				{
					shadow->setColor(QColor(128, 128, 128, static_cast<unsigned int>(opacity)));
				}
				i->setGraphicsEffect(shadow);
			}
			else if (l.at(x).ShadowStyle == QString("OUTLINED"))
			{
				; // TODO
			}
		}
		else
		{
			i->setDefaultTextColor(QColor(150, 0, 150, 255));
			if (true)
			{
				QGraphicsDropShadowEffect * shadow = new QGraphicsDropShadowEffect(i);
				shadow->setXOffset(1.5);
				shadow->setYOffset(1.5);
				shadow->setColor(QColor(128, 128, 128, 255));
				i->setGraphicsEffect(shadow);
			}
		}
		i->setPlainText(l.at(x).UnformattedTextValue);
		i->setZValue(1e+18);
		scene()->addItem(static_cast<QGraphicsItem*>(i));
		prtexts.push_back(i);
	}
}

void GraphicsView::draw_prgraphics(const ImageVariant * ivariant)
{
	// TODO complete
	clear_prgraphicobjects_items();
	if (parent->get_axis() != 2) return;
	if (!ivariant) return;
	if (ivariant->pr_graphicobjects.empty()) return;
	int idx = -1;
	const int z = ivariant->di->selected_z_slice;
	if (ivariant->pr_graphicobjects.contains(z))
	{
		idx = z;
	}
	else if (ivariant->pr_graphicobjects.contains(-1))
	{
		;
	}
	else
	{
		return;
	}
	const QList<PRGraphicObject> & l = ivariant->pr_graphicobjects.value(idx);
	for (int x = 0; x < l.size(); ++x)
	{
		if (!(l.at(x).GraphicData.size() > 1 && (l.at(x).GraphicData.size() % 2 == 0)))
		{
			continue;
		}
		const QString t = l.at(x).GraphicType;
		if (t == QString("POLYLINE"))
		{
			QPainterPath p;
			if (l.at(x).GraphicAnnotationUnits == QString("PIXEL"))
			{
				p.moveTo(
					l.at(x).GraphicData.at(0),
					l.at(x).GraphicData.at(1));
				for (unsigned int y = 2; y < l.at(x).GraphicData.size(); y += 2)
				{
					p.lineTo(
						l.at(x).GraphicData.at(y),
						l.at(x).GraphicData.at(y + 1));
				}
			}
			else if (l.at(x).GraphicAnnotationUnits == QString("DISPLAY"))
			{
				if (ivariant->pr_display_areas.contains(z))
				{
					const int dimx =
						ivariant->pr_display_areas.value(z).bottom_right_x -
						ivariant->pr_display_areas.value(z).top_left_x;
					const int dimy =
						ivariant->pr_display_areas.value(z).bottom_right_y -
						ivariant->pr_display_areas.value(z).top_left_y;
					if (dimx > 0 && dimy > 0)
					{
						for (unsigned int y = 0; y < l.at(x).GraphicData.size(); y += 2)
						{
							const double px =
								ivariant->pr_display_areas.value(z).top_left_x +
								dimx * l.at(x).GraphicData.at(y);
							const double py =
								ivariant->pr_display_areas.value(z).top_left_y +
								dimy * l.at(x).GraphicData.at(y + 1);
							if (y > 1) p.lineTo(px, py);
							else       p.moveTo(px, py);
						}
					}
				}
				else
				{
					for (unsigned int y = 0;
						y < l.at(x).GraphicData.size(); y += 2)
					{
						const double px = ivariant->di->idimx * l.at(x).GraphicData.at(y);
						const double py = ivariant->di->idimy * l.at(x).GraphicData.at(y + 1);
						if (y > 1) p.lineTo(px, py);
						else       p.moveTo(px, py);
					}
				}
			}
			else
			{
				continue;
			}
			{
				QGraphicsPathItem * i = new QGraphicsPathItem();
				i->setFlag(QGraphicsItem::ItemIsMovable, false);
				i->setFlag(QGraphicsItem::ItemIsFocusable, false);
				i->setAcceptHoverEvents(false);
				if (l.at(x).GraphicFilled == QString("Y"))
				{
					QBrush brush(QColor(131, 108, 154, 255));
					brush.setStyle(Qt::SolidPattern);
					i->setPen(QPen(Qt::NoPen));
					i->setBrush(brush);
				}
				else
				{
					QPen pen;
					pen.setBrush(QBrush(QColor(192, 30, 200, 255)));
					pen.setWidth(0);
					pen.setStyle(Qt::SolidLine);
					pen.setCapStyle(Qt::RoundCap);
					pen.setJoinStyle(Qt::RoundJoin);
					i->setPen(pen);
				}
				i->setPath(p);
				i->setZValue(1e+17);
				scene()->addItem(static_cast<QGraphicsItem*>(i));
				prpolylines.push_back(i);
			}
		}
		else if (t == QString("ELLIPSE"))
		{
			if (l.at(x).GraphicData.size() != 8) continue;
			double major0_x = 0.0;
			double major0_y = 0.0;
			double major1_x = 0.0;
			double major1_y = 0.0;
			double minor0_x = 0.0;
			double minor0_y = 0.0;
			double minor1_x = 0.0;
			double minor1_y = 0.0;
			if (l.at(x).GraphicAnnotationUnits == QString("PIXEL"))
			{
				major0_x = l.at(x).GraphicData.at(0);
				major0_y = l.at(x).GraphicData.at(1);
				major1_x = l.at(x).GraphicData.at(2);
				major1_y = l.at(x).GraphicData.at(3);
				minor0_x = l.at(x).GraphicData.at(4);
				minor0_y = l.at(x).GraphicData.at(5);
				minor1_x = l.at(x).GraphicData.at(6);
				minor1_y = l.at(x).GraphicData.at(7);
			}
			else if (l.at(x).GraphicAnnotationUnits == QString("DISPLAY"))
			{
				if (ivariant->pr_display_areas.contains(z))
				{
					const int dimx =
						ivariant->pr_display_areas.value(z).bottom_right_x -
						ivariant->pr_display_areas.value(z).top_left_x;
					const int dimy =
						ivariant->pr_display_areas.value(z).bottom_right_y -
						ivariant->pr_display_areas.value(z).top_left_y;
					if (dimx > 0 && dimy > 0)
					{
						major0_x =
							ivariant->pr_display_areas.value(z).top_left_x +
							dimx * l.at(x).GraphicData.at(0);
						major0_y =
							ivariant->pr_display_areas.value(z).top_left_y +
							dimy * l.at(x).GraphicData.at(1);
						major1_x =
							ivariant->pr_display_areas.value(z).top_left_x +
							dimx * l.at(x).GraphicData.at(2);
						major1_y =
							ivariant->pr_display_areas.value(z).top_left_y +
							dimy * l.at(x).GraphicData.at(3);
						minor0_x =
							ivariant->pr_display_areas.value(z).top_left_x +
							dimx * l.at(x).GraphicData.at(4);
						minor0_y =
							ivariant->pr_display_areas.value(z).top_left_y +
							dimy * l.at(x).GraphicData.at(5);
						minor1_x =
							ivariant->pr_display_areas.value(z).top_left_x +
							dimx * l.at(x).GraphicData.at(6);
						minor1_y =
							ivariant->pr_display_areas.value(z).top_left_y +
							dimy * l.at(x).GraphicData.at(7);
					}
					else
					{
						continue;
					}
				}
				else
				{
					major0_x = ivariant->di->idimx * l.at(x).GraphicData.at(0);
					major0_y = ivariant->di->idimy * l.at(x).GraphicData.at(1);
					major1_x = ivariant->di->idimx * l.at(x).GraphicData.at(2);
					major1_y = ivariant->di->idimy * l.at(x).GraphicData.at(3);
					minor0_x = ivariant->di->idimx * l.at(x).GraphicData.at(4);
					minor0_y = ivariant->di->idimy * l.at(x).GraphicData.at(5);
					minor1_x = ivariant->di->idimx * l.at(x).GraphicData.at(6);
					minor1_y = ivariant->di->idimy * l.at(x).GraphicData.at(7);
				}
			}
			else
			{
				continue;
			}
			{
				const double mid_major_x = (major0_x + major1_x) * 0.5;
				const double mid_major_y = (major0_y + major1_y) * 0.5;
				const double x0__ = major1_x - major0_x;
				const double y0__ = major1_y - major0_y;
				const double x1__ = minor1_x - minor0_x;
				const double y1__ = minor1_y - minor0_y;
				const double d0   = sqrt(x0__ * x0__ + y0__ * y0__);
				const double d1   = sqrt(x1__ * x1__ + y1__ * y1__);
				const double ma_j = 1.0 / d0;
				const double ma_nx = x0__ * ma_j;
				const double ma_ny = y0__ * ma_j;
				const double mi_j = 1.0 / d1;
				const double mi_nx = x1__ * mi_j;
				const double mi_ny = y1__ * mi_j;
				const double start = 0.0;
				const double span  = 360.0;
				QGraphicsPathItem * i = new QGraphicsPathItem();
				{
					QTransform ttt;
					ttt.translate(mid_major_x, mid_major_y);
					i->setTransform(ttt, true);
				}
				{
					QTransform ttt;
					ttt.setMatrix(ma_nx, ma_ny, 0, -mi_nx, -mi_ny, 0, 0, 0, 1);
					i->setTransform(ttt, true);
				}
				QRectF r___(-0.5 * d0, -0.5 * d1, d0, d1);
				QPainterPath path;
				path.arcMoveTo(r___, start);
				path.arcTo(r___, start, span);
				i->setFlag(QGraphicsItem::ItemIsMovable, false);
				i->setFlag(QGraphicsItem::ItemIsFocusable, false);
				i->setAcceptHoverEvents(false);
				if (l.at(x).GraphicFilled == QString("Y"))
				{
					QBrush brush(QColor(131, 108, 154, 255));
					brush.setStyle(Qt::SolidPattern);
					i->setPen(QPen(Qt::NoPen));
					i->setBrush(brush);
				}
				else
				{
					QPen pen;
					pen.setBrush(QBrush(QColor(192, 30, 200, 255)));
					pen.setWidth(0);
					pen.setStyle(Qt::SolidLine);
					pen.setCapStyle(Qt::RoundCap);
					pen.setJoinStyle(Qt::RoundJoin);
					i->setPen(pen);
				}
				i->setPath(path);
				i->setZValue(1e+17);
				scene()->addItem(static_cast<QGraphicsItem*>(i));
				prellipses.push_back(i);
			}
		}
		else if (t == QString("CIRCLE"))
		{
			if (l.at(x).GraphicData.size() != 4) continue;
			double center_x = 0.0;
			double center_y = 0.0;
			double point_x  = 0.0;
			double point_y  = 0.0;
			if (l.at(x).GraphicAnnotationUnits ==
					QString("PIXEL"))
			{
				center_x = l.at(x).GraphicData.at(0);
				center_y = l.at(x).GraphicData.at(1);
				point_x  = l.at(x).GraphicData.at(2);
				point_y  = l.at(x).GraphicData.at(3);
			}
			else if (l.at(x).GraphicAnnotationUnits == QString("DISPLAY"))
			{
				if (ivariant->pr_display_areas.contains(z))
				{
					const int dimx =
						ivariant->pr_display_areas.value(z).bottom_right_x -
						ivariant->pr_display_areas.value(z).top_left_x;
					const int dimy =
						ivariant->pr_display_areas.value(z).bottom_right_y -
						ivariant->pr_display_areas.value(z).top_left_y;
					if (dimx > 0 && dimy > 0)
					{
						center_x =
							ivariant->pr_display_areas.value(z).top_left_x +
							dimx * l.at(x).GraphicData.at(0);
						center_y =
							ivariant->pr_display_areas.value(z).top_left_y +
							dimy * l.at(x).GraphicData.at(1);
						point_x =
							ivariant->pr_display_areas.value(z).top_left_x +
							dimx * l.at(x).GraphicData.at(2);
						point_y =
							ivariant->pr_display_areas.value(z).top_left_y +
							dimy * l.at(x).GraphicData.at(3);
					}
					else
					{
						continue;
					}
				}
				else
				{
					center_x = ivariant->di->idimx * l.at(x).GraphicData.at(0);
					center_y = ivariant->di->idimy * l.at(x).GraphicData.at(1);
					point_x = ivariant->di->idimx * l.at(x).GraphicData.at(2);
					point_y = ivariant->di->idimy * l.at(x).GraphicData.at(3);
				}
			}
			else
			{
				continue;
			}
			{
				const double x__ = point_x - center_x;
				const double y__ = point_y - center_y;
				const double distance = sqrt(x__ * x__ + y__ * y__);
				QGraphicsEllipseItem * i =
					new QGraphicsEllipseItem(
						center_x - distance,
						center_y - distance,
						2 * distance,
						2 * distance);
				i->setFlag(QGraphicsItem::ItemIsMovable, false);
				i->setFlag(QGraphicsItem::ItemIsFocusable, false);
				i->setAcceptHoverEvents(false);
				if (l.at(x).GraphicFilled == QString("Y"))
				{
					QBrush brush(QColor(131, 108, 154, 255));
					brush.setStyle(Qt::SolidPattern);
					i->setPen(QPen(Qt::NoPen));
					i->setBrush(brush);
				}
				else
				{
					QPen pen;
					pen.setBrush(QBrush(QColor(192, 30, 200, 255)));
					pen.setWidth(0);
					pen.setStyle(Qt::SolidLine);
					pen.setCapStyle(Qt::RoundCap);
					pen.setJoinStyle(Qt::RoundJoin);
					i->setPen(pen);
				}
				i->setZValue(1e+17);
				scene()->addItem(static_cast<QGraphicsItem*>(i));
				prcircles.push_back(i);
			}
		}
		else if (t == QString("POINT"))
		{
			if (l.at(x).GraphicData.size()!=2) continue;
			double point_x  = 0.0;
			double point_y  = 0.0;
			if (l.at(x).GraphicAnnotationUnits ==
					QString("PIXEL"))
			{
				point_x  = l.at(x).GraphicData.at(0);
				point_y  = l.at(x).GraphicData.at(1);
			}
			else if (l.at(x).GraphicAnnotationUnits == QString("DISPLAY"))
			{
				if (ivariant->pr_display_areas.contains(z))
				{
					const int dimx =
						ivariant->pr_display_areas.value(z).bottom_right_x -
						ivariant->pr_display_areas.value(z).top_left_x;
					const int dimy =
						ivariant->pr_display_areas.value(z).bottom_right_y -
						ivariant->pr_display_areas.value(z).top_left_y;
					if (dimx > 0 && dimy > 0)
					{
						point_x =
							ivariant->pr_display_areas.value(z).top_left_x +
							dimx * l.at(x).GraphicData.at(0);
						point_y =
							ivariant->pr_display_areas.value(z).top_left_y +
							dimy * l.at(x).GraphicData.at(1);
					}
					else
					{
						continue;
					}
				}
				else
				{
					point_x = ivariant->di->idimx * l.at(x).GraphicData.at(0);
					point_y = ivariant->di->idimy * l.at(x).GraphicData.at(1);
				}
			}
			else
			{
				continue;
			}
			{
				QBrush brush(QColor(192, 30, 200, 255));
				brush.setStyle(Qt::SolidPattern);
				QGraphicsEllipseItem * i =
					new QGraphicsEllipseItem(
						point_x - 1.0,
						point_y - 1.0,
						2.0,
						2.0);
				i->setFlag(QGraphicsItem::ItemIsMovable, false);
				i->setFlag(QGraphicsItem::ItemIsFocusable, false);
				i->setAcceptHoverEvents(false);
				i->setPen(QPen(Qt::NoPen));
				i->setBrush(brush);
				i->setZValue(1e+17);
				scene()->addItem(static_cast<QGraphicsItem*>(i));
				prpoints.push_back(i);
			}
		}
		else if (t == QString("INTERPOLATED"))
		{
		}
	}
}

void GraphicsView::draw_shutter(const ImageVariant * ivariant)
{
	clear_shutters();
	if (parent->get_axis() != 2) return;
	if (!ivariant) return;
	if (ivariant->pr_display_shutters.empty()) return;
	int idx = -1;
	if (ivariant->pr_display_shutters.contains(-1))
	{
		;
	}
	else
	{
		const int z = ivariant->di->selected_z_slice;
		if (ivariant->pr_display_shutters.contains(z))
		{
			idx = z;
		}
		else
		{
			return;
		}
	}
	const PRDisplayShutter & a =
		ivariant->pr_display_shutters.value(idx);
#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
	const QStringList & l =
			a.ShutterShape.split(QString("\\"), Qt::SkipEmptyParts);
#else
	const QStringList & l =
			a.ShutterShape.split(QString("\\"), QString::SkipEmptyParts);
#endif
	for (int x = 0; x < l.size(); ++x)
	{
		if (l.at(x).trimmed() == QString("RECTANGULAR"))
		{
			if (a.ShutterLeftVerticalEdge    == -1 &&
				a.ShutterRightVerticalEdge   == -1 &&
				a.ShutterUpperHorizontalEdge == -1 &&
				a.ShutterLowerHorizontalEdge == -1)
			{
				continue;
			}
			QGraphicsPathItem * i = new QGraphicsPathItem();
			i->setFlag(QGraphicsItem::ItemIsMovable, false);
			i->setFlag(QGraphicsItem::ItemIsFocusable, false);
			i->setAcceptHoverEvents(false);
			QBrush brush(shutter_color);
			brush.setStyle(Qt::SolidPattern);
			i->setPen(QPen(Qt::NoPen));
			i->setBrush(brush);
			QPainterPath p;
			p.moveTo(0, 0);
			p.addPolygon(
				QPolygon(
					QRect(
						-2,
						-2,
						ivariant->di->idimx + 2,
						ivariant->di->idimy + 2)));
			p.moveTo(0, 0);
			p.addPolygon(
				QPolygon(
					QRect(
						(a.ShutterLeftVerticalEdge    - 1),
						(a.ShutterUpperHorizontalEdge - 1),
						a.ShutterRightVerticalEdge -
							a.ShutterLeftVerticalEdge,
						a.ShutterLowerHorizontalEdge -
							a.ShutterUpperHorizontalEdge)));
			i->setPath(p);
			i->setZValue(1e+16);
			scene()->addItem(static_cast<QGraphicsItem*>(i));
			display_shutters.push_back(i);
		}
		else if (l.at(x).trimmed() == QString("CIRCULAR"))
		{
			if (a.CenterofCircularShutter_x == -1 &&
				a.CenterofCircularShutter_y == -1 &&
				a.RadiusofCircularShutter   == -1)
			{
				continue;
			}
			if (a.RadiusofCircularShutter <= 0)
			{
				continue;
			}
			QGraphicsPathItem * i = new QGraphicsPathItem();
			i->setFlag(QGraphicsItem::ItemIsMovable, false);
			i->setFlag(QGraphicsItem::ItemIsFocusable, false);
			i->setAcceptHoverEvents(false);
			QBrush brush(shutter_color);
			brush.setStyle(Qt::SolidPattern);
			i->setPen(QPen(Qt::NoPen));
			i->setBrush(brush);
			QPainterPath p;
			p.moveTo(0, 0);
			p.addPolygon(
				QPolygon(
					QRect(
						-2,
						-2,
						ivariant->di->idimx + 2,
						ivariant->di->idimy + 2)));
			p.moveTo(0, 0);
			const int d = 2 * a.RadiusofCircularShutter;
			p.addEllipse(
				(a.CenterofCircularShutter_x - 1) - a.RadiusofCircularShutter,
				(a.CenterofCircularShutter_y - 1) - a.RadiusofCircularShutter,
				d,
				d);
			i->setPath(p);
			i->setZValue(1e+16);
			scene()->addItem(static_cast<QGraphicsItem*>(i));
			display_shutters.push_back(i);
		}
		else if (l.at(x).trimmed() == QString("POLYGONAL"))
		{
			if (a.VerticesofthePolygonalShutter.empty())
			{
				continue;
			}
			if (!(a.VerticesofthePolygonalShutter.size() > 1 &&
					a.VerticesofthePolygonalShutter.size() % 2 == 0))
			{
				continue;
			}
			QVector<QPoint> points;
			for (unsigned int j = 0;
				j < a.VerticesofthePolygonalShutter.size(); j += 2)
			{
				points.push_back(QPoint(
					a.VerticesofthePolygonalShutter.at(j + 1) - 1,
					a.VerticesofthePolygonalShutter.at(    j) - 1));
			}
			QGraphicsPathItem * i = new QGraphicsPathItem();
			i->setFlag(QGraphicsItem::ItemIsMovable, false);
			i->setFlag(QGraphicsItem::ItemIsFocusable, false);
			i->setAcceptHoverEvents(false);
			QBrush brush(shutter_color);
			brush.setStyle(Qt::SolidPattern);
			i->setPen(QPen(Qt::NoPen));
			i->setBrush(brush);
			QPainterPath p;
			p.moveTo(0, 0);
			p.addPolygon(
				QPolygon(
					QRect(
						-2,
						-2,
						ivariant->di->idimx + 2,
						ivariant->di->idimy + 2)));
			p.moveTo(0, 0);
			p.addPolygon(QPolygon(points));
			i->setPath(p);
			i->setZValue(1e+16);
			scene()->addItem(static_cast<QGraphicsItem*>(i));
			display_shutters.push_back(i);
		}
	}
}

void GraphicsView::reset_zoom()
{
	m_scale = 1.0;
	setTransform(QTransform());
	if (parent) parent->update_image(1, false);
	update_selection_rect_width();
}

void GraphicsView::set_zoom_factor(double factor)
{
	m_scale = factor;
	setTransform(QTransform().scale(factor, factor));
	update_selection_rect_width();
}

void GraphicsView::pan_view(int dx, int dy)
{
	if (horizontalScrollBar())
		horizontalScrollBar()->setValue(horizontalScrollBar()->value() + dx);
	if (verticalScrollBar())
		verticalScrollBar()->setValue(verticalScrollBar()->value() + dy);
}

void GraphicsView::rotate_view(double angle)
{
	if (angle > 0)
		rotate_cw();
	else
		rotate_ccw();
}

void GraphicsView::clear_extra_measurements()
{
	for (auto *item : saved_measure_paths) delete item;
	for (auto *item : saved_measure_labels) delete item;
	saved_measure_paths.clear(); saved_measure_labels.clear();
	delete extra_measure_path; delete extra_measure_label;
	extra_measure_path=nullptr; extra_measure_label=nullptr;
	extra_points.clear(); extra_tool=-1; extra_drag=false; extra_complete=false; extra_volume=nullptr;
}

void GraphicsView::clear_measurements()
{
	clear_extra_measurements();
	set_empty_distance();
	clear_collision_paths();
	if (parent)
		parent->set_measure_text(QString(""));
	if (viewport())
		viewport()->update();
}

void GraphicsView::delete_selected_measurement()
{
	clear_measurements();
}


