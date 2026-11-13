#ifndef RADIANTINPUTDISPATCHER_H
#define RADIANTINPUTDISPATCHER_H

#include <QObject>
#include <QPoint>
#include <QPointF>
#include <QMouseEvent>
#include <QWheelEvent>

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

signals:
    void tool_changed(RadiantTool tool);

private:
    explicit RadiantInputDispatcher(QObject * parent = nullptr);
    ~RadiantInputDispatcher() override = default;

    RadiantTool current_tool{RadiantTool::Browse};
    DragAction current_action{DragAction::None};
    QPoint start_pos;
    QPoint last_pos;
    int accumulated_browse_dy{0};
    int gl_pan_last_x{0};
    int gl_pan_last_y{0};
};

#endif // RADIANTINPUTDISPATCHER_H
