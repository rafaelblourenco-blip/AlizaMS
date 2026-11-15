#ifndef RADIANTINPUTDISPATCHER_H
#define RADIANTINPUTDISPATCHER_H

#include <QObject>
#include <QPoint>
#include <QPointF>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QPointer>
#include <QTimer>

class GraphicsView;
class StudyGraphicsView;
class GraphicsWidget;
class StudyGraphicsWidget;
class GLWidget;
class Aliza;

enum class RadiantTool {
    Browse = 0,    // 'B' - Navegação de fatias (padrão)
    WindowLevel,   // 'W' - Ajuste de Janela (WL/WW)
    Pan,           // 'M' - Mover imagem
    Zoom,          // 'Z' - Zoom
    Length,        // 'L' - Medida linear (Régua)
    Ellipse,       // 'E' - ROI Elíptica
    ClosedPolygon, // 'G' - Polígono fechado
    OpenPolygon,   // 'O' - Polígono aberto
    Angle,         // 'A' - Ângulo simples
    CobbAngle,     // 'C' - Ângulo de Cobb
    Deviation,     // 'D' - Desvio
    Arrow,         // 'Y' - Seta
    Pen            // 'P' - Lápis livre
};

enum class DragAction {
    None = 0,
    Browse,
    WindowLevel,
    Zoom,
    Pan,
    MeasureLength,
    Rotate3D,
    Roll3D,
    SlabThickness
};

class RadiantInputDispatcher : public QObject
{
    Q_OBJECT
public:
    static RadiantInputDispatcher * instance();

    void set_tool(RadiantTool tool);
    RadiantTool get_tool() const { return current_tool; }

    // 2D Viewport (GraphicsView / GraphicsWidget)
    bool handle_mouse_press(GraphicsView * view, QMouseEvent * e, GraphicsWidget * gw);
    bool handle_mouse_move(GraphicsView * view, QMouseEvent * e, GraphicsWidget * gw);
    bool handle_mouse_release(GraphicsView * view, QMouseEvent * e, GraphicsWidget * gw);
    bool handle_wheel(GraphicsView * view, QWheelEvent * e, GraphicsWidget * gw);

    // Multi-View Study Viewport (StudyGraphicsView / StudyGraphicsWidget)
    bool handle_study_mouse_press(StudyGraphicsView * view, QMouseEvent * e, StudyGraphicsWidget * sgw);
    bool handle_study_mouse_move(StudyGraphicsView * view, QMouseEvent * e, StudyGraphicsWidget * sgw);
    bool handle_study_mouse_release(StudyGraphicsView * view, QMouseEvent * e, StudyGraphicsWidget * sgw);
    bool handle_study_wheel(StudyGraphicsView * view, QWheelEvent * e, StudyGraphicsWidget * sgw);

    // 3D Viewport (GLWidget)
    bool handle_gl_mouse_press(GLWidget * glw, QMouseEvent * e, Aliza * aliza);
    bool handle_gl_mouse_move(GLWidget * glw, QMouseEvent * e, Aliza * aliza);
    bool handle_gl_mouse_release(GLWidget * glw, QMouseEvent * e, Aliza * aliza);
    bool handle_gl_wheel(GLWidget * glw, QWheelEvent * e, Aliza * aliza);

    // Event coalescing and slice throttling
    void flush_pending_slices();

signals:
    void tool_changed(RadiantTool tool);

private slots:
    void dispatch_pending_slices();

private:
    explicit RadiantInputDispatcher(QObject * parent = nullptr);
    ~RadiantInputDispatcher() override = default;

    void queue_slice_step_gw(GraphicsWidget * gw, int steps);
    void queue_slice_step_sgw(StudyGraphicsWidget * sgw, int steps);

    RadiantTool current_tool{RadiantTool::Browse};
    DragAction current_action{DragAction::None};
    QPoint start_pos;
    QPoint last_pos;
    int accumulated_browse_dy{0};
    int gl_pan_last_x{0};
    int gl_pan_last_y{0};

    QTimer * throttle_timer{nullptr};
    QTimer * m_wheel_idle_timer{nullptr};
    QPointer<GraphicsWidget> pending_gw;
    QPointer<StudyGraphicsWidget> pending_sgw;
    QPointer<GraphicsWidget> m_last_wheel_gw;
    QPointer<StudyGraphicsWidget> m_last_wheel_sgw;
    int pending_steps_gw{0};
    int pending_steps_sgw{0};
    int m_wheel_accumulator_gw{0};
    int m_wheel_accumulator_sgw{0};
};

#endif // RADIANTINPUTDISPATCHER_H
