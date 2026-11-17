#include "structures.h"
#include "matrixbutton.h"
#include "lutwidget.h"
#include "studyframewidget.h"
#include "studygraphicswidget.h"
#include "aliza.h"
#include "studyviewwidget.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QVariant>
#include <QKeySequence>
#include <QMenu>
#include <cmath>
#include <algorithm>
#include <vector>
#if MATRIX_BUTTON_CUSTOM_ACT == 1
#include "tabledialog.h"
#endif
#if 0
#include <iostream>
#endif

StudyViewWidget::StudyViewWidget(bool vertical, bool tab, float si)
	:
	horizontal(!vertical),
	in_tab(tab)
{
	setupUi(this);
	//
	const QSize s1 = QSize(static_cast<int>(18 * si),static_cast<int>(18 * si));
	lockon = QIcon(QString(":/bitmaps/lock.svg"));
	lockoff = QIcon(QString(":/bitmaps/unlock.svg"));
	resetlevel_pushButton->setIconSize(s1);
	lock_pushButton->setIconSize(s1);
	level1_frame->hide();
	mbutton = new MatrixButton(si);
	fitall_Action = new QAction(QIcon(QString(":/bitmaps/f2.svg")), QString("Fit to view"), this);
	scouts_Action = new QAction(QIcon(QString(":/bitmaps/collisions.svg")), QString("Show intersections"), this);
	scouts_Action->setCheckable(true);
	scouts_Action->setChecked(true);
	measure_Action = new QAction(QIcon(QString(":/bitmaps/distance.svg")), QString("Measurement"), this);
	measure_Action->setCheckable(true);
	anchor_Action = new QAction(QIcon(QString(":/bitmaps/anchor.svg")), QString("Anchor sliders for side-by-side view"), this);
	anchor_Action->setCheckable(true);
	if (in_tab)
	{
		close_Action = nullptr;
	}
	else
	{
		close_Action = new QAction(QIcon(QString(":/bitmaps/delete2.svg")), QString("Close"), this);
	}
	QWidget * spacer1 = new QWidget(this);
	spacer1->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	QToolBar * toolbar = new QToolBar(this);
	toolbar->setOrientation(Qt::Horizontal);
	toolbar->setIconSize(s1);
	toolbar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	if (toolbar->layout())
	{
		toolbar->layout()->setContentsMargins(0, 0, 0, 0);
		toolbar->layout()->setSpacing(0);
	}
	toolbar->addWidget(mbutton);
	toolbar->addAction(fitall_Action);
	toolbar->addAction(scouts_Action);
	toolbar->addAction(measure_Action);
	toolbar->addAction(anchor_Action);
	toolbar->addWidget(spacer1);
	if (!in_tab)
	{
		toolbar->addAction(close_Action);
	}
	QHBoxLayout * l1 = new QHBoxLayout(toolbar_frame);
	l1->setContentsMargins(0, 0, 0, 0);
	l1->setSpacing(4);
	l1->addWidget(toolbar);
	lutwidget  = new LUTWidget(si);
	lutwidget->add_items1();
	QVBoxLayout * l2 = new QVBoxLayout(lut_frame);
	l2->setContentsMargins(0, 0, 0, 0);
	l2->setSpacing(0);
	l2->addWidget(lutwidget);
	QGridLayout * gridLayout = new QGridLayout(scrollAreaWidgetContents);
	(void)gridLayout;
	//
	if (in_tab)
	{
		close_sc = nullptr;
#ifdef __APPLE__
		minimaze_sc = nullptr;
		fullsceen_sc = nullptr;
		normal_sc = nullptr;
#endif
	}
	else
	{
		close_sc = new QShortcut(QKeySequence::Close, this, SLOT(check_close()));
		close_sc->setAutoRepeat(false);
#ifdef __APPLE__
		minimaze_sc = new QShortcut(QKeySequence("Ctrl+M"), this, SLOT(showMinimized()));
		minimaze_sc->setAutoRepeat(false);
		fullsceen_sc = new QShortcut(QKeySequence("Ctrl+Meta+F"),this,SLOT(showFullScreen()));
		fullsceen_sc->setAutoRepeat(false);
		normal_sc = new QShortcut(QKeySequence("Esc"),this,SLOT(showNormal()));
		normal_sc->setAutoRepeat(false);
#endif
	}
	//
	readSettings();
}

QMenu * StudyViewWidget::create_toolbar_menu(QWidget * menuParent)
{
	QMenu * menu = new QMenu(menuParent);
	QMenu * layouts = menu->addMenu(tr("Layout do estudo"));
	for (int rows = 1; rows <= 4; ++rows)
		for (int columns = 1; columns <= 4; ++columns)
		{
			QAction * action = layouts->addAction(QString("%1 x %2").arg(rows).arg(columns));
			connect(action, &QAction::triggered, this, [this, rows, columns]() {
				update_grid(rows, columns);
			});
		}
	menu->addAction(fitall_Action);
	menu->addAction(scouts_Action);
	menu->addAction(measure_Action);
	menu->addAction(anchor_Action);
	// Detached study windows still need their own controls.
	if (in_tab) toolbar_frame->hide();
	return menu;
}

StudyViewWidget::~StudyViewWidget()
{
	QGridLayout * layout = static_cast<QGridLayout*>(scrollAreaWidgetContents->layout());
	if (layout)
	{
		const int r = layout->rowCount();
		const int c = layout->columnCount();
		for (int x = 0; x < r; ++x)
		{
			for (int y = 0; y < c; ++y)
			{
				QLayoutItem * li = layout->itemAtPosition(x, y);
				if (li)
				{
					QWidget * w = li->widget();
					if (w) layout->removeWidget(w);
				}
			}
		}
	}
	for (int x = 0; x < widgets.size(); ++x)
	{
		if (widgets.at(x))
		{
			if (widgets[x]->graphicswidget)
			{
				widgets[x]->graphicswidget->clear_();
			}
			delete widgets[x];
			widgets[x] = nullptr;
		}
	}
	widgets.clear();
}

void StudyViewWidget::init_(Aliza * a)
{
	aliza = a;
	const int widgets_size = 25;
	for (int x = 0; x < widgets_size; ++x)
	{
		StudyGraphicsWidget * w = new StudyGraphicsWidget();
		StudyFrameWidget * f = new StudyFrameWidget(w);
		w->set_id(x);
		w->set_studyview(this);
		w->set_aliza(aliza);
		w->set_slider(f->slider);
		w->set_top_label(f->top_label);
		w->set_left_label(f->left_label);
		w->set_measure_label(f->measure_label);
		w->set_icon_button(f->icon_button);
		connect(f, &StudyFrameWidget::close_requested, this, &StudyViewWidget::close_viewport);
		f->hide();
		widgets.push_back(f);
	}
	update_null();
	connect(
		mbutton, SIGNAL(matrix_selected(int, int)),
		this, SLOT(update_grid(int, int)));
	connect(
		fitall_Action, SIGNAL(triggered()),
		this, SLOT(all_to_fit()));
	connect(
		scouts_Action, SIGNAL(toggled(bool)),
		this, SLOT(toggle_scouts(bool)));
	connect(
		measure_Action, SIGNAL(toggled(bool)),
		this, SLOT(toggle_measure(bool)));
#if MATRIX_BUTTON_CUSTOM_ACT == 1
	connect(
		mbutton->p_action, SIGNAL(triggered()),
		this, SLOT(update_grid2()));
#endif
	if (in_tab)
	{
		calculate_grid(2);
	}
	else
	{
		connect(
			close_Action, SIGNAL(triggered()),
			this, SLOT(close()));
	}
	connect_tools();
}

void StudyViewWidget::invalidate_reference_lines()
{
    const auto next = property("referenceGeneration").toULongLong() + 1;
    setProperty("referenceGeneration", QVariant::fromValue<qulonglong>(next));
    for (StudyFrameWidget *frame : widgets)
        if (frame && frame->graphicswidget && frame->graphicswidget->graphicsview)
            frame->graphicswidget->graphicsview->clear_collision_paths();
}

void StudyViewWidget::clear_()
{
    invalidate_reference_lines();
	active_id = -1;
	saved_r = -1;
	saved_c = -1;
	update_null();
	for (int i = 0; i < widgets.size(); ++i)
	{
		if (widgets.at(i))
		{
			if (widgets.at(i)->graphicswidget)
			{
				widgets[i]->graphicswidget->clear_();
			}
			if (widgets.at(i)->frame0->frameShape() != QFrame::StyledPanel)
			{
				widgets[i]->frame0->setFrameShape(QFrame::StyledPanel);
			}
			if (widgets.at(i)->icon_button->isChecked())
			{
				widgets[i]->icon_button->blockSignals(true);
				widgets[i]->icon_button->setChecked(false);
				widgets[i]->icon_button->blockSignals(false);
			}
		}
	}
	QGridLayout * layout = static_cast<QGridLayout*>(scrollAreaWidgetContents->layout());
	if (!layout) return;
	const int r = layout->rowCount();
	const int c = layout->columnCount();
	for (int x = 0; x < r; ++x)
	{
		for (int y = 0; y < c; ++y)
		{
			QLayoutItem * li = layout->itemAtPosition(x, y);
			if (li)
			{
				QWidget * w = li->widget();
				if (w)
				{
					layout->removeWidget(w);
					w->hide();
				}
			}
		}
	}
	delete layout;
	layout = new QGridLayout(scrollAreaWidgetContents);
}

bool StudyViewWidget::get_in_tab() const
{
	return in_tab;
}

void StudyViewWidget::calculate_grid(int x)
{
	const bool h = this->width() > this->height();
	int r = 1;
	int c = 1;
	switch(x)
	{
	case 1:
		{
			r = 1;
			c = 1;
		}
		break;
	case 2:
		{
			if (h)
			{
				r = 1;
				c = 2;
			}
			else
			{
				r = 2;
				c = 1;
			}
		}
		break;
	case 3:
		{
			if (h)
			{
				r = 1;
				c = 3;
			}
			else
			{
				r = 3;
				c = 1;
			}
		}
		break;
	case 4:
		{
			r = 2;
			c = 2;
		}
		break;
	case 5:
	case 6:
		{
			if (h)
			{
				r = 2;
				c = 3;
			}
			else
			{
				r = 3;
				c = 2;
			}
		}
		break;
	case 7:
	case 8:
		{
			if (h)
			{
				r = 2;
				c = 4;
			}
			else
			{
				r = 4;
				c = 2;
			}
		}
		break;
	case 9:
		{
			r = 3;
			c = 3;
		}
		break;
	case 10:
		{
			if (h)
			{
				r = 2;
				c = 5;
			}
			else
			{
				r = 5;
				c = 2;
			}
		}
		break;
	case 11:
	case 12:
		{
			if (h)
			{
				r = 3;
				c = 4;
			}
			else
			{
				r = 4;
				c = 3;
			}
		}
		break;
	case 13:
	case 14:
	case 15:
	case 16:
		{
			r = 4;
			c = 4;
		}
		break;
	case 17:
	case 18:
	case 19:
	case 20:
		{
			if (h)
			{
				r = 4;
				c = 5;
			}
			else
			{
				r = 5;
				c = 4;
			}
		}
		break;
	case 21:
	case 22:
	case 23:
	case 24:
	case 25:
	default:
		{
			r = 5;
			c = 5;
		}
		break;
	}
	update_grid(r, c);
}

void StudyViewWidget::connect_tools()
{
	connect(center_horizontalSlider,SIGNAL(valueChanged(int)),       this,SLOT(set_center_slider(int)));
	connect(width_horizontalSlider, SIGNAL(valueChanged(int)),       this,SLOT(set_width_slider(int)));
	connect(center_doubleSpinBox,   SIGNAL(valueChanged(double)),    this,SLOT(set_center_spinbox(double)));
	connect(width_doubleSpinBox,    SIGNAL(valueChanged(double)),    this,SLOT(set_width_spinbox(double)));
	connect(comboBox,               SIGNAL(currentIndexChanged(int)),this,SLOT(set_lut_function(int)));
	connect(lutwidget->comboBox,    SIGNAL(currentIndexChanged(int)),this,SLOT(set_lut(int)));
	connect(lock_pushButton,        SIGNAL(toggled(bool)),           this,SLOT(toggle_lock_window(bool)));
	connect(resetlevel_pushButton,  SIGNAL(clicked()),               this,SLOT(reset_level()));
}

void StudyViewWidget::disconnect_tools()
{
	disconnect(center_horizontalSlider,SIGNAL(valueChanged(int)),       this,SLOT(set_center_slider(int)));
	disconnect(width_horizontalSlider, SIGNAL(valueChanged(int)),       this,SLOT(set_width_slider(int)));
	disconnect(center_doubleSpinBox,   SIGNAL(valueChanged(double)),    this,SLOT(set_center_spinbox(double)));
	disconnect(width_doubleSpinBox,    SIGNAL(valueChanged(double)),    this,SLOT(set_width_spinbox(double)));
	disconnect(comboBox,               SIGNAL(currentIndexChanged(int)),this,SLOT(set_lut_function(int)));
	disconnect(lutwidget->comboBox,    SIGNAL(currentIndexChanged(int)),this,SLOT(set_lut(int)));
	disconnect(lock_pushButton,        SIGNAL(toggled(bool)),           this,SLOT(toggle_lock_window(bool)));
	disconnect(resetlevel_pushButton,  SIGNAL(clicked()),               this,SLOT(reset_level()));
}

void StudyViewWidget::block_signals(bool t)
{
	center_horizontalSlider->blockSignals(t);
	width_horizontalSlider->blockSignals(t);
	center_doubleSpinBox->blockSignals(t);
	width_doubleSpinBox->blockSignals(t);
	comboBox->blockSignals(t);
	lutwidget->comboBox->blockSignals(t);
	lock_pushButton->blockSignals(t);
	resetlevel_pushButton->blockSignals(t);
}

void StudyViewWidget::closeEvent(QCloseEvent * e)
{
	clear_();
	e->accept();
}

void StudyViewWidget::update_grid(int r, int c)
{
    invalidate_reference_lines();
	for (int x = 0; x < widgets.size(); ++x)
	{
		if (widgets.at(x))
		{
			if (widgets.at(x)->icon_button->isChecked())
			{
				widgets[x]->icon_button->blockSignals(true);
				widgets[x]->icon_button->setChecked(false);
				widgets[x]->icon_button->blockSignals(false);
			}
		}
	}
	QGridLayout * layout = static_cast<QGridLayout*>(scrollAreaWidgetContents->layout());
	if (layout)
	{
		const int ar = layout->rowCount();
		const int ac = layout->columnCount();
		for (int x = 0; x < ar; ++x)
		{
			for (int y = 0; y < ac; ++y)
			{
				QLayoutItem * li = layout->itemAtPosition(x, y);
				if (li)
				{
					QWidget * w = li->widget();
					if (w)
					{
						layout->removeWidget(w);
						w->hide();
					}
				}
			}
		}
		delete layout;
		layout = nullptr;
	}
	QGridLayout * gridLayout = new QGridLayout(scrollAreaWidgetContents);
	int j = 0;
	for (int x = 0; x < r; ++x)
	{
		for (int y = 0; y < c; ++y)
		{
			gridLayout->addWidget(widgets.at(j), x, y);
			widgets[j]->show();
			widgets[j]->graphicswidget->show();
			++j;
		}
	}
	j = 0;
	for (int x = 0; x < r; ++x)
	{
		for (int y = 0; y < c; ++y)
		{
			widgets[j]->graphicswidget->update_image(1);
			++j;
		}
	}
	qApp->processEvents();
    update_scouts_now();
}

void StudyViewWidget::update_grid2()
{
#if MATRIX_BUTTON_CUSTOM_ACT == 1
	bool ok = false;
	int r = 0, c = 0;
	TableDialog * d = new TableDialog();
	if (d->exec() == QDialog::Accepted)
	{
		ok = true;
		r = d->get_rows();
		c = d->get_columns();
	}
	delete d;
	if (ok) update_grid(r, c);
#endif
}

int StudyViewWidget::get_active_id() const
{
	return active_id;
}

void StudyViewWidget::set_active_id(int x)
{
	active_id = x;
}

bool StudyViewWidget::get_scouts() const
{
	return scouts_Action->isChecked();
}

void StudyViewWidget::set_active_image(int id)
{
	if (id < 0)
	{
		active_id = -1;
		for (int i = 0; i < widgets.size(); ++i)
		{
			if (widgets.at(i))
			{
				if (widgets.at(i)->frame0->frameShape() != QFrame::StyledPanel)
				{
					widgets[i]->frame0->setFrameShape(QFrame::StyledPanel);
				}
			}
		}
		update_null();
		return;
	}
	if (active_id == id) return;
	active_id = id;
	ImageContainer * c{};
	for (int i = 0; i < widgets.size(); ++i)
	{
		if (widgets.at(i))
		{
			if (widgets.at(i)->graphicswidget)
			{
				if (widgets.at(i)->graphicswidget->image_container.image3D)
				{
					if (widgets.at(i)->graphicswidget->get_id() == active_id)
					{
						c = &widgets.at(i)->graphicswidget->image_container;
						widgets[i]->frame0->setFrameShape(QFrame::Box);
					}
					else
					{
						widgets[i]->frame0->setFrameShape(QFrame::StyledPanel);
					}
				}
				else
				{
					widgets[i]->frame0->setFrameShape(QFrame::StyledPanel);
				}
			}
			else
			{
				widgets[i]->frame0->setFrameShape(QFrame::StyledPanel);
			}
		}
	}
	level_frame->setEnabled(true);
	update_full(c);
	update_scouts();
}

void StudyViewWidget::update_full(ImageContainer * c)
{
	if (!c) return; // not required
	const ImageVariant * v = c->image3D;
	if (!v) return; // not required
	block_signals(true);
	update_max_width(v->di->rmax - v->di->rmin + 1.0);
	update_window_upper(v->di->rmax);
	update_window_lower(v->di->rmin);
	update_locked_window(c->level_locked_ext);
	if (v->image_type >= 0 && v->image_type < 10)
	{
		if (v->frame_levels.contains(c->selected_z_slice_ext))
		{
			const FrameLevel & fl = v->frame_levels.value(c->selected_z_slice_ext);
			update_locked_center(fl.us_window_center);
			update_locked_width(fl.us_window_width);
		}
		else
		{
			update_locked_center(v->di->default_us_window_center);
			update_locked_width(v->di->default_us_window_width);
		}
	}
	else
	{
		update_locked_center(0.0);
		update_locked_width(0.0);
	}
	update_spinbox_width_d(c->us_window_width_ext);
	update_spinbox_center_d(c->us_window_center_ext);
	if (v->di->disable_int_level)
	{
		center_horizontalSlider->setEnabled(false);
		width_horizontalSlider->setEnabled(false);
		center_horizontalSlider->setValue(0);
		width_horizontalSlider->setValue(0);
	}
	else
	{
		center_horizontalSlider->setEnabled(true);
		width_horizontalSlider->setEnabled(true);
		center_horizontalSlider->setValue(
			static_cast<int>(c->us_window_center_ext));
		width_horizontalSlider->setValue(
			static_cast<int>(c->us_window_width_ext));
	}
	update_lut_function(c->lut_function_ext);
	lutwidget->set_lut(c->selected_lut_ext);
	block_signals(false);
}

void StudyViewWidget::update_level(ImageContainer * c)
{
	if (!c) return;
	if (!c->image3D) return;
	const ImageVariant * v = c->image3D;
	if (c->level_locked_ext)
	{
		if (v->image_type >= 0 && v->image_type < 10)
		{
			if (v->frame_levels.contains(c->selected_z_slice_ext))
			{
				const FrameLevel & fl = v->frame_levels.value(c->selected_z_slice_ext);
				update_locked_center(fl.us_window_center);
				update_locked_width(fl.us_window_width);
			}
			else
			{
				update_locked_center(v->di->default_us_window_center);
				update_locked_width(v->di->default_us_window_width);
			}
		}
		else
		{
			update_locked_center(0.0);
			update_locked_width(0.0);
		}
	}
#if 0
	else // not required for changed slice
	{
		center_horizontalSlider->blockSignals(true);
		width_horizontalSlider->blockSignals(true);
		center_doubleSpinBox->blockSignals(true);
		width_doubleSpinBox->blockSignals(true);
		update_spinbox_center_d(c->us_window_center_ext);
		update_spinbox_width_d(c->us_window_width_ext);
		center_horizontalSlider->setValue(static_cast<int>(c->us_window_center_ext));
		width_horizontalSlider->setValue(static_cast<int>(c->us_window_width_ext));
		center_horizontalSlider->blockSignals(false);
		width_horizontalSlider->blockSignals(false);
		center_doubleSpinBox->blockSignals(false);
		width_doubleSpinBox->blockSignals(false);
	}
#endif
}

void StudyViewWidget::set_center_slider(int x)
{
	center_doubleSpinBox->blockSignals(true);
	const double k = static_cast<double>(x);
	for (int i = 0; i < widgets.size(); ++i)
	{
		if (widgets.at(i))
		{
			if (widgets.at(i)->graphicswidget)
			{
				if (widgets.at(i)->graphicswidget->image_container.image3D)
				{
					if (widgets.at(i)->graphicswidget->get_id() == active_id)
					{
						widgets[i]->graphicswidget->set_center(k);
					}
				}
			}
		}
	}
	center_doubleSpinBox->setValue(k);
	center_doubleSpinBox->blockSignals(false);
}

void StudyViewWidget::set_width_slider(int x)
{
	width_doubleSpinBox->blockSignals(true);
	const double k = static_cast<double>(x);
	for (int i = 0; i < widgets.size(); ++i)
	{
		if (widgets.at(i))
		{
			if (widgets.at(i)->graphicswidget)
			{
				if (widgets.at(i)->graphicswidget->image_container.image3D)
				{
					if (widgets.at(i)->graphicswidget->get_id() == active_id)
					{
						widgets[i]->graphicswidget->set_width(k);
					}
				}
			}
		}
	}
	width_doubleSpinBox->setValue(k);
	width_doubleSpinBox->blockSignals(false);
}

void StudyViewWidget::set_center_spinbox(double x)
{
	center_horizontalSlider->blockSignals(true);
	for (int i = 0; i < widgets.size(); ++i)
	{
		if (widgets.at(i))
		{
			if (widgets.at(i)->graphicswidget)
			{
				if (widgets.at(i)->graphicswidget->image_container.image3D)
				{
					if (widgets.at(i)->graphicswidget->get_id() == active_id)
					{
						widgets[i]->graphicswidget->set_center(x);
					}
				}
			}
		}
	}
	center_horizontalSlider->setValue(static_cast<int>(x));
	center_horizontalSlider->blockSignals(false);
}

void StudyViewWidget::set_width_spinbox(double x)
{
	width_horizontalSlider->blockSignals(true);
	for (int i = 0; i < widgets.size(); ++i)
	{
		if (widgets.at(i))
		{
			if (widgets.at(i)->graphicswidget)
			{
				if (widgets.at(i)->graphicswidget->image_container.image3D)
				{
					if (widgets.at(i)->graphicswidget->get_id() == active_id)
					{
						widgets[i]->graphicswidget->set_width(x);
					}
				}
			}
		}
	}
	width_horizontalSlider->setValue(static_cast<int>(x));
	width_horizontalSlider->blockSignals(false);
}

void StudyViewWidget::set_lut_function(int x)
{
	for (int i = 0; i < widgets.size(); ++i)
	{
		if (widgets.at(i))
		{
			if (widgets.at(i)->graphicswidget)
			{
				if (widgets.at(i)->graphicswidget->image_container.image3D)
				{
					if (widgets.at(i)->graphicswidget->get_id() == active_id)
					{
						widgets[i]->graphicswidget->set_lut_function(x);
					}
				}
			}
		}
	}
}

void StudyViewWidget::set_lut(int x)
{
	const short k = static_cast<short>(x);
	for (int i = 0; i < widgets.size(); ++i)
	{
		if (widgets.at(i))
		{
			if (widgets.at(i)->graphicswidget)
			{
				if (widgets.at(i)->graphicswidget->image_container.image3D)
				{
					if (widgets.at(i)->graphicswidget->get_id() == active_id)
					{
						widgets[i]->graphicswidget->set_lut(k);
					}
				}
			}
		}
	}
}

void StudyViewWidget::toggle_lock_window(bool t)
{
	center2_doubleSpinBox->setEnabled(t);
	width2_doubleSpinBox->setEnabled(t);
	if (t)
	{
		lock_pushButton->setIcon(lockon);
		level1_frame->hide();
		level0_frame->show();
		level_spacer->show();
	}
	else
	{
		lock_pushButton->setIcon(lockoff);
		level0_frame->hide();
		level_spacer->hide();
		level1_frame->show();
	}
	for (int i = 0; i < widgets.size(); ++i)
	{
		if (widgets.at(i))
		{
			if (widgets.at(i)->graphicswidget)
			{
				if (widgets.at(i)->graphicswidget->image_container.image3D)
				{
					if (widgets.at(i)->graphicswidget->get_id() == active_id)
					{
						widgets[i]->graphicswidget->set_locked_window(t);
					}
				}
			}
		}
	}
	level1_frame->setEnabled(!t);
}

void StudyViewWidget::reset_level()
{
	block_signals(true);
	double c{};
	double w{};
	short l{};
	bool once{};
	for (int i = 0; i < widgets.size(); ++i)
	{
		if (widgets.at(i))
		{
			if (widgets.at(i)->graphicswidget)
			{
				if (widgets.at(i)->graphicswidget->image_container.image3D)
				{
					if (widgets.at(i)->graphicswidget->get_id() == active_id)
					{
						widgets[i]->graphicswidget->reset_level();
						if (!once)
						{
							c = widgets.at(i)->graphicswidget->image_container.us_window_center_ext;
							w = widgets.at(i)->graphicswidget->image_container.us_window_width_ext;
							l = widgets.at(i)->graphicswidget->image_container.lut_function_ext;
							once = true;
						}
					}
				}
			}
		}
	}
	center_horizontalSlider->setValue(static_cast<int>(c));
	width_horizontalSlider->setValue(static_cast<int>(w));
	center_doubleSpinBox->setValue(c);
	width_doubleSpinBox->setValue(w);
	comboBox->setCurrentIndex(l);
	block_signals(false);
}

void StudyViewWidget::all_to_fit()
{
	for (int i = 0; i < widgets.size(); ++i)
	{
		if (widgets.at(i))
		{
			if (widgets.at(i)->graphicswidget)
			{
				if (widgets.at(i)->graphicswidget->image_container.image3D)
				{
					widgets[i]->graphicswidget->update_image(1);
				}
			}
		}
	}
}

void StudyViewWidget::all_to_original()
{
}

void StudyViewWidget::toggle_scouts(bool t)
{
	if (t)
	{
		update_scouts_now();
	}
	else
	{
		if (m_scouts_debounce_timer && m_scouts_debounce_timer->isActive())
		{
			m_scouts_debounce_timer->stop();
		}
		for (int i = 0; i < widgets.size(); ++i)
		{
			if (widgets.at(i) && widgets.at(i)->graphicswidget)
			{
				widgets[i]->graphicswidget->graphicsview->clear_collision_paths();
			}
		}
	}
}

void StudyViewWidget::toggle_measure(bool t)
{
	const short x = (t) ? 2 : 0;
	for (int i = 0; i < widgets.size(); ++i)
	{
		if (widgets.at(i) && widgets.at(i)->graphicswidget)
		{
			widgets[i]->graphicswidget->set_mouse_modus(x, true);
		}
	}
}

void StudyViewWidget::check_close() // FIXME
{
	if (isFullScreen())
	{
		showNormal();
	}
	else
	{
		close();
	}
}

void StudyViewWidget::update_locked_window(bool t)
{
	lock_pushButton->setChecked(t);
	center2_doubleSpinBox->setEnabled(t);
	width2_doubleSpinBox->setEnabled(t);
	if (t)
	{
		lock_pushButton->setIcon(lockon);
		level1_frame->hide();
		level0_frame->show();
		level_spacer->show();
	}
	else
	{
		lock_pushButton->setIcon(lockoff);
		level0_frame->hide();
		level_spacer->hide();
		level1_frame->show();
	}
	level1_frame->setEnabled(!t);
}

void StudyViewWidget::update_lut_function(int x)
{
	comboBox->setCurrentIndex(x);
}

void StudyViewWidget::update_spinbox_width_d(double i)
{
	width_doubleSpinBox->setValue(i);
}

void StudyViewWidget::update_spinbox_center_d(double i)
{
	center_doubleSpinBox->setValue(i);
}

void StudyViewWidget::update_window_upper(double i)
{
	center_doubleSpinBox->setMaximum(i);
	center_horizontalSlider->setMaximum(static_cast<int>(i));
}

void StudyViewWidget::update_window_lower(double i)
{
	center_doubleSpinBox->setMinimum(i);
	center_horizontalSlider->setMinimum(static_cast<int>(i));
}

void StudyViewWidget::update_max_width(double i)
{
	width_doubleSpinBox->setMinimum(0.000001);
	width_horizontalSlider->setMinimum(0);
	width_doubleSpinBox->setMaximum(i);
	width_horizontalSlider->setMaximum(static_cast<int>(i));
}

void StudyViewWidget::update_locked_center(double x)
{
	center2_doubleSpinBox->setValue(x);
}

void StudyViewWidget::update_locked_width(double x)
{
	width2_doubleSpinBox->setValue(x);
}

void StudyViewWidget::update_null()
{
	block_signals(true);
	level_frame->setEnabled(false);
	center_horizontalSlider->setValue(0);
	width_horizontalSlider->setValue(1);
	update_max_width(1);
	update_window_upper(1);
	update_window_lower(0);
	update_locked_center(0.0);
	update_locked_width(0.0);
	update_spinbox_width_d(1);
	update_spinbox_center_d(0);
	update_lut_function(0);
	lutwidget->set_lut(0);
	block_signals(false);
}

void StudyViewWidget::update_scouts()
{
	if (!scouts_Action || !scouts_Action->isChecked()) return;

	if (!m_scouts_debounce_timer)
	{
		m_scouts_debounce_timer = new QTimer(this);
		m_scouts_debounce_timer->setSingleShot(true);
		connect(m_scouts_debounce_timer, &QTimer::timeout, this, [this]() {
			if (scouts_Action && scouts_Action->isChecked())
			{
				m_scouts_last_update_ms = QDateTime::currentMSecsSinceEpoch();
				emit update_scouts_required();
			}
		});
	}

	const qint64 now = QDateTime::currentMSecsSinceEpoch();
	// Throttle: if more than 100ms has elapsed since the last reference lines computation during continuous scrolling, allow an update
	if (m_scouts_last_update_ms > 0 && (now - m_scouts_last_update_ms) >= 100)
	{
		m_scouts_debounce_timer->stop();
		m_scouts_last_update_ms = now;
		emit update_scouts_required();
		return;
	}

	// Debounce: reset timer for 60ms trailing edge
	m_scouts_debounce_timer->start(60);
}

void StudyViewWidget::update_scouts_now()
{
	if (m_scouts_debounce_timer && m_scouts_debounce_timer->isActive())
	{
		m_scouts_debounce_timer->stop();
	}
	if (scouts_Action && scouts_Action->isChecked())
	{
		m_scouts_last_update_ms = QDateTime::currentMSecsSinceEpoch();
		emit update_scouts_required();
	}
}

bool StudyViewWidget::get_anchored_sliders() const
{
	return (sync_mode != SyncModeOff && sync_slice_position) || (anchor_Action && anchor_Action->isChecked());
}

StudyViewWidget::PlaneType StudyViewWidget::get_plane_type(const ImageVariant * iv)
{
	if (!iv || !iv->di) return PlaneUnknown;
	const double * dc = iv->di->dircos;
	double nx = dc[1] * dc[5] - dc[2] * dc[4];
	double ny = dc[2] * dc[3] - dc[0] * dc[5];
	double nz = dc[0] * dc[4] - dc[1] * dc[3];
	double ax = std::abs(nx);
	double ay = std::abs(ny);
	double az = std::abs(nz);
	if (az >= ax && az >= ay && az > 1e-4) return PlaneAxial;
	if (ax >= ay && ax >= az && ax > 1e-4) return PlaneSagittal;
	if (ay >= ax && ay >= az && ay > 1e-4) return PlaneCoronal;
	if (!iv->orientation_string.isEmpty()) {
		QString s = iv->orientation_string.toUpper();
		if (s.contains("AXIAL") || s.contains("TRA") || s.contains("AXI")) return PlaneAxial;
		if (s.contains("SAG")) return PlaneSagittal;
		if (s.contains("COR")) return PlaneCoronal;
	}
	return PlaneAxial;
}

void StudyViewWidget::set_sync_mode(SeriesSyncMode mode)
{
	sync_mode = mode;
	if (anchor_Action) anchor_Action->setChecked(sync_mode != SyncModeOff);
}

void StudyViewWidget::cycle_sync_mode()
{
	if (sync_mode == SyncModeOff) set_sync_mode(SyncModeAuto);
	else if (sync_mode == SyncModeAuto) set_sync_mode(SyncModeManual);
	else set_sync_mode(SyncModeOff);
}

void StudyViewWidget::toggle_single_mode(int wid)
{
	if (is_single_mode())
	{
		restore_multi();
	}
	else
	{
		QGridLayout * layout = static_cast<QGridLayout*>(scrollAreaWidgetContents->layout());
		if (layout && layout->rowCount() * layout->columnCount() > 1)
		{
			set_single(wid);
		}
	}
}

void StudyViewWidget::sync_zoom_pan_from(int wid, double scale, const QPointF & center)
{
	if (!sync_zoom_pan || sync_mode == SyncModeOff) return;
	if (wid < 0 || wid >= widgets.size() || !widgets[wid] || !widgets[wid]->graphicswidget) return;
	ImageVariant * src_iv = widgets[wid]->graphicswidget->image_container.image3D;
	if (!src_iv) return;
	PlaneType src_plane = get_plane_type(src_iv);

	for (int k = 0; k < widgets.size(); ++k)
	{
		if (k == wid || !widgets[k] || !widgets[k]->graphicswidget) continue;
		StudyGraphicsWidget * tgt = widgets[k]->graphicswidget;
		ImageVariant * tgt_iv = tgt->image_container.image3D;
		if (!tgt_iv) continue;
		if (get_plane_type(tgt_iv) != src_plane) continue;

		tgt->graphicsview->m_scale = scale;
		tgt->graphicsview->resetTransform();
		tgt->graphicsview->scale(scale, scale);
		tgt->graphicsview->centerOn(center);
	}
}

void StudyViewWidget::sync_window_from(int wid, double center, double width)
{
	if (!sync_window_settings) return;
	for (int k = 0; k < widgets.size(); ++k)
	{
		if (k == wid || !widgets[k] || !widgets[k]->graphicswidget) continue;
		StudyGraphicsWidget * tgt = widgets[k]->graphicswidget;
		if (!tgt || !tgt->image_container.image3D) continue;
		tgt->set_center(center);
		tgt->set_width(width);
	}
}

void StudyViewWidget::update_all_sliders(int wid, int x, int dimz)
{
	if (wid < 0 || wid >= widgets.size() || !widgets[wid] || !widgets[wid]->graphicswidget) return;
	StudyGraphicsWidget * src = widgets[wid]->graphicswidget;
	ImageVariant * src_iv = src->image_container.image3D;
	if (!src_iv || !src_iv->di) return;

	int delta = x - prev_slice[wid];
	prev_slice[wid] = x;

	if (sync_mode == SyncModeOff || !sync_slice_position)
	{
		update_scouts();
		return;
	}

	PlaneType src_plane = get_plane_type(src_iv);
	const double * sdc = src_iv->di->dircos;
	double snx = sdc[1] * sdc[5] - sdc[2] * sdc[4];
	double sny = sdc[2] * sdc[3] - sdc[0] * sdc[5];
	double snz = sdc[0] * sdc[4] - sdc[1] * sdc[3];
	double sn_len = std::sqrt(snx * snx + sny * sny + snz * snz);
	if (sn_len > 1e-6) { snx /= sn_len; sny /= sn_len; snz /= sn_len; }
	else { snx = 0; sny = 0; snz = 1; }

	double d_src = 0.0;
	bool src_has_physical = false;
	if (x >= 0 && x < static_cast<int>(src_iv->di->image_slices.size()))
	{
		const double * sipp = src_iv->di->image_slices[x]->ipp_iop;
		d_src = sipp[0] * snx + sipp[1] * sny + sipp[2] * snz;
		src_has_physical = true;
	}

	for (int k = 0; k < widgets.size(); ++k)
	{
		if (k == wid || !widgets[k] || !widgets[k]->graphicswidget) continue;
		StudyGraphicsWidget * tgt = widgets[k]->graphicswidget;
		ImageVariant * tgt_iv = tgt->image_container.image3D;
		if (!tgt_iv || !tgt_iv->di) continue;

		// Images of different cutting planes NEVER roll together
		PlaneType tgt_plane = get_plane_type(tgt_iv);
		if (src_plane != tgt_plane) continue;

		const int tgt_dimz = tgt_iv->di->idimz;
		if (tgt_dimz <= 0) continue;

		int new_slice = -1;

		if (sync_mode == SyncModeAuto)
		{
			if (src_has_physical && !tgt_iv->di->image_slices.empty())
			{
				double min_dist = 1e12;
				int best_idx = -1;
				for (size_t s = 0; s < tgt_iv->di->image_slices.size(); ++s)
				{
					const double * tipp = tgt_iv->di->image_slices[s]->ipp_iop;
					double d_tgt = tipp[0] * snx + tipp[1] * sny + tipp[2] * snz;
					double dist = std::abs(d_tgt - d_src);
					if (dist < min_dist)
					{
						min_dist = dist;
						best_idx = static_cast<int>(s);
					}
				}
				if (best_idx >= 0) new_slice = best_idx;
			}
			if (new_slice < 0)
			{
				if (dimz > 1)
					new_slice = static_cast<int>(std::round((static_cast<double>(x) * (tgt_dimz - 1)) / static_cast<double>(dimz - 1)));
				else
					new_slice = 0;
			}
		}
		else if (sync_mode == SyncModeManual)
		{
			int cur_tgt = tgt->image_container.selected_z_slice_ext;
			new_slice = cur_tgt + delta;
		}

		if (new_slice < 0) new_slice = 0;
		if (new_slice >= tgt_dimz) new_slice = tgt_dimz - 1;

		prev_slice[k] = new_slice;
		tgt->set_slider_only(new_slice);
		tgt->set_selected_slice2(new_slice, false);
	}
	update_scouts();
}

void StudyViewWidget::set_single(int wid)
{
    invalidate_reference_lines();
	QGridLayout * layout = static_cast<QGridLayout*>(scrollAreaWidgetContents->layout());
	if (!layout)
	{
		// should never happen
		saved_r = -1;
		saved_c = -1;
		active_id = -1;
		update_null();
		return;
	}
	saved_r = layout->rowCount();
	saved_c = layout->columnCount();
	StudyFrameWidget * selected = nullptr;
	for (int x = 0; x < saved_r; ++x)
	{
		for (int y = 0; y < saved_c; ++y)
		{
			QLayoutItem * li = layout->itemAtPosition(x, y);
			if (li)
			{
				QWidget * w = li->widget();
				if (w)
				{
					layout->removeWidget(w);
					StudyFrameWidget * f = static_cast<StudyFrameWidget*>(w);
					if (f->graphicswidget && f->graphicswidget->get_id() == wid)
					{
						selected = f;
					}
					else
					{
						// should not happen
						if (f->icon_button->isChecked())
						{
							f->icon_button->blockSignals(true);
							f->icon_button->setChecked(false);
							f->icon_button->blockSignals(false);
						}
					}
					if (f->frame0->frameShape() != QFrame::StyledPanel)
					{
						f->frame0->setFrameShape(QFrame::StyledPanel);
					}
					w->hide();
				}
			}
		}
	}
	delete layout;
	layout = nullptr;
	active_id = -1;
	if (selected)
	{
		QGridLayout * gridLayout = new QGridLayout(scrollAreaWidgetContents);
		gridLayout->addWidget(selected, 0, 0);
		selected->show();
		if (selected->graphicswidget)
		{
			selected->graphicswidget->show();
			set_active_image(selected->graphicswidget->get_id());
			selected->graphicswidget->update_image(1);
		}
		else
		{
			// should not happen
			update_null();
		}
	}
	else
	{
		// should not happen
		update_null();
	}
	qApp->processEvents();
}

void StudyViewWidget::restore_multi()
{
	if (saved_r == -1 || saved_c == -1)
	{
		saved_r = -1;
		saved_c = -1;
		active_id = -1;
		update_null();
		return;
	}
	update_grid(saved_r, saved_c);
	saved_r = -1;
	saved_c = -1;
	for (int x = 0; x < widgets.size(); ++x)
	{
		if (widgets.at(x))
		{
			if (widgets.at(x)->icon_button->isChecked())
			{
				widgets[x]->icon_button->blockSignals(true);
				widgets[x]->icon_button->setChecked(false);
				widgets[x]->icon_button->blockSignals(false);
			}
		}
	}
}

void StudyViewWidget::readSettings()
{
	const int w = horizontal ? 1000 : 700;
	const int h = horizontal ? 700  : 1000;
	QSettings settings(
		QSettings::IniFormat, QSettings::UserScope,
		QApplication::organizationName(), QApplication::applicationName());
	settings.setFallbacksEnabled(false);
	settings.beginGroup(QString("StudyViewWidget"));
	resize(settings.value(QString("size"), QSize(w, h)).toSize());
	move(settings.value(QString("pos"), QPoint(50, 50)).toPoint());
	settings.endGroup();
}

void StudyViewWidget::writeSettings(QSettings & settings)
{
	settings.beginGroup(QString("StudyViewWidget"));
	if (!isMaximized() && !isFullScreen())
	{
		settings.setValue(QString("size"), QVariant(this->size()));
		settings.setValue(QString("pos"), QVariant(this->pos()));
	}
	settings.endGroup();
}

void StudyViewWidget::update_background_color()
{
	for (int x = 0; x < widgets.size(); ++x)
	{
		if (widgets.at(x) && widgets.at(x)->graphicswidget)
		{
			widgets[x]->graphicswidget->update_background_color();
		}
	}
}

void StudyViewWidget::set_next_panel()
{
	if (widgets.isEmpty()) return;
	int next = (active_id + 1) % widgets.size();
	set_active_id(next);
}

void StudyViewWidget::set_prev_panel()
{
	if (widgets.isEmpty()) return;
	int prev = (active_id - 1 + widgets.size()) % widgets.size();
	set_active_id(prev);
}

void StudyViewWidget::toggle_sync_series()
{
	if (anchor_Action) anchor_Action->trigger();
}




void StudyViewWidget::load_series_split(int series_id)
{
	if (!aliza) return;
	ImageVariant * iv = aliza->get_image(series_id);
	if (!iv) return;

	if (saved_r != -1 && saved_c != -1)
	{
		restore_multi();
	}

	// Collect currently loaded unique images in studyview
	std::vector<ImageVariant*> loaded_images;
	for (int i = 0; i < widgets.size(); ++i)
	{
		if (widgets[i]->graphicswidget && widgets[i]->graphicswidget->image_container.image3D)
		{
			ImageVariant * v = widgets[i]->graphicswidget->image_container.image3D;
			if (std::find(loaded_images.begin(), loaded_images.end(), v) == loaded_images.end())
			{
				loaded_images.push_back(v);
			}
		}
	}

	// If studyview had no active images loaded, check if single view has an image
	if (loaded_images.empty())
	{
		ImageVariant * current_v = aliza->get_selected_image();
		if (current_v)
		{
			loaded_images.push_back(current_v);
		}
	}

	// Add new series image if not already in list
	if (std::find(loaded_images.begin(), loaded_images.end(), iv) == loaded_images.end())
	{
		loaded_images.push_back(iv);
	}

	int count = static_cast<int>(loaded_images.size());
	if (count < 2) count = 2;

	int r = 1;
	int c = 2;
	if (count == 2) { r = 1; c = 2; }
	else if (count == 3) { r = 1; c = 3; }
	else if (count == 4) { r = 2; c = 2; }
	else if (count <= 6) { r = 2; c = 3; }
	else if (count <= 8) { r = 2; c = 4; }
	else if (count <= 9) { r = 3; c = 3; }
	else { r = (count + 3) / 4; c = 4; }

	// Clear existing widgets
	for (int i = 0; i < widgets.size(); ++i)
	{
		widgets[i]->graphicswidget->clear_();
	}

	// Update grid layout
	update_grid(r, c);

	// Load images into each cell
	for (size_t i = 0; i < loaded_images.size() && static_cast<int>(i) < widgets.size(); ++i)
	{
		widgets[i]->graphicswidget->set_image(loaded_images[i], 1, true);
	}

	// Focus the newly added series
	for (int i = 0; i < r * c && i < widgets.size(); ++i)
	{
		if (widgets[i]->graphicswidget->get_id() == iv->id)
		{
			set_active_image(widgets[i]->graphicswidget->get_id());
			break;
		}
	}

	update_scouts_now();
}

void StudyViewWidget::close_viewport(StudyFrameWidget * frame)
{
	if (!frame) return;

	// Invalidate scout lines
	invalidate_reference_lines();

	// Clear the target frame
	frame->graphicswidget->clear_();

	// Collect remaining active images
	std::vector<ImageVariant*> remaining;
	for (int i = 0; i < widgets.size(); ++i)
	{
		if (widgets[i] != frame && widgets[i]->graphicswidget && widgets[i]->graphicswidget->image_container.image3D)
		{
			remaining.push_back(widgets[i]->graphicswidget->image_container.image3D);
		}
	}

	const int count = static_cast<int>(remaining.size());

	// If no remaining images, clear all and reset to 1x1 empty
	if (count == 0)
	{
		clear_();
		update_grid(1, 1);
		active_id = -1;
		update_null();
		return;
	}

	int r = 1;
	int c = 1;
	if (count == 1) { r = 1; c = 1; }
	else if (count == 2) { r = 1; c = 2; }
	else if (count == 3) { r = 1; c = 3; }
	else if (count == 4) { r = 2; c = 2; }
	else if (count <= 6) { r = 2; c = 3; }
	else if (count <= 8) { r = 2; c = 4; }
	else if (count <= 9) { r = 3; c = 3; }
	else { r = (count + 3) / 4; c = 4; }

	// Clear all widgets
	for (int i = 0; i < widgets.size(); ++i)
	{
		widgets[i]->graphicswidget->clear_();
	}

	update_grid(r, c);

	for (int i = 0; i < count && i < widgets.size(); ++i)
	{
		widgets[i]->graphicswidget->set_image(remaining[i], 1, true);
	}

	if (!remaining.empty())
	{
		set_active_image(widgets[0]->graphicswidget->get_id());
	}
	else
	{
		active_id = -1;
		update_null();
	}

	update_scouts_now();
}
