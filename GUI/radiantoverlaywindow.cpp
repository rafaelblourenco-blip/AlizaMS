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
#include "radianttoolbarscale.h"
#include "radiantexportdialog.h"
#include <QVBoxLayout>
#include <QMenu>
#include <QToolButton>
#include <QIcon>
#include <QSize>
#include <QSettings>
#include <QApplication>
#include <QActionGroup>
#include <QGridLayout>
#include <QTimer>
#include <QEvent>
#include <QMouseEvent>
#include <QMessageBox>
#include <QWidgetAction>
#include <QDoubleSpinBox>
#include <QHBoxLayout>
#include <QDesktopServices>
#include <QUrl>
#include <QFileDialog>
#include <QPixmap>

RadiantOverlayWindow::RadiantOverlayWindow(
    const QString & title,
    QWidget * contentWidget,
    MainWindow * parentWindow,
    OverlayType type)
    : QWidget(parentWindow, Qt::Window | Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint),
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
        if (overlay_type == OverlayType::MPR)
            for (GraphicsWidget * gw : content->findChildren<GraphicsWidget*>())
                if (gw && gw->graphicsview) {
                    gw->graphicsview->viewport()->installEventFilter(this);
                    gw->set_mouse_modus(1, false);
                    gw->set_reference_lines_visible(true);
                }
    }
}

bool RadiantOverlayWindow::eventFilter(QObject * watched, QEvent * event)
{
    if (overlay_type == OverlayType::MPR && mpr_rotate_volume && content)
    {
        for (GraphicsWidget *gw : content->findChildren<GraphicsWidget*>()) {
            if (!gw || !gw->graphicsview || gw->graphicsview->viewport() != watched) continue;
            const auto state = gw->mpr_state();
            if (!state || !state->enabled || !state->initialized) break;
            if (event->type() == QEvent::MouseButtonPress) {
                QMouseEvent *mouse = static_cast<QMouseEvent *>(event);
                if (mouse->button() != Qt::LeftButton) break;
                mpr_rotation_dragging = true;
                mpr_rotation_last = mouse->pos();
                return true;
            }
            if (event->type() == QEvent::MouseButtonRelease && mpr_rotation_dragging) {
                mpr_rotation_dragging = false;
                return true;
            }
            if (event->type() == QEvent::MouseMove && mpr_rotation_dragging) {
                QMouseEvent *mouse = static_cast<QMouseEvent *>(event);
                const QPoint delta = mouse->pos() - mpr_rotation_last;
                mpr_rotation_last = mouse->pos();
                const int viewAxis = gw->get_axis();
                if (viewAxis < 0 || viewAxis >= 3) return true;
                const auto axisU = state->planes.planes[viewAxis].u;
                const auto axisV = state->planes.planes[viewAxis].v;
                for (auto &plane : state->planes.planes) {
                    plane.u = mpr_geometry::rotate(mpr_geometry::rotate(plane.u, axisV, delta.x() * 0.005), axisU, delta.y() * 0.005);
                    plane.v = mpr_geometry::rotate(mpr_geometry::rotate(plane.v, axisV, delta.x() * 0.005), axisU, delta.y() * 0.005);
                }
                gw->graphicsview->sync_mpr_sliders();
                gw->graphicsview->refresh_mpr_views();
                return true;
            }
            break;
        }
    }
    const RadiantTool activeTool = RadiantInputDispatcher::instance()->get_tool();
    const bool drawingMeasure = activeTool == RadiantTool::Length || activeTool == RadiantTool::Angle ||
        activeTool == RadiantTool::Ellipse || activeTool == RadiantTool::ClosedPolygon || activeTool == RadiantTool::Arrow;
    if (overlay_type == OverlayType::MPR && event->type() == QEvent::MouseButtonDblClick && content && !drawingMeasure)
    {
        for (GraphicsWidget * gw : content->findChildren<GraphicsWidget*>())
        {
            if (!gw || !gw->graphicsview || gw->graphicsview->viewport() != watched) continue;
            QWidget * frame = gw;
            while (frame && frame->parentWidget() != content) frame = frame->parentWidget();
            if (!frame) break;
            const char * const names[] = {"graphicswidget_frameZ", "graphicswidget_frameY", "graphicswidget_frameX"};
            int plane = -1;
            for (int i = 0; i < 3; ++i) if (frame == content->findChild<QWidget *>(names[i])) plane = i;
            if (plane < 0) break;
            mpr_layout_state.double_click(plane);
            apply_mpr_layout();
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

RadiantOverlayWindow::~RadiantOverlayWindow()
{
}

void RadiantOverlayWindow::setup_toolbar()
{
    toolbar = new QToolBar(QString("RadiantOverlayToolBar"), this);
    toolbar->setObjectName(QString("RadiantOverlayToolBar"));
    toolbar->setIconSize(QSize(22, 22));
    new RadiantToolbarScale(this, toolbar);
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
    auto addTool = [this](const QIcon &icon, const QString &label, RadiantTool tool, int mode) {
        QAction *action = toolbar->addAction(icon, label);
        action->setCheckable(true);
        tool_action_group->addAction(action);
        connect(action, &QAction::triggered, this, [this, tool, mode]() {
            RadiantInputDispatcher::instance()->set_tool(tool);
            set_mpr_tool(mode);
        });
    };

    act_mpr_reset_planes = new QAction(QIcon(":/bitmaps/reload.svg"), QString::fromUtf8("Restaurar imagens MPR"), this);
    connect(act_mpr_reset_planes, &QAction::triggered, this, [this]() {
        if (content) {
            for (GraphicsWidget *gw : content->findChildren<GraphicsWidget*>()) {
                if (!gw || !gw->mpr_state() || !gw->mpr_state()->enabled || !gw->image_container.image3D || !gw->image_container.image3D->di) continue;
                const auto state = gw->mpr_state();
                state->reset_position();
                auto *di = gw->image_container.image3D->di;
                di->selected_x_slice = state->initialSliderStep[0];
                di->selected_y_slice = state->initialSliderStep[1];
                di->selected_z_slice = state->initialSliderStep[2];
                break;
            }
        }
        on_mpr_reset_planes();
        on_mpr_wl_preset(0);
        on_mpr_zoom_fit();
    });
    toolbar->addAction(act_mpr_reset_planes);

    QAction *saveImage = toolbar->addAction(QIcon(":/bitmaps/radiant_export.svg"), QString::fromUtf8("Salvar imagem MPR"));
    connect(saveImage, &QAction::triggered, this, [this]() {
        if (!content) return;
        const QString path = QFileDialog::getSaveFileName(this, QString::fromUtf8("Salvar imagem MPR"), QString(), QString::fromUtf8("Imagem PNG (*.png);;Imagem JPEG (*.jpg *.jpeg)"));
        if (!path.isEmpty() && !content->grab().save(path))
            QMessageBox::warning(this, QString::fromUtf8("Salvar imagem"), QString::fromUtf8("Não foi possível salvar a imagem."));
    });

    // 7. Layout Switcher
    act_mpr_layout = new QAction(QIcon(":/bitmaps/radiant_grid.svg"), QString("Layout MPR"), this);
    QMenu * layoutMenu = new QMenu(toolbar);
    auto addLayoutChoice = [this, layoutMenu](const QString & label, int idx) {
        QAction * a = layoutMenu->addAction(label);
        connect(a, &QAction::triggered, [this, idx]() { on_mpr_layout_switched(idx); });
    };
    addLayoutChoice(QString("Grade 2x2 (Axial, Coronal, Sagital)"), 0);
    addLayoutChoice(QString("Linha 1x3 (Horizontal)"), 1);
    addLayoutChoice(QString("Coluna 3x1 (Vertical)"), 2);
    layoutMenu->addSeparator();
    addLayoutChoice(QString("Maximizar Plano Axial"), 3);
    addLayoutChoice(QString("Maximizar Plano Coronal"), 4);
    addLayoutChoice(QString("Maximizar Plano Sagital"), 5);
    create_menu_button(act_mpr_layout, layoutMenu, true);

    // 4. Reference Lines Toggle & Rotation Mode
    act_mpr_ref_lines = new QAction(QIcon(":/bitmaps/radiant_reference_lines.svg"), QString("Linhas de Referência e Rotação"), this);
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
    QAction * actRotMode = refMenu->addAction(QString::fromUtf8("Rotação oblíqua"));
    actRotMode->setToolTip(QString::fromUtf8("Arraste os círculos nas linhas de referência para girar os planos"));
    connect(actRotMode, &QAction::triggered, this, [this]() { activate_oblique_interaction(); });
    refMenu->addSeparator();
    QAction * actLockLines = refMenu->addAction(QString("Fixar Orientação Ortogonal Padrão"));
    connect(actLockLines, SIGNAL(triggered()), this, SLOT(on_mpr_reset_planes()));
    QAction *lock = refMenu->addAction(QString::fromUtf8("Fixar 90°"));
    lock->setIcon(QIcon(":/bitmaps/radiant_lock_90.svg"));
    lock->setStatusTip(QString::fromUtf8("Travar linhas a 90 graus"));
    lock->setCheckable(true);
    lock->setToolTip(QString::fromUtf8("Ao travar, restaura os planos ortogonais e mantém 90 graus durante a rotação"));
    connect(lock, &QAction::toggled, this, [this, lock](bool checked) {
        if (!content) return;
        if (checked) on_mpr_reset_planes();
        for (GraphicsWidget *gw : content->findChildren<GraphicsWidget*>())
            if (gw && gw->mpr_state()) gw->mpr_state()->lock_orthogonal = checked;
    });
    create_menu_button(act_mpr_ref_lines, refMenu, false);
    QAction *volumeRotation = toolbar->addAction(QIcon(":/bitmaps/radiant_rotate_volume.svg"), QString::fromUtf8("Girar volume 3D no MPR (arraste)"));
    volumeRotation->setCheckable(true);
    tool_action_group->addAction(volumeRotation);
    connect(volumeRotation, &QAction::triggered, this, [this]() {
        RadiantInputDispatcher::instance()->set_tool(RadiantTool::Browse);
        set_mpr_tool(0);
        if (content) for (GraphicsWidget *gw : content->findChildren<GraphicsWidget*>())
            if (gw && gw->graphicsview) gw->graphicsview->viewport()->installEventFilter(this);
        mpr_rotate_volume = true;
    });
    QAction *rotateTool = toolbar->addAction(QIcon(":/bitmaps/mpr_oblique.svg"), QString::fromUtf8("Girar planos pelas linhas de referência"));
    rotateTool->setCheckable(true); tool_action_group->addAction(rotateTool);
    connect(rotateTool, &QAction::triggered, this, [this]() { activate_oblique_interaction(); });
    rotateTool->setChecked(true);
    toolbar->addAction(lock);
    addTool(QIcon(":/bitmaps/radiant_mpr.svg"), QString::fromUtf8("Rolar cortes (arraste vertical)"), RadiantTool::Browse, 0);
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


    // 1. WL Presets with Dropdown
    act_mpr_wl = new QAction(QIcon(":/bitmaps/radiant_wl.svg"), QString("Janelamento WL/WW (W)"), this);
    act_mpr_wl->setCheckable(true);
    tool_action_group->addAction(act_mpr_wl);
    connect(act_mpr_wl, &QAction::triggered, [this]() {
        RadiantInputDispatcher::instance()->set_tool(RadiantTool::WindowLevel);
        set_mpr_tool(0);
    });

    QMenu * wlMenu = new QMenu(toolbar);
    auto addWlPreset = [this, wlMenu](const QString & name, int preset) {
        QAction * a = wlMenu->addAction(name);
        connect(a, &QAction::triggered, [this, preset]() { on_mpr_wl_preset(preset); });
    };
    addWlPreset(QString("0: Reset (Original DICOM)"), 0);
    addWlPreset(QString::fromUtf8("2: TC Abdome (WL: 60, WW: 400)"), 2);
    addWlPreset(QString::fromUtf8("4: TC Osso (WL: 300, WW: 1500)"), 4);
    addWlPreset(QString::fromUtf8("5: TC Cérebro (WL: 40, WW: 80)"), 5);
    addWlPreset(QString::fromUtf8("7: TC Pulmão (WL: -400, WW: 1500)"), 7);
    create_menu_button(act_mpr_wl, wlMenu, false);

    // 5. Slab MIP Thickness
    act_mpr_slab_mip = new QAction(QString("MIP / Média"), this);
    act_mpr_slab_mip->setToolTip(QString("Modo de projeção e espessura do corte"));
    QMenu * slabMenu = new QMenu(toolbar);
    auto * thicknessAction = new QWidgetAction(slabMenu);
    auto * thicknessRow = new QWidget(slabMenu);
    auto * thicknessLayout = new QHBoxLayout(thicknessRow);
    thicknessLayout->setContentsMargins(8, 4, 8, 4);
    thicknessLayout->addWidget(new QLabel(QString::fromUtf8("Espessura:"), thicknessRow));
    auto * thicknessInput = new QDoubleSpinBox(thicknessRow);
    thicknessInput->setRange(0.1, 1000.0);
    thicknessInput->setDecimals(1);
    thicknessInput->setSingleStep(0.5);
    thicknessInput->setSuffix(" mm");
    thicknessInput->setValue(slab_menu_thickness);
    thicknessInput->setKeyboardTracking(false);
    thicknessLayout->addWidget(thicknessInput);
    thicknessAction->setDefaultWidget(thicknessRow);
    slabMenu->addAction(thicknessAction);
    auto * modes = new QActionGroup(slabMenu);
    modes->setExclusive(true);
    auto addSlabMode = [this, slabMenu, modes](const QString & label, int mode) {
        QAction * action = slabMenu->addAction(label);
        action->setCheckable(true);
        action->setChecked(mode == 0);
        modes->addAction(action);
        connect(action, &QAction::triggered, this, [this, mode]() {
            slab_menu_mode = mode;
            on_mpr_slab_mip_thickness(mode, slab_menu_thickness);
        });
    };
    addSlabMode(QString::fromUtf8("Fatia fina"), 0);
    addSlabMode(QString("MIP"), 1);
    addSlabMode(QString("MinIP"), 2);
    addSlabMode(QString::fromUtf8("Média (Average)"), 3);
    connect(thicknessInput, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
            [this](double value) {
        slab_menu_thickness = value;
        if (slab_menu_mode != 0) on_mpr_slab_mip_thickness(slab_menu_mode, value);
    });
    create_menu_button(act_mpr_slab_mip, slabMenu, true);

    QAction *measure = new QAction(QIcon(":/bitmaps/radiant_measure.svg"), QString::fromUtf8("Medidas"), this);
    measure->setCheckable(true);
    tool_action_group->addAction(measure);
    connect(measure, &QAction::triggered, this, [this]() {
        RadiantInputDispatcher::instance()->set_tool(RadiantTool::Length);
        set_mpr_tool(2);
    });
    QMenu *measureMenu = new QMenu(toolbar);
    auto addMeasure = [this, measure, measureMenu](const QString &label, RadiantTool tool) {
        QAction *action = measureMenu->addAction(label);
        connect(action, &QAction::triggered, this, [this, measure, tool]() {
            measure->setChecked(true);
            RadiantInputDispatcher::instance()->set_tool(tool);
            set_mpr_tool(2);
        });
    };
    addMeasure(QString::fromUtf8("Distância"), RadiantTool::Length);
    addMeasure(QString::fromUtf8("Ângulo"), RadiantTool::Angle);
    addMeasure(QString::fromUtf8("Elipse"), RadiantTool::Ellipse);
    addMeasure(QString::fromUtf8("Polígono fechado"), RadiantTool::ClosedPolygon);
    addMeasure(QString::fromUtf8("Seta"), RadiantTool::Arrow);
    create_menu_button(measure, measureMenu, false);
    QAction *settings = new QAction(QIcon(":/bitmaps/radiant_config.svg"), QString::fromUtf8("Configurações"), this);
    QMenu *settingsMenu = new QMenu(toolbar);
#ifdef Q_OS_WIN
    QAction *gpuSettings = settingsMenu->addAction(QString::fromUtf8("Configurações de GPU do Windows"));
    gpuSettings->setToolTip(QString::fromUtf8("Escolha a GPU e reinicie o aplicativo após alterar"));
    connect(gpuSettings, &QAction::triggered, this, []() { QDesktopServices::openUrl(QUrl("ms-settings:display-advancedgraphics")); });
#endif
    create_menu_button(settings, settingsMenu, true);
}

void RadiantOverlayWindow::setup_3d_toolbar()
{
    auto viewport = [this]() -> GLWidget * {
        return content ? content->findChild<GLWidget*>() : nullptr;
    };

    act_3d_reset_orientation = toolbar->addAction(QIcon(":/bitmaps/reload.svg"), QString::fromUtf8("Restaurar visualização 3D"));
    connect(act_3d_reset_orientation, &QAction::triggered, this, &RadiantOverlayWindow::on_3d_reset_orientation);
    act_3d_snapshot = toolbar->addAction(QIcon(":/bitmaps/save2.svg"), QString::fromUtf8("Salvar imagem 3D"));
    connect(act_3d_snapshot, &QAction::triggered, this, &RadiantOverlayWindow::on_3d_snapshot_export);

    QAction *cine = toolbar->addAction(QIcon(":/bitmaps/radiant_cine.svg"), QString::fromUtf8("Rotação automática do volume"));
    act_3d_cine = cine;
    cine->setCheckable(true);
    cine->setToolTip(QString::fromUtf8("Girar automaticamente o volume; não reproduz séries temporais"));
    connect(cine, &QAction::toggled, this, [this, viewport](bool on) {
        if (auto *gl = viewport()) {
            if (on && act_3d_scalpel && act_3d_scalpel->isChecked()) {
                act_3d_trackball->setChecked(true);
                gl->set_3d_interaction(0);
            }
            gl->set_3d_cine(on);
        }
    });
    connect(act_3d_reset_orientation, &QAction::triggered, cine, [cine]() { cine->setChecked(false); });

    QAction *annotations = toolbar->addAction(QIcon(":/bitmaps/radiant_text_overlay.svg"), QString::fromUtf8("Remover anotações 3D"));
    annotations->setEnabled(false);
    annotations->setToolTip(QString::fromUtf8("Anotações vinculadas ao volume ainda não estão disponíveis"));

    auto tool = [this, viewport](const char *icon, const QString &label, int mode) {
        QAction *action = toolbar->addAction(QIcon(QString::fromLatin1(icon)), label);
        action->setCheckable(true);
        tool_action_group->addAction(action);
        connect(action, &QAction::triggered, this, [this, viewport, mode]() {
            if (mode == 5 && mw && mw->raycastAct) mw->raycastAct->setChecked(true);
            if (auto *gl = viewport()) gl->set_3d_interaction(mode);
        });
        return action;
    };
    act_3d_trackball = tool(":/bitmaps/radiant_rotate_volume.svg", QString::fromUtf8("Girar volume"), 0);
    act_3d_trackball->setChecked(true);
    tool(":/bitmaps/radiant_rotate_plane.svg", QString::fromUtf8("Girar no plano"), 1);
    act_3d_pan = tool(":/bitmaps/radiant_pan.svg", QString::fromUtf8("Panorâmica"), 2);
    act_3d_zoom = tool(":/bitmaps/radiant_zoom.svg", QString::fromUtf8("Zoom"), 3);
    tool(":/bitmaps/radiant_wl.svg", QString::fromUtf8("Janelamento 3D: brilho e contraste"), 4);
    act_3d_scalpel = tool(":/bitmaps/radiant_scalpel.svg", QString::fromUtf8("Bisturi 3D"), 5);
    act_3d_scalpel->setToolTip(QString::fromUtf8("Desenhe uma região; Delete exclui dentro e Esc exclui fora"));
    connect(act_3d_scalpel, &QAction::triggered, cine, [cine]() { cine->setChecked(false); });

    QAction *measure = toolbar->addAction(QIcon(":/bitmaps/radiant_measure.svg"), QString::fromUtf8("Medidas 3D"));
    measure->setEnabled(false);
    measure->setToolTip(QString::fromUtf8("Medidas físicas 3D exigem seleção de profundidade, ainda não disponível"));

    act_3d_preset = new QAction(QIcon(":/bitmaps/radiant_grid.svg"), QString::fromUtf8("Presets e modos 3D"), this);
    QMenu *presetMenu = new QMenu(toolbar);
    QMenu *protocols = presetMenu->addMenu(QString::fromUtf8("Protocolos anatômicos"));
    const char *protocolNames[] = {"Angio", "Airways", "Bones and skin 1", "Bones and skin 2", "Bones and skin 3", "Bones B/W", "Skin B/W", "MIP B/W", "MIP B/W Inverse"};
    for (const char *name : protocolNames) {
        QAction *item = protocols->addAction(QString::fromLatin1(name));
        if (QString::fromLatin1(name) == QString("MIP B/W")) {
            connect(item, &QAction::triggered, this, [this, viewport]() {
                if (mw && mw->mipAct) mw->mipAct->setChecked(true);
                if (auto *gl = viewport()) gl->set_3d_lut(0);
            });
        } else {
            item->setEnabled(false);
            item->setToolTip(QString::fromUtf8("Este protocolo de transferência ainda não está implementado"));
        }
    }
    QMenu *modeMenu = presetMenu->addMenu(QString::fromUtf8("Modo de renderização"));
    modeMenu->addAction(mw->slicesAct);
    modeMenu->addAction(mw->raycastAct);
    modeMenu->addAction(mw->mipAct);
    QMenu *lutMenu = presetMenu->addMenu(QString::fromUtf8("Cores (LUT 3D)"));
    QActionGroup *lutGroup = new QActionGroup(lutMenu);
    lutGroup->setExclusive(true);
    const QString names[] = {QString::fromUtf8("Escala de cinza"), QString("Hot metal 768"), QString("RainbowB 1536"), QString("Syngo 1792"), QString("Hot iron"), QString("Hot metal blue"), QString("PET"), QString("PET 20 steps"), QString("Labels 4096")};
    for (int i = 0; i < 9; ++i) {
        QAction *item = lutMenu->addAction(names[i]);
        item->setCheckable(true);
        lutGroup->addAction(item);
        connect(item, &QAction::triggered, this, [viewport, i]() { if (auto *gl = viewport()) gl->set_3d_lut(i); });
        connect(lutMenu, &QMenu::aboutToShow, this, [viewport, item, i]() { if (auto *gl = viewport()) { gl->sync_3d_display(); item->setChecked(gl->get_3d_lut() == i); } });
    }
    create_menu_button(act_3d_preset, presetMenu, true);

    QAction *settings = new QAction(QIcon(":/bitmaps/radiant_config.svg"), QString::fromUtf8("Configurações"), this);
    QMenu *settingsMenu = new QMenu(toolbar);
#ifdef Q_OS_WIN
    QAction *gpu = settingsMenu->addAction(QString::fromUtf8("Selecionar GPU nas configurações do Windows"));
    gpu->setToolTip(QString::fromUtf8("Escolha a GPU para o aplicativo e reinicie-o após alterar"));
    connect(gpu, &QAction::triggered, this, []() { QDesktopServices::openUrl(QUrl("ms-settings:display-advancedgraphics")); });
#else
    QAction *gpu = settingsMenu->addAction(QString::fromUtf8("Seleção de GPU pelo sistema operacional"));
    gpu->setEnabled(false);
#endif
    create_menu_button(settings, settingsMenu, true);
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

void RadiantOverlayWindow::showEvent(QShowEvent * event)
{
    QWidget::showEvent(event);
    // The views are reparented from the main viewer and may retain its zoom
    // and scroll position. Fit after the MPR layout receives its final size.
    if (overlay_type == OverlayType::MPR)
        QTimer::singleShot(0, this, [this]() {
            activate_oblique_interaction();
            on_mpr_zoom_fit();
        });
}

void RadiantOverlayWindow::hideEvent(QHideEvent * event)
{
    if (overlay_type == OverlayType::VR_3D) {
        if (act_3d_cine) act_3d_cine->setChecked(false);
        if (act_3d_trackball) act_3d_trackball->setChecked(true);
        if (content)
            if (GLWidget *gl = content->findChild<GLWidget*>()) {
                gl->set_3d_cine(false);
                gl->set_3d_interaction(0);
            }
    }
    QWidget::hideEvent(event);
}

void RadiantOverlayWindow::closeEvent(QCloseEvent * event)
{
    if (overlay_type == OverlayType::MPR) leave_mpr();
    if (overlay_type == OverlayType::VR_3D && content)
        if (GLWidget *gl = content->findChild<GLWidget*>()) { gl->set_3d_cine(false); gl->set_3d_interaction(0); }
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
        case 7: mw->get_radiant_shortcuts_manager()->apply_preset_7(); break;
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
    set_mpr_tool(0);
}

void RadiantOverlayWindow::on_mpr_zoom_triggered()
{
    RadiantInputDispatcher::instance()->set_tool(RadiantTool::Zoom);
    set_mpr_tool(0);
}

void RadiantOverlayWindow::on_mpr_zoom_fit()
{
    if (!content) return;
    const auto gwList = content->findChildren<GraphicsWidget*>();
    for (GraphicsWidget * gw : gwList)
    {
        if (gw && gw->graphicsview) {
            gw->graphicsview->reset_zoom();
            if (gw->graphicsview->image_item)
                gw->graphicsview->centerOn(gw->graphicsview->image_item);
        }
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

void RadiantOverlayWindow::set_mpr_tool(int mode)
{
    mpr_rotate_volume = false;
    mpr_rotation_dragging = false;
    if (!content) return;
    for (GraphicsWidget *gw : content->findChildren<GraphicsWidget*>()) {
        if (!gw) continue;
        gw->set_mouse_modus(static_cast<short>(mode), false);
        gw->set_reference_lines_visible(act_mpr_ref_lines->isChecked());
    }
}

void RadiantOverlayWindow::leave_mpr()
{
    if (!content) return;
    for (GraphicsWidget *gw : content->findChildren<GraphicsWidget*>())
        if (gw && gw->mpr_state()) { gw->mpr_state()->reset_orientation(); gw->mpr_state()->enabled=false; }
    for (GraphicsWidget *gw : content->findChildren<GraphicsWidget*>()) {
        if (!gw) continue;
        gw->set_reference_lines_visible(false);
        gw->set_mouse_modus(0, false);
        gw->set_slab_projection(0, 0);
        gw->reset_oblique_slice();
    }
    RadiantInputDispatcher::instance()->set_tool(RadiantTool::Browse);
}

void RadiantOverlayWindow::activate_oblique_interaction()
{
    RadiantInputDispatcher::instance()->set_tool(RadiantTool::Browse);
    mpr_rotate_volume = false;
    mpr_rotation_dragging = false;
    if (!content) return;
    act_mpr_ref_lines->setChecked(true);
    GraphicsView * refresh = nullptr;
    for (GraphicsWidget * gw : content->findChildren<GraphicsWidget*>()) {
        if (!gw || !gw->graphicsview) continue;
        const auto & state=gw->mpr_state();
        if (state && state->initialized && state->volume == gw->image_container.image3D) {
            state->enabled=true;
            refresh=gw->graphicsview;
        }
        // The axial view is reparented only when MPR opens, after construction.
        gw->graphicsview->viewport()->installEventFilter(this);
        gw->set_mouse_modus(1, false);
        gw->set_reference_lines_visible(true);
    }
    if (refresh) { refresh->sync_mpr_sliders(); refresh->refresh_mpr_views(); }
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
    if (!content) return;
    bool attempted = false;
    bool failed = false;
    const char * const frameNames[] = {"graphicswidget_frameZ", "graphicswidget_frameY", "graphicswidget_frameX"};
    for (const char * name : frameNames)
    {
        QWidget * frame = content->findChild<QWidget *>(name);
        GraphicsWidget * gw = frame ? frame->findChild<GraphicsWidget *>() : nullptr;
        if (!gw) { failed = true; continue; }
        attempted = true;
        if (!gw->set_slab_projection(mode, thickness)) failed = true;
    }
    if (!attempted || failed)
        QMessageBox::warning(this, QString("Projeção MPR"),
            QString("Não foi possível aplicar a projeção em todos os planos. Selecione uma série volumétrica compatível."));
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
    if (!content || layoutIndex < 0 || layoutIndex > 5) return;
    mpr_layout_state.select(layoutIndex);
    apply_mpr_layout();
}

void RadiantOverlayWindow::apply_mpr_layout()
{
    if (!content) return;
    QGridLayout * grid = qobject_cast<QGridLayout *>(content->layout());
    QWidget * frames[] = {content->findChild<QWidget *>("graphicswidget_frameZ"),
                          content->findChild<QWidget *>("graphicswidget_frameY"),
                          content->findChild<QWidget *>("graphicswidget_frameX")};
    if (!grid || !frames[0] || !frames[1] || !frames[2]) return;
    for (int i = 0; i < 3; ++i) grid->removeWidget(frames[i]);
    for (int i = 0; i < 3; ++i)
    {
        const auto position = mpr_layout_state.position(i);
        grid->addWidget(frames[i], position.row, position.column);
        frames[i]->setVisible(mpr_layout_state.visible(i));
    }
    QTimer::singleShot(0, this, [this]() { on_mpr_zoom_fit(); });
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
        glw->set_sculpt_tool(checked);
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
        if (glw) glw->reset_3d_display();
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

