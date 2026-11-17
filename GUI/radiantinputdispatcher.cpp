#include "radiantinputdispatcher.h"
#include "graphicsview.h"
#include "graphicswidget.h"
#include "studygraphicsview.h"
#include "studygraphicswidget.h"
#include "studyviewwidget.h"
#include "sliderwidget.h"
#include "aliza.h"
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
#ifdef ALIZA_GL_3_2_CORE
#include "CG/glwidget-qt5-core.h"
#else
#include "CG/glwidget-qt5.h"
#endif
#else
#ifdef ALIZA_GL_3_2_CORE
#include "CG/glwidget-qt4-core.h"
#else
#include "CG/glwidget-qt4.h"
#endif
#endif
#include <QScrollBar>
#include <QApplication>
#include <cmath>

RadiantInputDispatcher::RadiantInputDispatcher(QObject * parent)
    : QObject(parent)
{
    throttle_timer = new QTimer(this);
    throttle_timer->setSingleShot(true);
    connect(throttle_timer, &QTimer::timeout, this, &RadiantInputDispatcher::dispatch_pending_slices);

    m_wheel_idle_timer = new QTimer(this);
    m_wheel_idle_timer->setSingleShot(true);
    connect(m_wheel_idle_timer, &QTimer::timeout, this, [this]() {
        m_wheel_accumulator_gw = 0;
        m_wheel_accumulator_sgw = 0;
    });
}

RadiantInputDispatcher * RadiantInputDispatcher::instance()
{
    static RadiantInputDispatcher inst;
    return &inst;
}

void RadiantInputDispatcher::set_tool(RadiantTool tool)
{
    if (current_tool != tool)
    {
        current_tool = tool;
        emit tool_changed(tool);
    }
}

void RadiantInputDispatcher::queue_slice_step_gw(GraphicsWidget * gw, int steps)
{
    if (!gw || steps == 0) return;
    if (gw->slider_m && gw->slider_m->slices_slider)
    {
        const int cur = gw->slider_m->slices_slider->value();
        const int max = gw->slider_m->slices_slider->maximum();
        const int min = gw->slider_m->slices_slider->minimum();
        int next = cur + steps;
        if (next < min) next = min;
        if (next > max) next = max;
        if (next != cur)
        {
            gw->slider_m->set_slice(next);
        }
    }
}

void RadiantInputDispatcher::queue_slice_step_sgw(StudyGraphicsWidget * sgw, int steps)
{
    if (!sgw || steps == 0) return;
    if (sgw->get_slider())
    {
        const int cur = sgw->get_slider()->value();
        const int max = sgw->get_slider()->maximum();
        int next = cur + steps;
        if (next < 0) next = 0;
        if (next > max) next = max;
        if (next != cur)
        {
            sgw->get_slider()->setValue(next);
        }
    }
}

void RadiantInputDispatcher::dispatch_pending_slices()
{
    if (pending_gw && pending_steps_gw != 0)
    {
        if (pending_gw->slider_m && pending_gw->slider_m->slices_slider)
        {
            const int cur = pending_gw->slider_m->slices_slider->value();
            const int max = pending_gw->slider_m->slices_slider->maximum();
            const int min = pending_gw->slider_m->slices_slider->minimum();
            int next = cur + pending_steps_gw;
            if (next < min) next = min;
            if (next > max) next = max;
            pending_steps_gw = 0;
            if (next != cur)
            {
                pending_gw->slider_m->set_slice(next);
            }
        }
        else
        {
            pending_steps_gw = 0;
        }
        pending_gw = nullptr;
    }
    else
    {
        pending_steps_gw = 0;
        pending_gw = nullptr;
    }

    if (pending_sgw && pending_steps_sgw != 0)
    {
        if (pending_sgw->get_slider())
        {
            const int cur = pending_sgw->get_slider()->value();
            const int max = pending_sgw->get_slider()->maximum();
            int next = cur + pending_steps_sgw;
            if (next < 0) next = 0;
            if (next > max) next = max;
            pending_steps_sgw = 0;
            if (next != cur)
            {
                pending_sgw->get_slider()->setValue(next);
            }
        }
        else
        {
            pending_steps_sgw = 0;
        }
        pending_sgw = nullptr;
    }
    else
    {
        pending_steps_sgw = 0;
        pending_sgw = nullptr;
    }
}

void RadiantInputDispatcher::flush_pending_slices()
{
    if (throttle_timer && throttle_timer->isActive())
    {
        throttle_timer->stop();
    }
    if (m_wheel_idle_timer && m_wheel_idle_timer->isActive())
    {
        m_wheel_idle_timer->stop();
    }
    dispatch_pending_slices();
    m_wheel_accumulator_gw = 0;
    m_wheel_accumulator_sgw = 0;
}

// =========================================================================
// 2D VIEWPORT (GraphicsView / GraphicsWidget)
// =========================================================================

bool RadiantInputDispatcher::handle_mouse_press(GraphicsView * view, QMouseEvent * e, GraphicsWidget * gw)
{
    start_pos = e->pos();
    last_pos = e->pos();
    accumulated_browse_dy = 0;

    const Qt::MouseButton btn = e->button();
    const Qt::KeyboardModifiers mods = e->modifiers();

    // 1. Instant Overrides with Left Mouse Button
    if (btn == Qt::LeftButton)
    {
        if (mods & Qt::ControlModifier)
        {
            current_action = DragAction::WindowLevel;
            return true;
        }
        if (mods & Qt::ShiftModifier)
        {
            current_action = DragAction::Pan;
            return true;
        }

        // Active primary tool execution
        switch (current_tool)
        {
        case RadiantTool::Browse:
            current_action = DragAction::Browse;
            return true;
        case RadiantTool::WindowLevel:
            current_action = DragAction::WindowLevel;
            return true;
        case RadiantTool::Pan:
            current_action = DragAction::Pan;
            return true;
        case RadiantTool::Zoom:
            current_action = DragAction::Zoom;
            return true;
        case RadiantTool::Length:
            current_action = DragAction::MeasureLength;
            return false; // let GraphicsView initiate ruler tracking
        default:
            current_action = DragAction::None;
            break;
        }
    }
    else if (btn == Qt::MiddleButton)
    {
        // MMB Drag: Window / Level
        current_action = DragAction::WindowLevel;
        return true;
    }
    else if (btn == Qt::RightButton)
    {
        // RMB Drag: Zoom In / Zoom Out
        current_action = DragAction::Zoom;
        return true;
    }
    else if (btn == Qt::BackButton)
    {
        // Mouse Button 4: Pan
        current_action = DragAction::Pan;
        return true;
    }
    else if (btn == Qt::ForwardButton)
    {
        // Mouse Button 5: Ruler (Length)
        current_action = DragAction::MeasureLength;
        return false; // let GraphicsView handle ruler coordinates
    }

    return false;
}

bool RadiantInputDispatcher::handle_mouse_move(GraphicsView * view, QMouseEvent * e, GraphicsWidget * gw)
{
    if (current_action == DragAction::None)
    {
        return false;
    }

    if (current_action == DragAction::WindowLevel)
    {
        const int dx = e->pos().x() - last_pos.x();
        const int dy = last_pos.y() - e->pos().y(); // upward increases brightness/center
        last_pos = e->pos();

        const double delta_w = static_cast<double>(dx) * 2.0;
        const double delta_c = static_cast<double>(dy) * 2.0;

        if (gw && gw->get_aliza())
        {
            gw->get_aliza()->adjust_wl_delta(delta_c, delta_w);
        }
        return true;
    }
    else if (current_action == DragAction::Zoom)
    {
        const int dy = last_pos.y() - e->pos().y(); // upward zooms in
        last_pos = e->pos();

        const double factor = std::pow(1.008, static_cast<double>(dy));
        if (view)
        {
            view->scale_view(factor);
        }
        return true;
    }
    else if (current_action == DragAction::Pan)
    {
        const int dx = e->pos().x() - last_pos.x();
        const int dy = e->pos().y() - last_pos.y();
        last_pos = e->pos();

        if (view)
        {
            view->horizontalScrollBar()->setValue(view->horizontalScrollBar()->value() - dx);
            view->verticalScrollBar()->setValue(view->verticalScrollBar()->value() - dy);
        }
        return true;
    }
    else if (current_action == DragAction::Browse)
    {
        const int dy = e->pos().y() - last_pos.y();
        last_pos = e->pos();
        accumulated_browse_dy += dy;

        const int step_px = 4;
        if (std::abs(accumulated_browse_dy) >= step_px)
        {
            const int steps = -accumulated_browse_dy / step_px;
            accumulated_browse_dy %= step_px;
            queue_slice_step_gw(gw, steps);
        }
        return true;
    }

    return false;
}

bool RadiantInputDispatcher::handle_mouse_release(GraphicsView * view, QMouseEvent * e, GraphicsWidget * gw)
{
    Q_UNUSED(view);
    Q_UNUSED(e);
    Q_UNUSED(gw);
    const DragAction prev = current_action;
    current_action = DragAction::None;
    accumulated_browse_dy = 0;
    flush_pending_slices();
    return (prev != DragAction::None && prev != DragAction::MeasureLength);
}

bool RadiantInputDispatcher::handle_wheel(GraphicsView * view, QWheelEvent * e, GraphicsWidget * gw)
{
    if (e->modifiers() & Qt::ControlModifier)
    {
        // Ctrl + Wheel: Dynamic Zoom focused on cursor
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
        const int delta = e->angleDelta().y();
#else
        const int delta = e->delta();
#endif
        const double factor = std::pow(2.0, delta / 480.0);
        if (view)
        {
            view->scale_view(factor);
        }
        return true;
    }
    else
    {
        // Normal Wheel: Coalesced, VSync-throttled slice navigation
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
        const int delta = e->angleDelta().y();
#else
        const int delta = e->delta();
#endif
        if (m_last_wheel_gw != gw)
        {
            m_last_wheel_gw = gw;
            m_wheel_accumulator_gw = 0;
        }

        m_wheel_accumulator_gw += delta;
        if (m_wheel_idle_timer)
        {
            m_wheel_idle_timer->start(200);
        }

        const int threshold = 120; // 1 notch = 1 slice step
        if (std::abs(m_wheel_accumulator_gw) >= threshold)
        {
            const int steps = m_wheel_accumulator_gw / threshold;
            m_wheel_accumulator_gw %= threshold;
            queue_slice_step_gw(gw, steps);
        }
        return true;
    }
}

// =========================================================================
// MULTI-VIEW STUDY VIEWPORT (StudyGraphicsView / StudyGraphicsWidget)
// =========================================================================

bool RadiantInputDispatcher::handle_study_mouse_press(StudyGraphicsView * view, QMouseEvent * e, StudyGraphicsWidget * sgw)
{
    start_pos = e->pos();
    last_pos = e->pos();
    accumulated_browse_dy = 0;

    const Qt::MouseButton btn = e->button();
    const Qt::KeyboardModifiers mods = e->modifiers();

    if (btn == Qt::LeftButton)
    {
        if (mods & Qt::ControlModifier)
        {
            current_action = DragAction::WindowLevel;
            return true;
        }
        if (mods & Qt::ShiftModifier)
        {
            current_action = DragAction::Pan;
            return true;
        }

        switch (current_tool)
        {
        case RadiantTool::Browse:
            current_action = DragAction::Browse;
            return true;
        case RadiantTool::WindowLevel:
            current_action = DragAction::WindowLevel;
            return true;
        case RadiantTool::Pan:
            current_action = DragAction::Pan;
            return true;
        case RadiantTool::Zoom:
            current_action = DragAction::Zoom;
            return true;
        case RadiantTool::Length:
            current_action = DragAction::MeasureLength;
            return false;
        default:
            current_action = DragAction::None;
            break;
        }
    }
    else if (btn == Qt::MiddleButton)
    {
        current_action = DragAction::WindowLevel;
        return true;
    }
    else if (btn == Qt::RightButton)
    {
        current_action = DragAction::Zoom;
        return true;
    }
    else if (btn == Qt::BackButton)
    {
        current_action = DragAction::Pan;
        return true;
    }
    else if (btn == Qt::ForwardButton)
    {
        current_action = DragAction::MeasureLength;
        return false;
    }

    return false;
}

bool RadiantInputDispatcher::handle_study_mouse_move(StudyGraphicsView * view, QMouseEvent * e, StudyGraphicsWidget * sgw)
{
    if (current_action == DragAction::None)
    {
        return false;
    }

    if (current_action == DragAction::WindowLevel)
    {
        const int dx = e->pos().x() - last_pos.x();
        const int dy = last_pos.y() - e->pos().y();
        last_pos = e->pos();

        const double delta_w = static_cast<double>(dx) * 2.0;
        const double delta_c = static_cast<double>(dy) * 2.0;

        if (sgw)
        {
            double cur_c = sgw->image_container.us_window_center_ext;
            double cur_w = sgw->image_container.us_window_width_ext;
            if (cur_w < 1.0 && sgw->image_container.image3D && sgw->image_container.image3D->di)
            {
                cur_c = sgw->image_container.image3D->di->us_window_center;
                cur_w = sgw->image_container.image3D->di->us_window_width;
            }
            double new_c = cur_c + delta_c;
            double new_w = cur_w + delta_w;
            if (new_w < 1.0) new_w = 1.0;

            sgw->image_container.level_locked_ext = false;
            if (sgw->image_container.image3D && sgw->image_container.image3D->di)
            {
                sgw->image_container.image3D->di->lock_level2D = false;
                sgw->image_container.image3D->di->us_window_center = new_c;
                sgw->image_container.image3D->di->us_window_width = new_w;
            }

            sgw->set_center(new_c);
            sgw->set_width(new_w);
        }
        return true;
    }
    else if (current_action == DragAction::Zoom)
    {
        const int dy = last_pos.y() - e->pos().y();
        last_pos = e->pos();

        const double factor = std::pow(1.008, static_cast<double>(dy));
        if (view)
        {
            view->scale_view(factor);
        }
        return true;
    }
    else if (current_action == DragAction::Pan)
    {
        const int dx = e->pos().x() - last_pos.x();
        const int dy = e->pos().y() - last_pos.y();
        last_pos = e->pos();

        if (view)
        {
            view->horizontalScrollBar()->setValue(view->horizontalScrollBar()->value() - dx);
            view->verticalScrollBar()->setValue(view->verticalScrollBar()->value() - dy);
        }
        return true;
    }
    else if (current_action == DragAction::Browse)
    {
        const int dy = e->pos().y() - last_pos.y();
        last_pos = e->pos();
        accumulated_browse_dy += dy;

        const int step_px = 5;
        if (std::abs(accumulated_browse_dy) >= step_px)
        {
            const int steps = -accumulated_browse_dy / step_px;
            accumulated_browse_dy %= step_px;
            queue_slice_step_sgw(sgw, steps);
        }
        return true;
    }

    return false;
}

bool RadiantInputDispatcher::handle_study_mouse_release(StudyGraphicsView * view, QMouseEvent * e, StudyGraphicsWidget * sgw)
{
    Q_UNUSED(view);
    Q_UNUSED(e);
    Q_UNUSED(sgw);
    const DragAction prev = current_action;
    current_action = DragAction::None;
    accumulated_browse_dy = 0;
    flush_pending_slices();
    return (prev != DragAction::None && prev != DragAction::MeasureLength);
}

bool RadiantInputDispatcher::handle_study_wheel(StudyGraphicsView * view, QWheelEvent * e, StudyGraphicsWidget * sgw)
{
    if (e->modifiers() & Qt::ControlModifier)
    {
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
        const int delta = e->angleDelta().y();
#else
        const int delta = e->delta();
#endif
        const double factor = std::pow(2.0, delta / 480.0);
        if (view)
        {
            view->scale_view(factor);
        }
        return true;
    }
    else
    {
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
        const int delta = e->angleDelta().y();
#else
        const int delta = e->delta();
#endif
        if (m_last_wheel_sgw != sgw)
        {
            m_last_wheel_sgw = sgw;
            m_wheel_accumulator_sgw = 0;
            if (sgw) sgw->set_active();
        }

        m_wheel_accumulator_sgw += delta;
        if (m_wheel_idle_timer)
        {
            m_wheel_idle_timer->start(200);
        }

        const int threshold = 120; // 1 notch = 1 slice step
        if (std::abs(m_wheel_accumulator_sgw) >= threshold)
        {
            const int steps = m_wheel_accumulator_sgw / threshold;
            m_wheel_accumulator_sgw %= threshold;
            if (sgw) sgw->set_active();
            queue_slice_step_sgw(sgw, steps);
        }
        return true;
    }
}

// =========================================================================
// 3D VIEWPORT (GLWidget)
// =========================================================================

bool RadiantInputDispatcher::handle_gl_mouse_press(GLWidget * glw, QMouseEvent * e, Aliza * aliza)
{
    start_pos = e->pos();
    last_pos = e->pos();
    gl_pan_last_x = e->pos().x();
    gl_pan_last_y = e->pos().y();

    const Qt::MouseButton btn = e->button();
    const Qt::KeyboardModifiers mods = e->modifiers();

    if (btn == Qt::LeftButton)
    {
        if ((mods & Qt::ControlModifier) && (mods & Qt::AltModifier))
        {
            current_action = DragAction::Roll3D;
            return true;
        }
        if ((mods & Qt::ControlModifier) && (mods & Qt::ShiftModifier))
        {
            current_action = DragAction::SlabThickness;
            return true;
        }
        if (mods & Qt::AltModifier)
        {
            current_action = DragAction::Rotate3D;
            return true;
        }
        if (mods & Qt::ControlModifier)
        {
            current_action = DragAction::WindowLevel;
            return true;
        }
        if (mods & Qt::ShiftModifier)
        {
            current_action = DragAction::Pan;
            return true;
        }
        // Default Left Click in 3D: Rotate
        current_action = DragAction::Rotate3D;
        return false; // let GLWidget do standard rotation tracking
    }
    else if (btn == Qt::MiddleButton)
    {
        // MMB: Window/Level in 3D
        current_action = DragAction::WindowLevel;
        return true;
    }
    else if (btn == Qt::RightButton)
    {
        // RMB: Continuous smooth zoom in 3D
        current_action = DragAction::Zoom;
        return true;
    }
    else if (btn == Qt::BackButton)
    {
        current_action = DragAction::Pan;
        return true;
    }

    return false;
}

bool RadiantInputDispatcher::handle_gl_mouse_move(GLWidget * glw, QMouseEvent * e, Aliza * aliza)
{
    if (current_action == DragAction::None)
    {
        return false;
    }

    if (current_action == DragAction::WindowLevel)
    {
        const int dx = e->pos().x() - last_pos.x();
        const int dy = last_pos.y() - e->pos().y();
        last_pos = e->pos();

        if (aliza)
        {
            aliza->adjust_wl_delta(static_cast<double>(dy) * 2.0, static_cast<double>(dx) * 2.0);
        }
        if (glw)
        {
            glw->updateGL();
        }
        return true;
    }
    else if (current_action == DragAction::Zoom)
    {
        const int dy = last_pos.y() - e->pos().y();
        last_pos = e->pos();

        if (glw)
        {
            const float tmp0 = glw->ortho_size - static_cast<float>(dy);
            glw->ortho_size = (tmp0 < 0.001f) ? 0.001f : tmp0;
            glw->updateGL();
        }
        return true;
    }
    else if (current_action == DragAction::Pan)
    {
        const int deltax = e->pos().x() - gl_pan_last_x;
        const int deltay = e->pos().y() - gl_pan_last_y;
        gl_pan_last_x = e->pos().x();
        gl_pan_last_y = e->pos().y();

        if (glw)
        {
            glw->set_pan_delta(deltax, deltay);
            glw->updateGL();
        }
        return true;
    }
    else if (current_action == DragAction::Rotate3D)
    {
        const QPoint p = e->pos();
        if (glw)
        {
            glw->set_win_old_position(last_pos.x(), last_pos.y());
            glw->set_win_new_position(p.x(), p.y());
            last_pos = p;
            glw->updateGL();
        }
        return true;
    }

    return false;
}

bool RadiantInputDispatcher::handle_gl_mouse_release(GLWidget * glw, QMouseEvent * e, Aliza * aliza)
{
    Q_UNUSED(glw);
    Q_UNUSED(e);
    Q_UNUSED(aliza);
    const DragAction prev = current_action;
    current_action = DragAction::None;
    return (prev != DragAction::None);
}

bool RadiantInputDispatcher::handle_gl_wheel(GLWidget * glw, QWheelEvent * e, Aliza * aliza)
{
    Q_UNUSED(aliza);
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
    const int delta = e->angleDelta().y();
#else
    const int delta = e->delta();
#endif
    if (glw)
    {
        const float incr = (e->modifiers() & Qt::ControlModifier) ? 1.0f : 4.0f;
        if (delta > 0)
        {
            glw->ortho_size += incr;
        }
        else
        {
            glw->ortho_size -= incr;
            if (glw->ortho_size < 0.001f) glw->ortho_size = 0.001f;
        }
        glw->updateGL();
    }
    return true;
}
