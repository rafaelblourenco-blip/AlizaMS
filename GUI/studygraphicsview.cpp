#include "studygraphicsview.h"
#include "studygraphicswidget.h"
#include "graphicsview.h"
#include "radiantinputdispatcher.h"
#include <QtGlobal>
#include <QColor>
#include <QRectF>
#include <QStyleOptionButton>
#include <QGraphicsSceneMouseEvent>
#include <QApplication>
#include <QList>
#include <QGraphicsDropShadowEffect>
#include "colorspace/colorspace.h"

StudyGraphicsView::StudyGraphicsView(StudyGraphicsWidget * p)
	: parent(p)
{
	setAcceptDrops(false);
	setFrameStyle(QFrame::Plain);
	setFrameShape(QFrame::NoFrame);
	QGraphicsScene * scene_ = new QGraphicsScene(this);
	scene_->setItemIndexMethod(QGraphicsScene::NoIndex);
	const QRectF rectf(0, 0, 100, 100);
	scene_->setSceneRect(rectf);
	setScene(scene_);
	QColor bc = qApp->palette().color(QPalette::Window);
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
	image_item->setPos(0,0);
	image_item->setZValue(-1.0);
	scene_->addItem(image_item);
#endif
	setViewportUpdateMode(BoundingRectViewportUpdate);
	setTransformationAnchor(AnchorUnderMouse);
	setDragMode(QGraphicsView::ScrollHandDrag);
	//
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
}

StudyGraphicsView::~StudyGraphicsView()
{
	clear_collision_paths();
	clear_us_regions();
	clear_prtexts_items();
	clear_prgraphicobjects_items();
	clear_shutters();
}

void StudyGraphicsView::keyPressEvent(QKeyEvent * e)
{
	const bool ctrl = (e->modifiers() & Qt::ControlModifier);

	if (ctrl)
	{
		switch (e->key())
		{
		case Qt::Key_0:
			parent->update_image(1);
			return;
		case Qt::Key_1:
			m_scale = 1.0;
			setTransform(QTransform());
			parent->update_image(0);
			return;
		case Qt::Key_2:
			m_scale = 2.0;
			setTransform(QTransform().scale(2.0, 2.0));
			parent->update_image(0);
			return;
		case Qt::Key_3:
			m_scale = 4.0;
			setTransform(QTransform().scale(4.0, 4.0));
			parent->update_image(0);
			return;
		case Qt::Key_4:
			m_scale = 8.0;
			setTransform(QTransform().scale(8.0, 8.0));
			parent->update_image(0);
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
			if (parent) parent->set_alt_mode(!parent->get_alt_mode());
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
	case Qt::Key_F:
		parent->update_image(1);
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

void StudyGraphicsView::keyReleaseEvent(QKeyEvent * e)
{
	switch (e->key())
	{
	case Qt::Key_Shift:
	default:
		break;
	}
	QGraphicsView::keyReleaseEvent(e);
}

void StudyGraphicsView::wheelEvent(QWheelEvent * e)
{
	if (RadiantInputDispatcher::instance()->handle_study_wheel(this, e, parent))
	{
		return;
	}
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
	scale_view(pow(2.0, -e->angleDelta().y() / 480.0));
#else
	scale_view(pow(2.0, -e->delta() / 480.0));
#endif
}

void StudyGraphicsView::drawBackground(QPainter * painter, const QRectF & rect)
{
	QGraphicsView::drawBackground(painter, rect);
}

void StudyGraphicsView::drawForeground(QPainter * painter, const QRectF & rect)
{
	Q_UNUSED(rect);
	if (!GraphicsView::is_hud_visible()) return;
	if (!parent || !parent->image_container.image3D) return;

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

	// 1. TOP-LEFT: Patient Name / ID, Data, Série
	int tl_y = vr.top() + margin + lineHeight;
	QString patText = GraphicsView::is_anonymous_mode() ? QString("[ANÔNIMO]") : (iv->pat_name.isEmpty() ? iv->pat_id : iv->pat_name);
	if (!patText.isEmpty())
	{
		drawShadow(vr.left() + margin, tl_y, patText);
		tl_y += lineHeight;
	}

	if (!GraphicsView::is_anonymous_mode() && !iv->pat_id.isEmpty())
	{
		drawShadow(vr.left() + margin, tl_y, QString("ID: ") + iv->pat_id);
		tl_y += lineHeight;
	}

	if (!iv->study_date.isEmpty())
	{
		drawShadow(vr.left() + margin, tl_y, iv->study_date);
		tl_y += lineHeight;
	}

	if (!iv->series_description.isEmpty())
	{
		drawShadow(vr.left() + margin, tl_y, iv->series_description);
		tl_y += lineHeight;
	}

	// 2. TOP-RIGHT: Instituição, Modalidade
	int tr_y = vr.top() + margin + lineHeight;
	if (!iv->institution.isEmpty())
	{
		drawShadowRight(tr_y, iv->institution);
		tr_y += lineHeight;
	}

	if (!iv->modality.isEmpty())
	{
		drawShadowRight(tr_y, iv->modality);
		tr_y += lineHeight;
	}

	// 3. BOTTOM-LEFT: Im: X/Y, Loc: Z mm, Thk: X mm, Zoom
	int bl_y = vr.bottom() - margin;
	const int zoomPercent = static_cast<int>(std::round(transform().m11() * 100.0));
	drawShadow(vr.left() + margin, bl_y, QString("Zoom: %1%").arg(zoomPercent));
	bl_y -= lineHeight;

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

	// 4. BOTTOM-RIGHT: WL: X WW: Y
	int br_y = vr.bottom() - margin;
	if (parent->measure_label && !parent->measure_label->text().isEmpty())
	{
		drawShadowRight(br_y, parent->measure_label->text());
		br_y -= lineHeight;
	}

	const int curWL = static_cast<int>(std::round(iv->di->us_window_center));
	const int curWW = static_cast<int>(std::round(iv->di->us_window_width));
	drawShadowRight(br_y, QString("WL: %1  WW: %2").arg(curWL).arg(curWW));

	painter->restore();
}

void StudyGraphicsView::scale_view(double scale_factor)
{
	m_scale *= scale_factor;
	setTransform(transform().scale(scale_factor, scale_factor));
}

void StudyGraphicsView::zoom_in()
{
	scale_view(1.015);
}

void StudyGraphicsView::zoom_out()
{
	scale_view(1.0 / 1.015);
}

void StudyGraphicsView::clear_collision_paths()
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

void StudyGraphicsView::clear_us_regions()
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

void StudyGraphicsView::clear_prtexts_items()
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

void StudyGraphicsView::clear_prgraphicobjects_items()
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

void StudyGraphicsView::clear_shutters()
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

void StudyGraphicsView::mousePressEvent(QMouseEvent * e)
{
	parent->set_active();

	if (RadiantInputDispatcher::instance()->handle_study_mouse_press(this, e, parent))
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

	QGraphicsView::mousePressEvent(e);
}

void StudyGraphicsView::mouseDoubleClickEvent(QMouseEvent * e)
{
	QGraphicsView::mouseDoubleClickEvent(e);
}

void StudyGraphicsView::mouseReleaseEvent(QMouseEvent * e)
{
	if (RadiantInputDispatcher::instance()->handle_study_mouse_release(this, e, parent))
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

	QGraphicsView::mouseReleaseEvent(e);
}

void StudyGraphicsView::mouseMoveEvent(QMouseEvent * e)
{
	if (RadiantInputDispatcher::instance()->handle_study_mouse_move(this, e, parent))
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

	QGraphicsView::mouseMoveEvent(e);
}

void StudyGraphicsView::set_win_old_position(int x, int y)
{
	old_win_pos_x = x;
	old_win_pos_y = y;
}

void StudyGraphicsView::set_win_new_position(int x, int y)
{
	new_win_pos_x = x;
	new_win_pos_y = y;
}

void StudyGraphicsView::set_win_last_position(int x, int y)
{
	last_win_pos_x = x;
	last_win_pos_y = y;
}

void StudyGraphicsView::update_background_color()
{
	QColor bc = qApp->palette().color(QPalette::Window);
	setBackgroundBrush(QBrush(bc));
	shutter_color = std::move(bc);
}

void StudyGraphicsView::draw_us_regions()
{
	clear_us_regions();
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
		scene()->addItem(i);
	}
}

void StudyGraphicsView::set_empty_distance()
{
	QPainterPath path___;
	measurment_line->setPath(path___);
}

void StudyGraphicsView::measure()
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
		parent->set_measure_text(QString(""));
	}
}

void StudyGraphicsView::flipX()
{
	global_flip_x = !global_flip_x;
	parent->update_image(0);
}

void StudyGraphicsView::flipY()
{
	global_flip_y = !global_flip_y;
	parent->update_image(0);
}

void StudyGraphicsView::rotate_cw()
{
	rotation_angle = (rotation_angle + 90) % 360;
	if (parent)
		parent->update_image(0);
}

void StudyGraphicsView::rotate_ccw()
{
	rotation_angle = (rotation_angle + 270) % 360;
	if (parent)
		parent->update_image(0);
}

void StudyGraphicsView::reset_transformations()
{
	rotation_angle = 0;
	global_flip_x = false;
	global_flip_y = false;
	setTransform(QTransform());
	if (parent)
		parent->update_image(0);
}

void StudyGraphicsView::draw_prtexts(const ImageVariant * ivariant)
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
		;;
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
#if 1
		i->setFlag(QGraphicsItem::ItemIsMovable, false);
#else
		i->setFlag(QGraphicsItem::ItemIsMovable, true);
		i->setFlag(QGraphicsItem::ItemIsSelectable, true);
#endif
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
					bottom_right_posx =
						ivariant->di->idimx * l.at(x).bb_bottom_right_x;
					bottom_right_posy =
						ivariant->di->idimy * l.at(x).bb_bottom_right_y;
				}
			}
			else if (l.at(x).BoundingBoxAnnotationUnits == QString("PIXEL"))
			{
				posx = l.at(x).bb_top_left_x;
				posy = l.at(x).bb_top_left_y;
				bottom_right_posx = l.at(x).bb_bottom_right_x;
				bottom_right_posy = l.at(x).bb_bottom_right_y;
			}
			if (posx < bottom_right_posx && posy < bottom_right_posy)
			{
				;;
			}
			else if (posx > bottom_right_posx && posy > bottom_right_posy)
			{
				r = 180.0;
			}
			else if (posx < bottom_right_posx && posy > bottom_right_posy)
			{
				r = 270.0;
			}
			else if (posx > bottom_right_posx && posy < bottom_right_posy)
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
			QGraphicsPathItem * a = new QGraphicsPathItem();
#if 1
			a->setFlag(QGraphicsItem::ItemIsMovable, false);
#else
			a->setFlag(QGraphicsItem::ItemIsMovable, true);
			a->setFlag(QGraphicsItem::ItemIsSelectable, true);
#endif
			a->setFlag(QGraphicsItem::ItemIsFocusable, false);
			a->setAcceptHoverEvents(false);
			a->setPath(p);
			a->setPen(pen);
			a->setZValue(1e+18);
			scene()->addItem(a);
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
					static_cast<int>(R * 255.0),
					static_cast<int>(G * 255.0),
					static_cast<int>(B * 255.0),
					255));
			}
			else
			{
				i->setDefaultTextColor(QColor(150, 0, 150, 255));
			}
			i->setFont(font);
			if (l.at(x).ShadowStyle == QString("NORMAL"))
			{
				QGraphicsDropShadowEffect * shadow =
					new QGraphicsDropShadowEffect(i);
				shadow->setXOffset(1.5); // not supported
				shadow->setYOffset(1.5); // not supported
				double opacity = 255.0;
				if (l.at(x).ShadowOpacity > 0.0 && l.at(x).ShadowOpacity < 1.0)
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
							static_cast<int>(R * 255.0),
							static_cast<int>(G * 255.0),
							static_cast<int>(B * 255.0),
							static_cast<int>(opacity)));
				}
				else
				{
					shadow->setColor(QColor(128, 128, 128, static_cast<int>(opacity)));
				}
				i->setGraphicsEffect(shadow);
			}
			else if (l.at(x).ShadowStyle == QString("OUTLINED"))
			{
				;; // TODO
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

void StudyGraphicsView::draw_prgraphics(const ImageVariant * ivariant)
{
	// TODO complete
	clear_prgraphicobjects_items();
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
		;;
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
				p.moveTo(l.at(x).GraphicData.at(0), l.at(x).GraphicData.at(1));
				for (unsigned int y = 2; y < l.at(x).GraphicData.size(); y += 2)
				{
					p.lineTo(l.at(x).GraphicData.at(y), l.at(x).GraphicData.at(y + 1));
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
					for (unsigned int y = 0; y < l.at(x).GraphicData.size(); y += 2)
					{
						const double px =
							ivariant->di->idimx * l.at(x).GraphicData.at(y);
						const double py =
							ivariant->di->idimy * l.at(x).GraphicData.at(y + 1);
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
#if 1
				i->setFlag(QGraphicsItem::ItemIsMovable, false);
#else
				i->setFlag(QGraphicsItem::ItemIsMovable, true);
				i->setFlag(QGraphicsItem::ItemIsSelectable, true);
#endif
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
				const double d0 = sqrt(x0__ * x0__ + y0__ * y0__);
				const double d1 = sqrt(x1__ * x1__ + y1__ * y1__);
				const double ma_j = 1.0 / d0;
				const double ma_nx = x0__ * ma_j;
				const double ma_ny = y0__ * ma_j;
				const double mi_j = 1.0 / d1;
				const double mi_nx = x1__ * mi_j;
				const double mi_ny = y1__ * mi_j;
				const double start = 0.0;
				const double span = 360.0;
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
#if 1
				i->setFlag(QGraphicsItem::ItemIsMovable, false);
#else
				i->setFlag(QGraphicsItem::ItemIsMovable, true);
				i->setFlag(QGraphicsItem::ItemIsSelectable, true);
#endif
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
			double point_x = 0.0;
			double point_y = 0.0;
			if (l.at(x).GraphicAnnotationUnits == QString("PIXEL"))
			{
				center_x = l.at(x).GraphicData.at(0);
				center_y = l.at(x).GraphicData.at(1);
				point_x = l.at(x).GraphicData.at(2);
				point_y = l.at(x).GraphicData.at(3);
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
						2.0 * distance,
						2.0 * distance);
#if 1
				i->setFlag(QGraphicsItem::ItemIsMovable, false);
#else
				i->setFlag(QGraphicsItem::ItemIsMovable, true);
				i->setFlag(QGraphicsItem::ItemIsSelectable, true);
#endif
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
			if (l.at(x).GraphicData.size() != 2) continue;
			double point_x = 0.0;
			double point_y = 0.0;
			if (l.at(x).GraphicAnnotationUnits == QString("PIXEL"))
			{
				point_x = l.at(x).GraphicData.at(0);
				point_y = l.at(x).GraphicData.at(1);
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
#if 1
				i->setFlag(QGraphicsItem::ItemIsMovable, false);
#else
				i->setFlag(QGraphicsItem::ItemIsMovable, true);
				i->setFlag(QGraphicsItem::ItemIsSelectable, true);
#endif
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

void StudyGraphicsView::draw_shutter(const ImageVariant * ivariant)
{
	clear_shutters();
	if (!ivariant) return;
	if (ivariant->pr_display_shutters.empty()) return;
	int idx = -1;
	if (ivariant->pr_display_shutters.contains(-1))
	{
		;;
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
						ivariant->di->idimx+2,
						ivariant->di->idimy+2)));
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
			const int d = 2.0 * a.RadiusofCircularShutter;
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
					a.VerticesofthePolygonalShutter.at(j) - 1));
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

