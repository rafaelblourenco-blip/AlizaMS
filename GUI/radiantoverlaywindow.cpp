#include "radiantoverlaywindow.h"
#include "mainwindow.h"
#include "graphicswidget.h"
#include "mprstate.h"
#include "graphicsview.h"
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
#include "radiantinputdispatcher.h"
#include "radiantshortcutsmanager.h"
#include "radiantexportdialog.h"
#include <QVBoxLayout>
#include <QMenu>
#include <QToolButton>
#include <QIcon>
#include <QSize>
#include <QSettings>
#include <QApplication>
#include <QActionGroup>

RadiantOverlayWindow::RadiantOverlayWindow(
    const QString & title,
    QWidget * contentWidget,
    MainWindow * parentWindow,
    OverlayType type)
    : QWidget(parentWindow, Qt::Window | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint),
      mw(parentWindow), content(contentWidget), overlay_type(type)
{
    setWindowTitle(title);
    resize(960, 680);
    setStyleSheet(
        "RadiantOverlayWindow { background-color: #071728; border: 2px solid #1E3A5F; }");

    // Auto-detect overlay type if Generic
    if (overlay_type == OverlayType::Generic)
    {
        if (title.contains("3D", Qt::CaseInsensitive) ||
            title.contains("Renderização", Qt::CaseInsensitive) ||
            title.contains("Tridimensional", Qt::CaseInsensitive) ||
            (content && content->findChild<GLWidget*>() != nullptr))
        {
            overlay_type = OverlayType::VR_3D;
        }
        else if (title.contains("MPR", Qt::CaseInsensitive) ||
                 title.contains("Multiplanar", Qt::CaseInsensitive) ||
                 (content && !content->findChildren<GraphicsWidget*>().isEmpty()))
        {
            overlay_type = OverlayType::MPR;
        }
    }

    main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(4, 4, 4, 4);
    main_layout->setSpacing(2);

    setup_toolbar();

    if (content)
    {
        main_layout->addWidget(content);
        content->show();
    }
}

RadiantOverlayWindow::~RadiantOverlayWindow()
{
}

void RadiantOverlayWindow::setup_toolbar()
{
    toolbar = new QToolBar(QString("RadiantOverlayToolBar"), this);
    toolbar->setObjectName(QString("RadiantOverlayToolBar"));
    toolbar->setIconSize(QSize(22, 22));
    toolbar->setMovable(false);
    toolbar->setStyleSheet(
        "QToolBar { background-color: #0D2137; border: none; border-bottom: 1px solid #173350; spacing: 3px; padding: 3px 6px; }"
        "QToolBar::separator { width: 1px; background-color: #173350; margin: 4px 6px; }");

    tool_action_group = new QActionGroup(this);

    if (overlay_type == OverlayType::MPR)
    {
        setup_mpr_toolbar();
    }
    else if (overlay_type == OverlayType::VR_3D)
    {
        setup_3d_toolbar();
    }

    main_layout->addWidget(toolbar);
}

QToolButton * RadiantOverlayWindow::create_menu_button(QAction * act, QMenu * menu, bool instantPopup)
{
    QToolButton * btn = new QToolButton(toolbar);
    btn->setDefaultAction(act);
    if (menu)
    {
        btn->setMenu(menu);
        btn->setPopupMode(instantPopup ? QToolButton::InstantPopup : QToolButton::MenuButtonPopup);
    }
    toolbar->addWidget(btn);
    return btn;
}

void RadiantOverlayWindow::setup_mpr_toolbar()
{
    // 1. WL Presets with Dropdown
    act_mpr_wl = new QAction(QIcon(":/bitmaps/radiant_wl.svg"), QString("Janelamento WL/WW (W)"), this);
    act_mpr_wl->setCheckable(true);
    tool_action_group->addAction(act_mpr_wl);
    connect(act_mpr_wl, &QAction::triggered, [this]() {
        RadiantInputDispatcher::instance()->set_tool(RadiantTool::WindowLevel);
    });

    QMenu * wlMenu = new QMenu(toolbar);
    auto addWlPreset = [this, wlMenu](const QString & name, int preset) {
        QAction * a = wlMenu->addAction(name);
        connect(a, &QAction::triggered, [this, preset]() { on_mpr_wl_preset(preset); });
    };
    addWlPreset(QString("0: Reset (Original DICOM)"), 0);
    addWlPreset(QString("2: TC Abdome (WL: 40, WW: 350)"), 2);
    addWlPreset(QString("3: TC Osso (WL: 300, WW: 1500)"), 3);
    addWlPreset(QString("4: TC Cérebro (WL: 40, WW: 80)"), 4);
    addWlPreset(QString("5: TC Pulmão (WL: -600, WW: 1500)"), 5);
    create_menu_button(act_mpr_wl, wlMenu, false);

    // 2. Pan Tool
    act_mpr_pan = new QAction(QIcon(":/bitmaps/radiant_pan.svg"), QString("Mover / Pan (M)"), this);
    act_mpr_pan->setCheckable(true);
    tool_action_group->addAction(act_mpr_pan);
    connect(act_mpr_pan, SIGNAL(triggered()), this, SLOT(on_mpr_pan_triggered()));
    toolbar->addAction(act_mpr_pan);

    // 3. Zoom Tool with Dropdown
    act_mpr_zoom = new QAction(QIcon(":/bitmaps/radiant_zoom.svg"), QString("Zoom Interativo (Z)"), this);
    act_mpr_zoom->setCheckable(true);
    tool_action_group->addAction(act_mpr_zoom);
    connect(act_mpr_zoom, SIGNAL(triggered()), this, SLOT(on_mpr_zoom_triggered()));

    QMenu * zoomMenu = new QMenu(toolbar);
    QAction * actZoomFit = zoomMenu->addAction(QString("Ajustar à Janela (Ctrl+0)"));
    connect(actZoomFit, SIGNAL(triggered()), this, SLOT(on_mpr_zoom_fit()));
    auto addZoomFactor = [this, zoomMenu](const QString & label, double factor) {
        QAction * a = zoomMenu->addAction(label);
        connect(a, &QAction::triggered, [this, factor]() { on_mpr_zoom_factor(factor); });
    };
    addZoomFactor(QString("Zoom 100% (Ctrl+1)"), 1.0);
    addZoomFactor(QString("Zoom 200% (Ctrl+2)"), 2.0);
    addZoomFactor(QString("Zoom 400% (Ctrl+3)"), 4.0);
    create_menu_button(act_mpr_zoom, zoomMenu, false);

    toolbar->addSeparator();

    // 4. Reference Lines Toggle & Rotation Mode
    act_mpr_ref_lines = new QAction(QIcon(":/bitmaps/cross.svg"), QString("Linhas de Referência e Rotação"), this);
    act_mpr_ref_lines->setCheckable(true);
    act_mpr_ref_lines->setChecked(true);
    connect(act_mpr_ref_lines, SIGNAL(toggled(bool)), this, SLOT(on_mpr_ref_lines_toggled(bool)));

    QMenu * refMenu = new QMenu(toolbar);
    QAction * actToggleLines = refMenu->addAction(QString("Mostrar/Ocultar Linhas de Referência"));
    actToggleLines->setCheckable(true);
    actToggleLines->setChecked(true);
    connect(actToggleLines, &QAction::toggled, act_mpr_ref_lines, &QAction::setChecked);
    connect(act_mpr_ref_lines, &QAction::toggled, actToggleLines, &QAction::setChecked);
    // Cosmetic pens keep the requested width stable when the image is zoomed.
    QSettings refSettings(QSettings::IniFormat, QSettings::UserScope,
                          QApplication::organizationName(), QApplication::applicationName());
    const int savedWidth = qBound(1, refSettings.value(QString("MPR/referenceLineWidth"), 1).toInt(), 4);
    auto applyReferenceWidth = [this](int width) {
        if (!content) return;
        for (GraphicsWidget * gw : content->findChildren<GraphicsWidget*>()) {
            if (!gw || !gw->graphicsview) continue;
            for (QGraphicsPathItem * line : {gw->graphicsview->line_x, gw->graphicsview->line_y, gw->graphicsview->line_z}) {
                if (!line) continue;
                QPen pen = line->pen();
                pen.setWidth(width);
                pen.setCosmetic(true);
                line->setPen(pen);
            }
        }
    };
    QMenu * widthMenu = refMenu->addMenu(QString("Espessura das linhas"));
    QActionGroup * widthGroup = new QActionGroup(widthMenu);
    widthGroup->setExclusive(true);
    for (int width = 1; width <= 4; ++width) {
        QAction * choice = widthMenu->addAction(QString("%1 px").arg(width));
        choice->setCheckable(true);
        choice->setChecked(width == savedWidth);
        widthGroup->addAction(choice);
        connect(choice, &QAction::triggered, this, [applyReferenceWidth, width]() {
            QSettings settings(QSettings::IniFormat, QSettings::UserScope,
                               QApplication::organizationName(), QApplication::applicationName());
            settings.setValue(QString("MPR/referenceLineWidth"), width);
            applyReferenceWidth(width);
        });
    }
    applyReferenceWidth(savedWidth);
    QAction * actRotMode = refMenu->addAction(QString("Rotação oblíqua - indisponível"));
    actRotMode->setEnabled(false);
    actRotMode->setToolTip(QString("Aguardando correção e validação da geometria física"));
    refMenu->addSeparator();
    QAction * actLockLines = refMenu->addAction(QString("Fixar Orientação Ortogonal Padrão"));
    connect(actLockLines, SIGNAL(triggered()), this, SLOT(on_mpr_reset_planes()));
    create_menu_button(act_mpr_ref_lines, refMenu, false);

    // 5. Slab MIP Thickness
    act_mpr_slab_mip = new QAction(QIcon(":/bitmaps/ray.svg"), QString("Modo Slab / MIP e Espessura"), this);
    act_mpr_slab_mip->setToolTip(QString("Projecoes MIP, MinIP e Average ainda nao implementadas"));
    QMenu * slabMenu = new QMenu(toolbar);
    auto addSlabMode = [this, slabMenu](const QString & label, int mode, double thickness) {
        QAction * a = slabMenu->addAction(mode == 0 ? label : label + QString(" - indisponivel"));
        if (mode == 0)
            connect(a, &QAction::triggered, [this, mode, thickness]() { on_mpr_slab_mip_thickness(mode, thickness); });
        else
        {
            a->setEnabled(false);
            a->setToolTip(QString("Projecao ainda nao implementada"));
        }
    };
    addSlabMode(QString("Fatia Fina / Thin (1 voxel)"), 0, 0.0);
    slabMenu->addSeparator();
    addSlabMode(QString("Slab MIP (5 mm)"), 1, 5.0);
    addSlabMode(QString("Slab MIP (10 mm)"), 1, 10.0);
    addSlabMode(QString("Slab MIP (20 mm)"), 1, 20.0);
    slabMenu->addSeparator();
    addSlabMode(QString("Slab MinIP (Mínima Intensidade)"), 2, 10.0);
    addSlabMode(QString("Slab Média / Average"), 3, 10.0);
    create_menu_button(act_mpr_slab_mip, slabMenu, true);

    // 6. Reset Planes (Reconstituição Ortogonal)
    act_mpr_reset_planes = new QAction(QIcon(":/bitmaps/reload.svg"), QString("Restaurar Planos Ortogonais"), this);
    connect(act_mpr_reset_planes, SIGNAL(triggered()), this, SLOT(on_mpr_reset_planes()));
    toolbar->addAction(act_mpr_reset_planes);

    toolbar->addSeparator();

    // 7. Layout Switcher
    act_mpr_layout = new QAction(QIcon(":/bitmaps/radiant_grid.svg"), QString("Layout MPR"), this);
    QMenu * layoutMenu = new QMenu(toolbar);
    auto addLayoutChoice = [this, layoutMenu](const QString & label, int idx) {
        QAction * a = layoutMenu->addAction(label);
        connect(a, &QAction::triggered, [this, idx]() { on_mpr_layout_switched(idx); });
    };
    addLayoutChoice(QString("Grade 2x2 (Axial, Coronal, Sagital, 3D)"), 0);
    addLayoutChoice(QString("Linha 1x3 (Horizontal)"), 1);
    addLayoutChoice(QString("Coluna 3x1 (Vertical)"), 2);
    layoutMenu->addSeparator();
    addLayoutChoice(QString("Maximizar Plano Axial"), 3);
    addLayoutChoice(QString("Maximizar Plano Coronal"), 4);
    addLayoutChoice(QString("Maximizar Plano Sagital"), 5);
    create_menu_button(act_mpr_layout, layoutMenu, true);

    // 8. Export MPR Slices
    act_mpr_export = new QAction(QIcon(":/bitmaps/radiant_export.svg"), QString("Exportar Fatias MPR (Ctrl+E)"), this);
    connect(act_mpr_export, SIGNAL(triggered()), this, SLOT(on_mpr_export()));
    toolbar->addAction(act_mpr_export);
}

void RadiantOverlayWindow::setup_3d_toolbar()
{
    // Use the native rendering actions so the renderer and its UI stay in sync.
    act_3d_preset = new QAction(QIcon(":/bitmaps/radiant_3d.svg"), QString("Modos de visualização 3D"), this);
    QMenu * modeMenu = new QMenu(toolbar);
    modeMenu->addAction(mw->slicesAct);
    modeMenu->addAction(mw->raycastAct);
    modeMenu->addAction(mw->mipAct);
    create_menu_button(act_3d_preset, modeMenu, true);

    // The native 3D viewport already supports these gestures directly. These
    // disabled indicators explain the gesture without claiming to select a tool.
    act_3d_trackball = new QAction(QIcon(":/bitmaps/radiant_transform.svg"), QString("Rotação: arraste com o botão esquerdo"), this);
    act_3d_trackball->setToolTip(QString("Rotação: arraste com o botão esquerdo na imagem 3D"));
    act_3d_trackball->setEnabled(false);
    toolbar->addAction(act_3d_trackball);
    act_3d_pan = new QAction(QIcon(":/bitmaps/radiant_pan.svg"), QString("Mover: Shift + arrastar ou botão central"), this);
    act_3d_pan->setToolTip(QString("Mover: Shift + arrastar ou arraste com o botão central"));
    act_3d_pan->setEnabled(false);
    toolbar->addAction(act_3d_pan);

    // Zoom buttons are immediate operations, not a drag-tool selector.
    act_3d_zoom = new QAction(QIcon(":/bitmaps/radiant_zoom.svg"), QString("Aproximar 3D"), this);
    connect(act_3d_zoom, SIGNAL(triggered()), this, SLOT(on_3d_zoom_triggered()));
    QMenu * zoomMenu = new QMenu(toolbar);
    QAction * zoomOut = zoomMenu->addAction(QString("Afastar 3D"));
    connect(zoomOut, &QAction::triggered, [this]() {
        GLWidget * glw = content ? content->findChild<GLWidget*>() : nullptr;
        if (glw) glw->zoom_out(true);
    });
    create_menu_button(act_3d_zoom, zoomMenu, false);

    toolbar->addSeparator();
    act_3d_scalpel = new QAction(QIcon(":/bitmaps/cut.svg"), QString("Bisturi 3D (indisponível)"), this);
    act_3d_scalpel->setToolTip(QString("O corte interativo 3D ainda não está implementado"));
    act_3d_scalpel->setEnabled(false);
    toolbar->addAction(act_3d_scalpel);

    act_3d_views = new QAction(QIcon(":/bitmaps/align.svg"), QString("Vistas anatômicas (indisponíveis)"), this);
    act_3d_views->setToolTip(QString("Vistas anatômicas padrão ainda não estão implementadas"));
    act_3d_views->setEnabled(false);
    toolbar->addAction(act_3d_views);

    // 7. Reset Orientation
    act_3d_reset_orientation = new QAction(QIcon(":/bitmaps/reload.svg"), QString("Restaurar visualização 3D"), this);
    act_3d_reset_orientation->setToolTip(QString("Restaurar câmera, zoom, brilho, transparência, cubo e contornos"));
    connect(act_3d_reset_orientation, SIGNAL(triggered()), this, SLOT(on_3d_reset_orientation()));
    toolbar->addAction(act_3d_reset_orientation);

    toolbar->addSeparator();

    // 8. Snapshot / Export 3D
    act_3d_snapshot = new QAction(QIcon(":/bitmaps/radiant_export.svg"), QString("Capturar Imagem 3D / Exportar"), this);
    connect(act_3d_snapshot, SIGNAL(triggered()), this, SLOT(on_3d_snapshot_export()));
    toolbar->addAction(act_3d_snapshot);
}

void RadiantOverlayWindow::set_content(QWidget * contentWidget)
{
    if (content && content != contentWidget)
    {
        main_layout->removeWidget(content);
    }
    content = contentWidget;
    if (content)
    {
        main_layout->addWidget(content);
        content->show();
    }
}

QWidget * RadiantOverlayWindow::take_content()
{
    QWidget * ret = content;
    if (content)
    {
        main_layout->removeWidget(content);
        content = nullptr;
    }
    return ret;
}

void RadiantOverlayWindow::closeEvent(QCloseEvent * event)
{
    emit window_closed();
    QWidget::closeEvent(event);
}

// =========================================================================
// MPR SLOTS
// =========================================================================

void RadiantOverlayWindow::on_mpr_wl_preset(int preset)
{
    if (mw && mw->get_radiant_shortcuts_manager())
    {
        switch (preset)
        {
        case 0: mw->get_radiant_shortcuts_manager()->apply_preset_0(); break;
        case 2: mw->get_radiant_shortcuts_manager()->apply_preset_2(); break;
        case 3: mw->get_radiant_shortcuts_manager()->apply_preset_3(); break;
        case 4: mw->get_radiant_shortcuts_manager()->apply_preset_4(); break;
        case 5: mw->get_radiant_shortcuts_manager()->apply_preset_5(); break;
        default: break;
        }
    }
    if (content)
    {
        const auto gwList = content->findChildren<GraphicsWidget*>();
        for (GraphicsWidget * gw : gwList)
        {
            if (gw) gw->update_image(0, true);
        }
    }
}

void RadiantOverlayWindow::on_mpr_pan_triggered()
{
    RadiantInputDispatcher::instance()->set_tool(RadiantTool::Pan);
}

void RadiantOverlayWindow::on_mpr_zoom_triggered()
{
    RadiantInputDispatcher::instance()->set_tool(RadiantTool::Zoom);
}

void RadiantOverlayWindow::on_mpr_zoom_fit()
{
    if (!content) return;
    const auto gwList = content->findChildren<GraphicsWidget*>();
    for (GraphicsWidget * gw : gwList)
    {
        if (gw && gw->graphicsview)
            gw->graphicsview->reset_zoom();
    }
}

void RadiantOverlayWindow::on_mpr_zoom_factor(double factor)
{
    if (!content) return;
    const auto gwList = content->findChildren<GraphicsWidget*>();
    for (GraphicsWidget * gw : gwList)
    {
        if (gw && gw->graphicsview)
            gw->graphicsview->set_zoom_factor(factor);
    }
}

void RadiantOverlayWindow::on_mpr_ref_lines_toggled(bool checked)
{
    if (content)
    {
        const auto gwList = content->findChildren<GraphicsWidget*>();
        for (GraphicsWidget * gw : gwList)
        {
            if (gw) gw->set_reference_lines_visible(checked);
        }
    }
}

void RadiantOverlayWindow::on_mpr_slab_mip_thickness(int mode, double thickness)
{
    Q_UNUSED(mode);
    Q_UNUSED(thickness);
    if (!content) return;
    const auto gwList = content->findChildren<GraphicsWidget*>();
    for (GraphicsWidget * gw : gwList)
    {
        if (gw) gw->update_image(0, false);
    }
}

void RadiantOverlayWindow::on_mpr_reset_planes()
{
    if (!content) return;
    const auto gwList = content->findChildren<GraphicsWidget*>();
    for (GraphicsWidget * gw : gwList)
    {
        if (gw && gw->mpr_state() && gw->mpr_state()->enabled)
        {
            gw->mpr_state()->reset_orientation();
            break;
        }
    }
    for (GraphicsWidget * gw : gwList)
    {
        if (gw)
        {
            if (gw->graphicsview) gw->graphicsview->reset_reference_line_angles();
            gw->reset_oblique_slice();
            gw->update_frames();
        }
    }
    for (GraphicsWidget * gw : gwList)
    {
        if (gw && gw->graphicsview && gw->mpr_state() && gw->mpr_state()->enabled)
        {
            gw->graphicsview->sync_mpr_sliders();
            break;
        }
    }
}

void RadiantOverlayWindow::on_mpr_layout_switched(int layoutIndex)
{
    if (!mw) return;
    switch (layoutIndex)
    {
    case 0:
    case 1:
    case 2:
        if (mw->zyxAct) mw->zyxAct->trigger();
        break;
    case 3:
        if (mw->graphicsAct_Z) mw->graphicsAct_Z->trigger();
        break;
    case 4:
        if (mw->graphicsAct_Y) mw->graphicsAct_Y->trigger();
        break;
    case 5:
        if (mw->graphicsAct_X) mw->graphicsAct_X->trigger();
        break;
    default:
        break;
    }
}

void RadiantOverlayWindow::on_mpr_export()
{
    if (mw && mw->get_radiant_shortcuts_manager())
    {
        mw->get_radiant_shortcuts_manager()->trigger_export();
    }
}

// =========================================================================
// 3D VR SLOTS
// =========================================================================

void RadiantOverlayWindow::on_3d_preset_triggered(int preset)
{
    GLWidget * glw = content ? content->findChild<GLWidget*>() : nullptr;
    if (!glw) return;

    switch (preset)
    {
    case 0: // Bone
    case 2: // Muscle
    case 3: // Skin
        glw->set_view_3d();
        break;
    case 1: // Angio
        glw->set_view_rc();
        break;
    case 4: // MIP
        glw->set_view_mip();
        break;
    default:
        break;
    }
    glw->updateGL();
}

void RadiantOverlayWindow::on_3d_trackball_triggered()
{
    GLWidget * glw = content ? content->findChild<GLWidget*>() : nullptr;
    if (glw)
    {
        glw->setCursor(Qt::ArrowCursor);
    }
}

void RadiantOverlayWindow::on_3d_pan_triggered()
{
    GLWidget * glw = content ? content->findChild<GLWidget*>() : nullptr;
    if (glw)
    {
        glw->setCursor(Qt::OpenHandCursor);
    }
}

void RadiantOverlayWindow::on_3d_zoom_triggered()
{
    GLWidget * glw = content ? content->findChild<GLWidget*>() : nullptr;
    if (glw)
    {
        glw->zoom_in(true);
        glw->updateGL();
    }
}

void RadiantOverlayWindow::on_3d_scalpel_toggled(bool checked)
{
    GLWidget * glw = content ? content->findChild<GLWidget*>() : nullptr;
    if (glw)
    {
        glw->draw_frames_3d = checked;
        glw->updateGL();
    }
}

void RadiantOverlayWindow::on_3d_anatomical_view(int viewIndex)
{
    GLWidget * glw = content ? content->findChild<GLWidget*>() : nullptr;
    if (!glw) return;

    switch (viewIndex)
    {
    case 0: // Anterior-Posterior (AP)
        glw->set_forward();
        break;
    case 1: // Posterior-Anterior (PA)
        glw->set_backward();
        break;
    case 2: // Lateral Left
        glw->set_left();
        break;
    case 3: // Lateral Right
        glw->set_right();
        break;
    case 4: // Superior (Axial)
        glw->set_forward();
        glw->x_rotation = 90 * 16;
        break;
    case 5: // Inferior
        glw->set_forward();
        glw->x_rotation = -90 * 16;
        break;
    default:
        break;
    }
    glw->updateGL();
}

void RadiantOverlayWindow::on_3d_reset_orientation()
{
    if (mw)
    {
        // Use the native reset, including its camera and fit-to-volume state.
        QMetaObject::invokeMethod(mw, "reset_3d", Qt::DirectConnection);
        GLWidget * glw = content ? content->findChild<GLWidget*>() : nullptr;
        if (glw) glw->updateGL();
    }
}

void RadiantOverlayWindow::on_3d_snapshot_export()
{
    GLWidget * glw = content ? content->findChild<GLWidget*>() : nullptr;
    if (glw)
    {
        glw->get_screen(true);
    }
}
