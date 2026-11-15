#include "radiantshortcutsmanager.h"
#include "radiantinputdispatcher.h"
#include "radiantexportdialog.h"
#include "radiantconfigdialog.h"
#include "radiantfiltermanager.h"
#include "mainwindow.h"
#include "aliza.h"
#include "graphicswidget.h"
#include "graphicsview.h"
#include "studygraphicswidget.h"
#include "studygraphicsview.h"
#include "studyviewwidget.h"
#include "sliderwidget.h"
#include "imagesbox.h"
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

#include <QApplication>
#include <QMenu>
#include <QToolButton>
#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>
#include <QDir>
#include <QTimer>
#include <QStorageInfo>



RadiantShortcutsManager::RadiantShortcutsManager(MainWindow * mainWindow, QObject * parent)
    : QObject(parent ? parent : mainWindow), mw(mainWindow)
{
    connect(RadiantInputDispatcher::instance(), SIGNAL(tool_changed(RadiantTool)),
            this, SLOT(on_dispatcher_tool_changed(RadiantTool)));
    if (mw && mw->imagesbox && mw->imagesbox->listWidget)
        connect(mw->imagesbox->listWidget, &QListWidget::itemSelectionChanged, this, [this]() {
            if (cine_timer && cine_timer->isActive()) toggle_cine();
        });
}

void RadiantShortcutsManager::setup_shortcut(const QKeySequence & seq, const char * slot, bool autoRepeat)
{
    if (!mw) return;
    QShortcut * sc = new QShortcut(seq, mw);
    sc->setAutoRepeat(autoRepeat);
    connect(sc, SIGNAL(activated()), this, slot);
}

void RadiantShortcutsManager::initialize_shortcuts()
{
    if (!mw) return;

    // A. Tools
    setup_shortcut(QKeySequence(Qt::Key_B), SLOT(select_tool_browse()), false);
    setup_shortcut(QKeySequence(Qt::Key_W), SLOT(select_tool_wl()), false);
    setup_shortcut(QKeySequence(Qt::Key_M), SLOT(select_tool_pan()), false);
    setup_shortcut(QKeySequence(Qt::Key_Z), SLOT(select_tool_zoom()), false);
    setup_shortcut(QKeySequence(Qt::Key_L), SLOT(select_tool_length()), false);
    setup_shortcut(QKeySequence(Qt::Key_E), SLOT(select_tool_ellipse()), false);
    setup_shortcut(QKeySequence(Qt::Key_G), SLOT(select_tool_closed_poly()), false);
    setup_shortcut(QKeySequence(Qt::Key_O), SLOT(select_tool_open_poly()), false);
    setup_shortcut(QKeySequence(Qt::Key_A), SLOT(select_tool_angle()), false);
    setup_shortcut(QKeySequence(Qt::Key_C), SLOT(select_tool_cobb()), false);
    setup_shortcut(QKeySequence(Qt::Key_D), SLOT(select_tool_deviation()), false);
    setup_shortcut(QKeySequence(Qt::Key_Y), SLOT(select_tool_arrow()), false);
    setup_shortcut(QKeySequence(Qt::Key_P), SLOT(select_tool_pen()), false);

    // B. Presets (0 to 9)
    setup_shortcut(QKeySequence(Qt::Key_0), SLOT(apply_preset_0()), false);
    setup_shortcut(QKeySequence(Qt::Key_1), SLOT(apply_preset_1()), false);
    setup_shortcut(QKeySequence(Qt::Key_2), SLOT(apply_preset_2()), false);
    setup_shortcut(QKeySequence(Qt::Key_3), SLOT(apply_preset_3()), false);
    setup_shortcut(QKeySequence(Qt::Key_4), SLOT(apply_preset_4()), false);
    setup_shortcut(QKeySequence(Qt::Key_5), SLOT(apply_preset_5()), false);
    setup_shortcut(QKeySequence(Qt::Key_6), SLOT(apply_preset_6()), false);
    setup_shortcut(QKeySequence(Qt::Key_7), SLOT(apply_preset_7()), false);
    setup_shortcut(QKeySequence(Qt::Key_8), SLOT(apply_preset_8()), false);
    setup_shortcut(QKeySequence(Qt::Key_9), SLOT(apply_preset_9()), false);
    setup_shortcut(QKeySequence(Qt::Key_F11), SLOT(toggle_invert_lut()), false);
    setup_shortcut(QKeySequence(Qt::CTRL + Qt::Key_F11), SLOT(show_custom_wl_dialog()), false);

    // C. Navigation, Zoom, Transformations & Cine
    setup_shortcut(QKeySequence(Qt::Key_Up), SLOT(prev_image()), true);
    setup_shortcut(QKeySequence(Qt::Key_Down), SLOT(next_image()), true);
    setup_shortcut(QKeySequence(Qt::Key_PageUp), SLOT(prev_10_images()), true);
    setup_shortcut(QKeySequence(Qt::Key_PageDown), SLOT(next_10_images()), true);
    setup_shortcut(QKeySequence(Qt::Key_Left), SLOT(prev_series()), false);
    setup_shortcut(QKeySequence(Qt::Key_Right), SLOT(next_series()), false);
    setup_shortcut(QKeySequence(Qt::Key_Home), SLOT(first_image_or_series()), false);
    setup_shortcut(QKeySequence(Qt::Key_End), SLOT(last_image_or_series()), false);
    setup_shortcut(QKeySequence(Qt::Key_Tab), SLOT(next_panel()), false);
    setup_shortcut(QKeySequence(Qt::SHIFT + Qt::Key_Tab), SLOT(prev_panel()), false);
    setup_shortcut(QKeySequence(Qt::Key_Space), SLOT(toggle_cine()), false);

    setup_shortcut(QKeySequence(Qt::CTRL + Qt::Key_0), SLOT(zoom_fit()), false);
    setup_shortcut(QKeySequence(Qt::CTRL + Qt::Key_1), SLOT(zoom_100()), false);
    setup_shortcut(QKeySequence(Qt::CTRL + Qt::Key_2), SLOT(zoom_200()), false);
    setup_shortcut(QKeySequence(Qt::CTRL + Qt::Key_3), SLOT(zoom_400()), false);
    setup_shortcut(QKeySequence(Qt::CTRL + Qt::Key_4), SLOT(zoom_800()), false);
    setup_shortcut(QKeySequence(Qt::CTRL + Qt::Key_Plus), SLOT(zoom_in()), true);
    setup_shortcut(QKeySequence(Qt::CTRL + Qt::Key_Minus), SLOT(zoom_out()), true);

    setup_shortcut(QKeySequence(Qt::CTRL + Qt::Key_BracketLeft), SLOT(rotate_90_ccw()), false);
    setup_shortcut(QKeySequence(Qt::CTRL + Qt::Key_BracketRight), SLOT(rotate_90_cw()), false);
    setup_shortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_BracketLeft), SLOT(flip_horizontal()), false);
    setup_shortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_BracketRight), SLOT(flip_vertical()), false);
    setup_shortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Backslash), SLOT(clear_geometric_transforms()), false);

    // Filters
    setup_shortcut(QKeySequence(Qt::Key_I), SLOT(next_sharpen_filter()), false);
    setup_shortcut(QKeySequence(Qt::SHIFT + Qt::Key_I), SLOT(apply_filter_none()), false);
    setup_shortcut(QKeySequence(Qt::CTRL + Qt::Key_I), SLOT(next_filter()), false);
    setup_shortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_I), SLOT(prev_filter()), false);

    // D. Interface, HUD, MPR, 3D
    setup_shortcut(QKeySequence(Qt::Key_F12), SLOT(toggle_hud()), false);
    setup_shortcut(QKeySequence(Qt::SHIFT + Qt::Key_F12), SLOT(toggle_patient_id_hud()), false);
    setup_shortcut(QKeySequence(Qt::Key_F5), SLOT(toggle_series_sync()), false);
    setup_shortcut(QKeySequence(Qt::Key_F3), SLOT(trigger_3d_vr_window()), false);
    setup_shortcut(QKeySequence(Qt::CTRL + Qt::Key_O), SLOT(trigger_open_cd()), false);
    setup_shortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_O), SLOT(trigger_open_dir()), false);
    setup_shortcut(QKeySequence(Qt::CTRL + Qt::Key_E), SLOT(trigger_export()), false);

    // ROIs deletion
    setup_shortcut(QKeySequence(Qt::Key_Delete), SLOT(delete_selected_roi()), false);
    setup_shortcut(QKeySequence(Qt::SHIFT + Qt::Key_Delete), SLOT(delete_all_rois_image()), false);
}

void RadiantShortcutsManager::create_radiant_toolbar(QToolBar * parentToolbar)
{
    if (!mw) return;

    if (!parentToolbar)
    {
        radiant_toolbar = mw->addToolBar(QString("RadiAnt Tools"));
        radiant_toolbar->setObjectName(QString("RadiAntToolBar"));
    }
    else
    {
        radiant_toolbar = parentToolbar;
    }

    radiant_toolbar->setIconSize(QSize(22, 22));
    radiant_toolbar->setMovable(false);

    tool_action_group = new QActionGroup(this);

    // Helper to create a ToolButton with menu
    auto createMenuButton = [&](QAction * act, QMenu * menu, bool instant = false) -> QToolButton* {
        QToolButton * btn = new QToolButton(radiant_toolbar);
        btn->setDefaultAction(act);
        if (menu) {
            btn->setMenu(menu);
            btn->setPopupMode(instant ? QToolButton::InstantPopup : QToolButton::MenuButtonPopup);
        }
        radiant_toolbar->addWidget(btn);
        return btn;
    };

    // 1. Abrir exame da pasta
    action_open_dir = new QAction(QIcon(":/bitmaps/radiant_open_folder.svg"), QString("Abrir pasta DICOM (Ctrl+Shift+O)"), this);
    connect(action_open_dir, SIGNAL(triggered()), this, SLOT(trigger_open_dir()));
    QMenu * studiesMenu = new QMenu(radiant_toolbar);
    studiesMenu->addAction(mw->imagesbox->actionStudy);
    studiesMenu->addAction(mw->imagesbox->actionStudyEmpty);
    studiesMenu->addAction(mw->imagesbox->actionStudyChecked);
    studiesMenu->addSeparator();
    studiesMenu->addAction(mw->imagesbox->actionClear);
    studiesMenu->addAction(mw->imagesbox->actionClearAll);
    createMenuButton(action_open_dir, studiesMenu, false);
    mw->imagesbox->toolbar_frame->hide();

    // 2. Abrir exame do CD / DVD
    action_open_cd = new QAction(QIcon(":/bitmaps/radiant_open_cd.svg"), QString("Abrir exame de CD/DVD"), this);
    connect(action_open_cd, SIGNAL(triggered()), this, SLOT(trigger_open_cd()));
    radiant_toolbar->addAction(action_open_cd);

    // 3. Baixar exame do servidor DICOM
    action_pacs_download = new QAction(QIcon(":/bitmaps/radiant_pacs_download.svg"), QString("Baixar exame do servidor DICOM / PACS"), this);
    connect(action_pacs_download, SIGNAL(triggered()), this, SLOT(trigger_pacs_download()));
    radiant_toolbar->addAction(action_pacs_download);

    // 4. Gravar / Exportar exame aberto
    action_export = new QAction(QIcon(":/bitmaps/radiant_export.svg"), QString("Gravar / Exportar exame (Ctrl+E)"), this);
    connect(action_export, SIGNAL(triggered()), this, SLOT(trigger_export()));
    radiant_toolbar->addAction(action_export);

    radiant_toolbar->addSeparator();

    // 5. Configurar GRID de exibição das imagens
    action_grid = new QAction(QIcon(":/bitmaps/radiant_grid.svg"), QString("Configurar Grid de exibição"), this);
    QMenu * gridMenu = new QMenu(radiant_toolbar);
    auto addGridAction = [&](int r, int c) {
        QAction * a = gridMenu->addAction(QString("%1 x %2").arg(r).arg(c));
        connect(a, &QAction::triggered, [this, r, c]() { this->set_grid_layout(r, c); });
    };
    addGridAction(1, 1);
    addGridAction(2, 1);
    addGridAction(1, 2);
    addGridAction(2, 2);
    addGridAction(3, 2);
    addGridAction(2, 3);
    addGridAction(3, 3);
    addGridAction(2, 4);
    addGridAction(5, 4);
    addGridAction(5, 5);
    if (mw->studyview) {
        QMenu * studyMenu = mw->studyview->create_toolbar_menu(gridMenu);
        studyMenu->setTitle(QString("Ferramentas do estudo"));
        gridMenu->addMenu(studyMenu);
    }
    createMenuButton(action_grid, gridMenu, true);

    // 6. Ícone de Letra ("A")
    action_text_overlay = new QAction(QIcon(":/bitmaps/radiant_text_overlay.svg"), QString("Anotações de Texto / HUD (F12)"), this);
    connect(action_text_overlay, SIGNAL(triggered()), this, SLOT(toggle_hud()));
    QMenu * textMenu = new QMenu(radiant_toolbar);
    QAction * actHudToggle = textMenu->addAction(QString("Mostrar/ocultar todas as anotações (F12)"));
    connect(actHudToggle, SIGNAL(triggered()), this, SLOT(toggle_hud()));
    QAction * actAnonToggle = textMenu->addAction(QString("Modo Anônimo / Ocultar identificação (Shift+F12)"));
    connect(actAnonToggle, SIGNAL(triggered()), this, SLOT(toggle_patient_id_hud()));
    textMenu->addSeparator();
    QAction * actDeidentify = textMenu->addAction(QString("Desidentificar"));
    connect(actDeidentify, SIGNAL(triggered()), this, SLOT(trigger_deidentify()));
    textMenu->addSeparator();
    QAction * actConfigHud = textMenu->addAction(QString("Configurar atributos exibidos..."));
    connect(actConfigHud, SIGNAL(triggered()), this, SLOT(trigger_config_dialog()));
    createMenuButton(action_text_overlay, textMenu, false);

    // 7. Funções dos botões do mouse & ordenação
    action_mouse_config = new QAction(QIcon(":/bitmaps/radiant_mouse.svg"), QString("Funções do Mouse e Ordenação"), this);
    QMenu * mouseMenu = new QMenu(radiant_toolbar);
    mouseMenu->addAction(QString("Botão Esquerdo: Ação da Ferramenta Ativa"))->setEnabled(false);
    mouseMenu->addAction(QString("Botão do Meio (Scroll): Janelamento WL / WW"))->setEnabled(false);
    mouseMenu->addAction(QString("Botão Direito: Zoom Contínuo"))->setEnabled(false);
    mouseMenu->addAction(QString("Roda do Mouse: Navegar Fatias"))->setEnabled(false);
    mouseMenu->addSeparator();
    QMenu * sortMenu = mouseMenu->addMenu(QString("Ordenação das Imagens - indisponível"));
    sortMenu->setEnabled(false);
    createMenuButton(action_mouse_config, mouseMenu, true);

    radiant_toolbar->addSeparator();

    // 8. Botão de Janelamento (WL / WW)
    action_wl = new QAction(QIcon(":/bitmaps/radiant_wl.svg"), QString("Janelamento / Brilho e Contraste (W)"), this);
    action_wl->setCheckable(true);
    tool_action_group->addAction(action_wl);
    connect(action_wl, SIGNAL(triggered()), this, SLOT(select_tool_wl()));

    QMenu * wlMenu = new QMenu(radiant_toolbar);
    wlMenu->addAction(QString("0: Reset (Original DICOM)"), this, SLOT(apply_preset_0()));
    wlMenu->addAction(QString("1: Alcance Dinâmico Total"), this, SLOT(apply_preset_1()));
    wlMenu->addAction(QString("2: Abdome TC (WL: 40, WW: 350)"), this, SLOT(apply_preset_2()));
    wlMenu->addAction(QString("3: Osso TC (WL: 300, WW: 1500)"), this, SLOT(apply_preset_3()));
    wlMenu->addAction(QString("4: Cérebro TC (WL: 40, WW: 80)"), this, SLOT(apply_preset_4()));
    wlMenu->addAction(QString("5: Pulmão TC (WL: -600, WW: 1500)"), this, SLOT(apply_preset_5()));
    wlMenu->addAction(QString("6: Mediastino TC (WL: 50, WW: 350)"), this, SLOT(apply_preset_6()));
    wlMenu->addAction(QString("7: Pélvis TC (WL: 40, WW: 400)"), this, SLOT(apply_preset_7()));
    wlMenu->addAction(QString("8: Angio TC (WL: 300, WW: 600)"), this, SLOT(apply_preset_8()));
    wlMenu->addAction(QString("9: Customizado / PET"), this, SLOT(apply_preset_9()));
    wlMenu->addSeparator();
    wlMenu->addAction(QString("Janelamento Manual... (Ctrl+F11)"), this, SLOT(show_custom_wl_dialog()));
    wlMenu->addAction(QString("Inverter Negativo / LUT (F11)"), this, SLOT(toggle_invert_lut()));
    createMenuButton(action_wl, wlMenu, false);

    // 9. Botão Pan
    action_pan = new QAction(QIcon(":/bitmaps/radiant_pan.svg"), QString("Mover Imagem / Pan (M)"), this);
    action_pan->setCheckable(true);
    tool_action_group->addAction(action_pan);
    connect(action_pan, SIGNAL(triggered()), this, SLOT(select_tool_pan()));
    radiant_toolbar->addAction(action_pan);

    // 10. Botão Zoom
    action_zoom = new QAction(QIcon(":/bitmaps/radiant_zoom.svg"), QString("Zoom Interativo (Z)"), this);
    action_zoom->setCheckable(true);
    tool_action_group->addAction(action_zoom);
    connect(action_zoom, SIGNAL(triggered()), this, SLOT(select_tool_zoom()));

    QMenu * zoomMenu = new QMenu(radiant_toolbar);
    zoomMenu->addAction(QString("Ajustar à Janela (Ctrl+0)"), this, SLOT(zoom_fit()));
    zoomMenu->addAction(QString("Zoom 100% (Ctrl+1)"), this, SLOT(zoom_100()));
    zoomMenu->addAction(QString("Zoom 200% (Ctrl+2)"), this, SLOT(zoom_200()));
    zoomMenu->addAction(QString("Zoom 400% (Ctrl+3)"), this, SLOT(zoom_400()));
    zoomMenu->addAction(QString("Zoom 800% (Ctrl+4)"), this, SLOT(zoom_800()));
    createMenuButton(action_zoom, zoomMenu, false);

    // 11. Botão de Medidas (Length padrão + dropdown de medidas avançadas)
    action_measure = new QAction(QIcon(":/bitmaps/radiant_measure.svg"), QString("Medição Linear / Régua (L)"), this);
    action_measure->setCheckable(true);
    tool_action_group->addAction(action_measure);
    connect(action_measure, SIGNAL(triggered()), this, SLOT(select_tool_length()));

    QMenu * measureMenu = new QMenu(radiant_toolbar);
    measureMenu->addAction(QString("Distância Linear (L)"), this, SLOT(select_tool_length()));
    measureMenu->addAction(QString("Elipse (Área e HU) (E)"), this, SLOT(select_tool_ellipse()));
    measureMenu->addAction(QString("Polígono Fechado (G)"), this, SLOT(select_tool_closed_poly()));
    measureMenu->addAction(QString("Polígono Aberto (O)"), this, SLOT(select_tool_open_poly()));
    measureMenu->addAction(QString("Ângulo Simples (A)"), this, SLOT(select_tool_angle()));
    measureMenu->addAction(QString("Ângulo de Cobb (C)"), this, SLOT(select_tool_cobb()));
    measureMenu->addAction(QString("Desvio (D)"), this, SLOT(select_tool_deviation()));
    measureMenu->addAction(QString("Seta (Y)"), this, SLOT(select_tool_arrow()));
    measureMenu->addAction(QString("Lápis (P)"), this, SLOT(select_tool_pen()));
    measureMenu->addSeparator();
    measureMenu->addAction(QString("Apagar Medida Selecionada (Del)"), this, SLOT(delete_selected_roi()));
    measureMenu->addAction(QString("Apagar Todas as Medidas (Shift+Del)"), this, SLOT(delete_all_rois_image()));
    createMenuButton(action_measure, measureMenu, false);

    // 12. Botão de Filtros de Imagem (ao lado de medidas)
    action_filters = new QAction(QIcon(":/bitmaps/radiant_filters.svg"), QString("Filtros de Imagem"), this);
    QMenu * filterMenu = new QMenu(radiant_toolbar);
    filterMenu->addAction(QString("Nenhum Filtro (Shift+I)"), this, SLOT(apply_filter_none()));
    filterMenu->addSeparator();
    filterMenu->addAction(QString("Nitidez / Sharpen 1"), this, SLOT(apply_filter_sharpen1()));
    filterMenu->addAction(QString("Nitidez / Sharpen 2"), this, SLOT(apply_filter_sharpen2()));
    filterMenu->addAction(QString("Nitidez / Sharpen 3"), this, SLOT(apply_filter_sharpen3()));
    filterMenu->addSeparator();
    filterMenu->addAction(QString("Suavização / Smooth 1"), this, SLOT(apply_filter_smooth1()));
    filterMenu->addAction(QString("Suavização / Smooth 2"), this, SLOT(apply_filter_smooth2()));
    filterMenu->addAction(QString("Suavização / Smooth 3"), this, SLOT(apply_filter_smooth3()));
    filterMenu->addSeparator();
    filterMenu->addAction(QString("Bordas / Edge 1"), this, SLOT(apply_filter_edge1()));
    filterMenu->addAction(QString("Bordas / Edge 2"), this, SLOT(apply_filter_edge2()));
    filterMenu->addAction(QString("Bordas / Edge 3"), this, SLOT(apply_filter_edge3()));
    filterMenu->addSeparator();
    filterMenu->addAction(QString("Relevo / Emboss N"), this, SLOT(apply_filter_emboss_n()));
    filterMenu->addAction(QString("Relevo / Emboss W"), this, SLOT(apply_filter_emboss_w()));
    filterMenu->addAction(QString("Relevo / Emboss D"), this, SLOT(apply_filter_emboss_d()));
    filterMenu->addAction(QString("Relevo / Emboss 1"), this, SLOT(apply_filter_emboss1()));
    filterMenu->addAction(QString("Relevo / Emboss 2"), this, SLOT(apply_filter_emboss2()));
    filterMenu->addSeparator();
    filterMenu->addAction(QString("Próximo Filtro (Ctrl+I)"), this, SLOT(next_filter()));
    filterMenu->addAction(QString("Filtro Anterior (Ctrl+Shift+I)"), this, SLOT(prev_filter()));
    createMenuButton(action_filters, filterMenu, true);

    // 13. Espelhar vertical e horizontalmente a imagem
    action_transform = new QAction(QIcon(":/bitmaps/radiant_transform.svg"), QString("Espelhar e Girar"), this);
    QMenu * transformMenu = new QMenu(radiant_toolbar);
    transformMenu->addAction(QString("Espelhar Horizontalmente (Ctrl+Shift+[)"), this, SLOT(flip_horizontal()));
    transformMenu->addAction(QString("Espelhar Verticalmente (Ctrl+Shift+])"), this, SLOT(flip_vertical()));
    transformMenu->addAction(QString("Girar 90° Horário (Ctrl+])"), this, SLOT(rotate_90_cw()));
    transformMenu->addAction(QString("Girar 90° Anti-horário (Ctrl+[)"), this, SLOT(rotate_90_ccw()));
    transformMenu->addSeparator();
    transformMenu->addAction(QString("Restaurar Posição Original (Ctrl+Shift+\\)"), this, SLOT(clear_geometric_transforms()));
    createMenuButton(action_transform, transformMenu, true);

    // 14. Modo Cine
    action_cine = new QAction(QIcon(":/bitmaps/radiant_cine.svg"), QString("Modo Cine (Espaço)"), this);
    action_cine->setCheckable(true);
    connect(action_cine, SIGNAL(triggered()), this, SLOT(toggle_cine()));

    QMenu * cineMenu = new QMenu(radiant_toolbar);
    cineMenu->addAction(QString("Reproduzir / Pausar (Espaço)"), this, SLOT(toggle_cine()));
    cineMenu->addSeparator();
    auto addFps = [&](int fps) {
        QAction * a = cineMenu->addAction(QString("%1 FPS").arg(fps));
        connect(a, &QAction::triggered, [this, fps]() { this->set_cine_fps(fps); });
    };
    addFps(5);
    addFps(10);
    addFps(15);
    addFps(20);
    addFps(25);
    addFps(30);
    createMenuButton(action_cine, cineMenu, false);

    radiant_toolbar->addSeparator();

    // 15. MPR (Multiplanar)
    action_mpr = new QAction(QIcon(":/bitmaps/radiant_mpr.svg"), QString("MPR (F2)"), this);
    connect(action_mpr, SIGNAL(triggered()), this, SLOT(trigger_mpr_window()));
    QMenu * mprMenu = new QMenu(radiant_toolbar);
    QAction * oblique = mprMenu->addAction(QString("MPR oblíquo (indisponível)"));
    oblique->setEnabled(false);
    mprMenu->addSeparator();
    auto addPlane = [this, mprMenu](const QString & label, QAction * plane, int key) {
        QAction * item = mprMenu->addAction(label);
        item->setShortcut(QKeySequence(Qt::CTRL + Qt::ALT + key));
        item->setShortcutContext(Qt::WindowShortcut);
        mw->addAction(item);
        connect(item, &QAction::triggered, [plane]() { if (plane) plane->setChecked(true); });
    };
    addPlane(QString("Coronal"), mw->graphicsAct_Y, Qt::Key_1);
    addPlane(QString("Sagital"), mw->graphicsAct_X, Qt::Key_2);
    addPlane(QString("Axial"), mw->graphicsAct_Z, Qt::Key_3);
    mprMenu->addSeparator();
    mprMenu->addAction(mw->zyxAct);
    createMenuButton(action_mpr, mprMenu, false);
    setup_shortcut(QKeySequence(Qt::Key_F2), SLOT(trigger_mpr_window()), false);
    radiant_toolbar->addAction(mw->histogramAct);

    // 16. Renderização 3D (Volume Rendering) - Carregamento sob demanda
    action_3d_vr = new QAction(QIcon(":/bitmaps/radiant_3d.svg"), QString("Renderização 3D - Volume Rendering (F3)"), this);
    connect(action_3d_vr, SIGNAL(triggered()), this, SLOT(trigger_3d_vr_window()));
    radiant_toolbar->addAction(action_3d_vr);

    radiant_toolbar->addSeparator();

    // 17. Metadados DICOM
    action_dicom_meta = new QAction(QIcon(":/bitmaps/meta.svg"), QString("Metadados DICOM"), this);
    connect(action_dicom_meta, SIGNAL(triggered()), this, SLOT(trigger_dicom_metadata()));

    // 18. Ajuda
    action_help = new QAction(QIcon(":/bitmaps/radiant_help.svg"), QString("Ajuda e Guia do Usuário"), this);
    connect(action_help, SIGNAL(triggered()), this, SLOT(trigger_help_dialog()));

    // 19. Configuração
    action_config = new QAction(QIcon(":/bitmaps/radiant_config.svg"), QString("Configurações do Sistema"), this);
    connect(action_config, SIGNAL(triggered()), this, SLOT(trigger_config_dialog()));
    QMenu * configMenu = new QMenu(radiant_toolbar);
    configMenu->addAction(QString("Configurações Gerais..."), this, SLOT(trigger_config_dialog()));
    configMenu->addAction(QString("Configurações Avançadas do Sistema..."), this, SLOT(trigger_settings_tab()));
    configMenu->addSeparator();
    QMenu * referenceMenu = configMenu->addMenu(QString("Linhas de referência"));
    QActionGroup * referenceGroup = new QActionGroup(referenceMenu);
    referenceGroup->setExclusive(true);
    auto addReferenceMode = [this, referenceMenu, referenceGroup](const QString & label, int mode) {
        QAction * item = referenceMenu->addAction(label);
        item->setCheckable(true);
        item->setChecked(mode == 0);
        referenceGroup->addAction(item);
        connect(item, &QAction::triggered, [this, mode]() {
            if (mw && mw->aliza) mw->aliza->set_reference_mode(mode);
        });
    };
    addReferenceMode(QString("Corte atual"), 0);
    addReferenceMode(QString("Início e fim"), 1);
    addReferenceMode(QString("Todos os cortes"), 2);
    configMenu->addSeparator();
    configMenu->addAction(action_dicom_meta);
    configMenu->addAction(action_help);
    createMenuButton(action_config, configMenu, false);

    // Default active tool: Browse
    RadiantInputDispatcher::instance()->set_tool(RadiantTool::Browse);
}

void RadiantShortcutsManager::on_dispatcher_tool_changed(RadiantTool tool)
{
    if (action_wl) action_wl->setChecked(tool == RadiantTool::WindowLevel);
    if (action_pan) action_pan->setChecked(tool == RadiantTool::Pan);
    if (action_zoom) action_zoom->setChecked(tool == RadiantTool::Zoom);
    if (action_measure) action_measure->setChecked(tool == RadiantTool::Length ||
                                                  tool == RadiantTool::Ellipse ||
                                                  tool == RadiantTool::Angle ||
                                                  tool == RadiantTool::CobbAngle);
}

// =========================================================================
// SLOTS IMPLEMENTATION
// =========================================================================

void RadiantShortcutsManager::trigger_open_dir()
{
    if (mw && mw->openAct)
    {
        mw->openAct->trigger();
    }
}

void RadiantShortcutsManager::trigger_open_cd()
{
    // Search for optical drives or removable media
    QString cdPath;
    const auto volumes = QStorageInfo::mountedVolumes();
    for (const QStorageInfo & vol : volumes)
    {
        if (vol.isValid() && vol.isReady() && vol.isReadOnly())
        {
            cdPath = vol.rootPath();
            break;
        }
    }

    if (cdPath.isEmpty())
    {
        cdPath = QFileDialog::getExistingDirectory(mw, QString("Selecionar Unidade de CD/DVD / Mídia"),
            QString("D:\\"));
    }

    if (!cdPath.isEmpty() && mw && mw->aliza)
    {
        mw->open_args(QStringList() << cdPath);
    }
}

void RadiantShortcutsManager::trigger_pacs_download()
{
    if (mw)
    {
        mw->toggle_browser();
    }
}

void RadiantShortcutsManager::trigger_export()
{
    if (!mw) return;
    RadiantExportDialog dlg(mw, mw->aliza, mw);
    dlg.exec();
}

void RadiantShortcutsManager::set_grid_layout(int rows, int cols)
{
    if (!mw) return;
    if (mw->tabWidget)
    {
        mw->tabWidget->setCurrentIndex(1); // Multi-view tab
    }
    if (mw->studyview)
    {
        mw->studyview->update_grid(rows, cols);
    }
}

void RadiantShortcutsManager::select_tool_browse()
{
    if (mw) mw->set_image_view();
    RadiantInputDispatcher::instance()->set_tool(RadiantTool::Browse);
}

void RadiantShortcutsManager::select_tool_wl()
{
    if (mw) mw->set_image_view();
    RadiantInputDispatcher::instance()->set_tool(RadiantTool::WindowLevel);
}

void RadiantShortcutsManager::select_tool_pan()
{
    if (mw) mw->set_image_view();
    RadiantInputDispatcher::instance()->set_tool(RadiantTool::Pan);
}

void RadiantShortcutsManager::select_tool_zoom()
{
    if (mw) mw->set_image_view();
    RadiantInputDispatcher::instance()->set_tool(RadiantTool::Zoom);
}

void RadiantShortcutsManager::select_tool_length()
{
    if (mw) mw->set_image_view();
    RadiantInputDispatcher::instance()->set_tool(RadiantTool::Length);
    if (mw && mw->distanceAct) mw->distanceAct->setChecked(true);
}

void RadiantShortcutsManager::select_tool_ellipse()
{
    RadiantInputDispatcher::instance()->set_tool(RadiantTool::Ellipse);
}

void RadiantShortcutsManager::select_tool_closed_poly()
{
    RadiantInputDispatcher::instance()->set_tool(RadiantTool::ClosedPolygon);
}

void RadiantShortcutsManager::select_tool_open_poly()
{
    RadiantInputDispatcher::instance()->set_tool(RadiantTool::OpenPolygon);
}

void RadiantShortcutsManager::select_tool_angle()
{
    RadiantInputDispatcher::instance()->set_tool(RadiantTool::Angle);
}

void RadiantShortcutsManager::select_tool_cobb()
{
    RadiantInputDispatcher::instance()->set_tool(RadiantTool::CobbAngle);
}

void RadiantShortcutsManager::select_tool_deviation()
{
    RadiantInputDispatcher::instance()->set_tool(RadiantTool::Deviation);
}

void RadiantShortcutsManager::select_tool_arrow()
{
    RadiantInputDispatcher::instance()->set_tool(RadiantTool::Arrow);
}

void RadiantShortcutsManager::select_tool_pen()
{
    RadiantInputDispatcher::instance()->set_tool(RadiantTool::Pen);
}

// Presets (0 to 9)
void RadiantShortcutsManager::apply_preset_0()
{
    if (mw && mw->aliza) mw->aliza->apply_wl_preset(0);
}

void RadiantShortcutsManager::apply_preset_1()
{
    if (mw && mw->aliza) mw->aliza->apply_wl_preset(1);
}

void RadiantShortcutsManager::apply_preset_2()
{
    if (mw && mw->aliza) mw->aliza->apply_wl_preset(2); // Abdomen
}

void RadiantShortcutsManager::apply_preset_3()
{
    if (mw && mw->aliza) mw->aliza->apply_wl_preset(3); // Bone
}

void RadiantShortcutsManager::apply_preset_4()
{
    if (mw && mw->aliza) mw->aliza->apply_wl_preset(4); // Brain
}

void RadiantShortcutsManager::apply_preset_5()
{
    if (mw && mw->aliza) mw->aliza->apply_wl_preset(5); // Lung
}

void RadiantShortcutsManager::apply_preset_6()
{
    if (mw && mw->aliza) mw->aliza->apply_wl_preset(6); // Mediastinum
}

void RadiantShortcutsManager::apply_preset_7()
{
    if (mw && mw->aliza) mw->aliza->apply_wl_preset(7); // Pelvis
}

void RadiantShortcutsManager::apply_preset_8()
{
    if (mw && mw->aliza) mw->aliza->apply_wl_preset(8); // Angio
}

void RadiantShortcutsManager::apply_preset_9()
{
    if (mw && mw->aliza) mw->aliza->apply_wl_preset(9); // PET / Custom
}

void RadiantShortcutsManager::toggle_invert_lut()
{
    if (mw && mw->aliza) mw->aliza->toggle_invert_lut();
}

void RadiantShortcutsManager::show_custom_wl_dialog()
{
    if (!mw || !mw->aliza) return;
    bool ok = false;
    double c = QInputDialog::getDouble(mw, QString("Window Level"), QString("Level (WL):"), 40.0, -2000.0, 4000.0, 1, &ok);
    if (!ok) return;
    double w = QInputDialog::getDouble(mw, QString("Window Width"), QString("Width (WW):"), 350.0, 1.0, 8000.0, 1, &ok);
    if (!ok) return;
    mw->aliza->set_custom_window_level(c, w);
}

// Filters
void RadiantShortcutsManager::apply_filter_none()
{
    RadiantFilterManager::instance()->set_filter(RadiantFilterType::None);
    if (mw && mw->graphicswidget_m) mw->graphicswidget_m->set_smooth(true);
}

void RadiantShortcutsManager::apply_filter_sharpen1()
{
    RadiantFilterManager::instance()->set_filter(RadiantFilterType::Sharpen1);
    if (mw && mw->graphicswidget_m) mw->graphicswidget_m->set_smooth(false);
}

void RadiantShortcutsManager::apply_filter_sharpen2()
{
    RadiantFilterManager::instance()->set_filter(RadiantFilterType::Sharpen2);
    if (mw && mw->graphicswidget_m) mw->graphicswidget_m->set_smooth(false);
}

void RadiantShortcutsManager::apply_filter_sharpen3()
{
    RadiantFilterManager::instance()->set_filter(RadiantFilterType::Sharpen3);
    if (mw && mw->graphicswidget_m) mw->graphicswidget_m->set_smooth(false);
}

void RadiantShortcutsManager::apply_filter_smooth1()
{
    RadiantFilterManager::instance()->set_filter(RadiantFilterType::Smooth1);
    if (mw && mw->graphicswidget_m) mw->graphicswidget_m->set_smooth(true);
}

void RadiantShortcutsManager::apply_filter_smooth2()
{
    RadiantFilterManager::instance()->set_filter(RadiantFilterType::Smooth2);
    if (mw && mw->graphicswidget_m) mw->graphicswidget_m->set_smooth(true);
}

void RadiantShortcutsManager::apply_filter_smooth3()
{
    RadiantFilterManager::instance()->set_filter(RadiantFilterType::Smooth3);
    if (mw && mw->graphicswidget_m) mw->graphicswidget_m->set_smooth(true);
}

void RadiantShortcutsManager::apply_filter_edge1()
{
    RadiantFilterManager::instance()->set_filter(RadiantFilterType::Edge1);
}

void RadiantShortcutsManager::apply_filter_edge2()
{
    RadiantFilterManager::instance()->set_filter(RadiantFilterType::Edge2);
}

void RadiantShortcutsManager::apply_filter_edge3()
{
    RadiantFilterManager::instance()->set_filter(RadiantFilterType::Edge3);
}

void RadiantShortcutsManager::apply_filter_emboss_n()
{
    RadiantFilterManager::instance()->set_filter(RadiantFilterType::EmbossN);
}

void RadiantShortcutsManager::apply_filter_emboss_w()
{
    RadiantFilterManager::instance()->set_filter(RadiantFilterType::EmbossW);
}

void RadiantShortcutsManager::apply_filter_emboss_d()
{
    RadiantFilterManager::instance()->set_filter(RadiantFilterType::EmbossD);
}

void RadiantShortcutsManager::apply_filter_emboss1()
{
    RadiantFilterManager::instance()->set_filter(RadiantFilterType::Emboss1);
}

void RadiantShortcutsManager::apply_filter_emboss2()
{
    RadiantFilterManager::instance()->set_filter(RadiantFilterType::Emboss2);
}

void RadiantShortcutsManager::next_filter()
{
    RadiantFilterManager::instance()->next_filter();
}

void RadiantShortcutsManager::prev_filter()
{
    RadiantFilterManager::instance()->prev_filter();
}

void RadiantShortcutsManager::next_sharpen_filter()
{
    RadiantFilterManager::instance()->next_sharpen_filter();
}

// Navigation & Cine
void RadiantShortcutsManager::prev_image()
{
    if (mw && mw->slider_m && mw->slider_m->slices_slider)
    {
        const int cur = mw->slider_m->slices_slider->value();
        if (cur > 0) mw->slider_m->set_slice(cur - 1);
    }
}

void RadiantShortcutsManager::next_image()
{
    if (mw && mw->slider_m && mw->slider_m->slices_slider)
    {
        const int cur = mw->slider_m->slices_slider->value();
        const int max = mw->slider_m->slices_slider->maximum();
        if (cur < max) mw->slider_m->set_slice(cur + 1);
    }
}

void RadiantShortcutsManager::prev_10_images()
{
    if (mw && mw->slider_m && mw->slider_m->slices_slider)
    {
        const int cur = mw->slider_m->slices_slider->value();
        int next = cur - 10;
        if (next < 0) next = 0;
        mw->slider_m->set_slice(next);
    }
}

void RadiantShortcutsManager::next_10_images()
{
    if (mw && mw->slider_m && mw->slider_m->slices_slider)
    {
        const int cur = mw->slider_m->slices_slider->value();
        const int max = mw->slider_m->slices_slider->maximum();
        int next = cur + 10;
        if (next > max) next = max;
        mw->slider_m->set_slice(next);
    }
}

void RadiantShortcutsManager::prev_series()
{
    if (mw && mw->imagesbox) mw->imagesbox->prev_series();
}

void RadiantShortcutsManager::next_series()
{
    if (mw && mw->imagesbox) mw->imagesbox->next_series();
}

void RadiantShortcutsManager::first_image_or_series()
{
    if (mw && mw->slider_m) mw->slider_m->set_slice(0);
}

void RadiantShortcutsManager::last_image_or_series()
{
    if (mw && mw->slider_m && mw->slider_m->slices_slider)
        mw->slider_m->set_slice(mw->slider_m->slices_slider->maximum());
}

void RadiantShortcutsManager::next_panel()
{
    if (mw && mw->studyview) mw->studyview->set_next_panel();
}

void RadiantShortcutsManager::prev_panel()
{
    if (mw && mw->studyview) mw->studyview->set_prev_panel();
}

void RadiantShortcutsManager::toggle_cine()
{
    if (!cine_timer) {
        cine_timer = new QTimer(this);
        connect(cine_timer, &QTimer::timeout, this, &RadiantShortcutsManager::next_image);
    }
    if (cine_timer->isActive()) cine_timer->stop();
    else cine_timer->start(qMax(1, 1000 / current_fps));
    if (action_cine) action_cine->setChecked(cine_timer->isActive());
}

void RadiantShortcutsManager::set_cine_fps(int fps)
{
    if (fps <= 0) return;
    current_fps = fps;
    if (cine_timer && cine_timer->isActive()) cine_timer->setInterval(qMax(1, 1000 / fps));
}

// Zoom & Pan
void RadiantShortcutsManager::zoom_fit()
{
    if (mw && mw->graphicswidget_m && mw->graphicswidget_m->graphicsview)
        mw->graphicswidget_m->graphicsview->reset_zoom();
}

void RadiantShortcutsManager::zoom_100()
{
    if (mw && mw->graphicswidget_m && mw->graphicswidget_m->graphicsview)
        mw->graphicswidget_m->graphicsview->set_zoom_factor(1.0);
}

void RadiantShortcutsManager::zoom_200()
{
    if (mw && mw->graphicswidget_m && mw->graphicswidget_m->graphicsview)
        mw->graphicswidget_m->graphicsview->set_zoom_factor(2.0);
}

void RadiantShortcutsManager::zoom_400()
{
    if (mw && mw->graphicswidget_m && mw->graphicswidget_m->graphicsview)
        mw->graphicswidget_m->graphicsview->set_zoom_factor(4.0);
}

void RadiantShortcutsManager::zoom_800()
{
    if (mw && mw->graphicswidget_m && mw->graphicswidget_m->graphicsview)
        mw->graphicswidget_m->graphicsview->set_zoom_factor(8.0);
}

void RadiantShortcutsManager::zoom_in()
{
    if (mw && mw->graphicswidget_m && mw->graphicswidget_m->graphicsview)
        mw->graphicswidget_m->graphicsview->scale_view(1.15);
}

void RadiantShortcutsManager::zoom_out()
{
    if (mw && mw->graphicswidget_m && mw->graphicswidget_m->graphicsview)
        mw->graphicswidget_m->graphicsview->scale_view(1.0 / 1.15);
}

void RadiantShortcutsManager::pan_up()
{
    if (mw && mw->graphicswidget_m && mw->graphicswidget_m->graphicsview)
        mw->graphicswidget_m->graphicsview->pan_view(0, -20);
}

void RadiantShortcutsManager::pan_down()
{
    if (mw && mw->graphicswidget_m && mw->graphicswidget_m->graphicsview)
        mw->graphicswidget_m->graphicsview->pan_view(0, 20);
}

void RadiantShortcutsManager::pan_left()
{
    if (mw && mw->graphicswidget_m && mw->graphicswidget_m->graphicsview)
        mw->graphicswidget_m->graphicsview->pan_view(-20, 0);
}

void RadiantShortcutsManager::pan_right()
{
    if (mw && mw->graphicswidget_m && mw->graphicswidget_m->graphicsview)
        mw->graphicswidget_m->graphicsview->pan_view(20, 0);
}

// Geometric Transformations
void RadiantShortcutsManager::rotate_90_ccw()
{
    if (mw && mw->graphicswidget_m && mw->graphicswidget_m->graphicsview)
        mw->graphicswidget_m->graphicsview->rotate_view(-90.0);
}

void RadiantShortcutsManager::rotate_90_cw()
{
    if (mw && mw->graphicswidget_m && mw->graphicswidget_m->graphicsview)
        mw->graphicswidget_m->graphicsview->rotate_view(90.0);
}

void RadiantShortcutsManager::flip_horizontal()
{
    if (mw) mw->flipX();
}

void RadiantShortcutsManager::flip_vertical()
{
    if (mw) mw->flipY();
}

void RadiantShortcutsManager::clear_geometric_transforms()
{
    if (mw && mw->graphicswidget_m && mw->graphicswidget_m->graphicsview)
    {
        mw->graphicswidget_m->graphicsview->reset_transformations();
        mw->graphicswidget_m->graphicsview->reset_zoom();
    }
}

// Overlays & HUD
void RadiantShortcutsManager::toggle_hud()
{
    GraphicsView::toggle_hud_display();
    if (mw && mw->graphicswidget_m && mw->graphicswidget_m->graphicsview)
        mw->graphicswidget_m->graphicsview->viewport()->update();
    if (mw && mw->studyview)
        mw->studyview->update();
}

void RadiantShortcutsManager::toggle_patient_id_hud()
{
    GraphicsView::toggle_anonymous_display();
    if (mw && mw->graphicswidget_m && mw->graphicswidget_m->graphicsview)
        mw->graphicswidget_m->graphicsview->viewport()->update();
    if (mw && mw->studyview)
        mw->studyview->update();
}

void RadiantShortcutsManager::toggle_cross_reference()
{
    if (mw && mw->cursorAct) mw->cursorAct->trigger();
}

void RadiantShortcutsManager::toggle_measurements()
{
    if (mw && mw->distanceAct) mw->distanceAct->trigger();
}

void RadiantShortcutsManager::toggle_series_sync()
{
    if (mw && mw->studyview) mw->studyview->toggle_sync_series();
}

// MPR & 3D Overlaid Windows
void RadiantShortcutsManager::trigger_mpr_window()
{
    if (cine_timer && cine_timer->isActive()) toggle_cine();
    if (!mw) return;

    if (!mpr_window)
    {
        mpr_window = new RadiantOverlayWindow(
            QString("MPR - Reformatação Multiplanar Ortogonal"),
            mw->multi_frame, mw, RadiantOverlayWindow::OverlayType::MPR);
        connect(mpr_window, SIGNAL(window_closed()), this, SLOT(on_mpr_window_closed()));
    }

    if (mpr_window->isVisible())
    {
        mpr_window->hide();
    }
    else
    {
        mw->toggle_graphicswidget_zyx(true);
        mpr_window->show();
        mpr_window->raise();
        mpr_window->activateWindow();
    }
}

void RadiantShortcutsManager::on_mpr_window_closed()
{
    if (mw)
    {
        mw->toggle_graphicswidget_m_z(true);
    }
}

void RadiantShortcutsManager::trigger_3d_vr_window()
{
    if (!mw || !mw->aliza) return;
    if (!mw->glwidget)
    {
        QMessageBox::warning(mw, QString("Visualização 3D"),
            QString("A visualização 3D não está disponível nesta configuração de OpenGL."));
        return;
    }
    if (!mw->aliza->get_selected_image())
    {
        QMessageBox::information(mw, QString("Visualização 3D"),
            QString("Selecione uma série volumétrica antes de abrir a visualização 3D."));
        return;
    }
    if (!vr_3d_window)
    {
        vr_3d_window = new RadiantOverlayWindow(
            QString("Reconstrução 3D"), mw->gl_frame, mw,
            RadiantOverlayWindow::OverlayType::VR_3D);
        connect(vr_3d_window, SIGNAL(window_closed()), this, SLOT(on_3d_window_closed()));
    }
    if (vr_3d_window->isVisible())
    {
        vr_3d_window->hide();
        on_3d_window_closed();
        return;
    }
    // Showing the widget creates its OpenGL context before allocating textures.
    vr_3d_window->show();
    qApp->processEvents();
    if (!mw->aliza->prepare_selected_volume_3d())
    {
        vr_3d_window->hide();
        QMessageBox::warning(mw, QString("Visualização 3D"),
            QString("Não foi possível criar o volume 3D desta série. Verifique se é uma série volumétrica compatível e se o OpenGL está disponível."));
        return;
    }
    mw->show3DAct->blockSignals(true);
    mw->show3DAct->setChecked(true);
    mw->show3DAct->blockSignals(false);
    mw->aliza->update_selection();
    mw->raycastAct->setChecked(true);
    mw->set_view_rc(true);
    vr_3d_window->raise();
    vr_3d_window->activateWindow();
}

void RadiantShortcutsManager::on_3d_window_closed()
{
    if (mw && mw->show3DAct)
    {
        mw->show3DAct->blockSignals(true);
        mw->show3DAct->setChecked(false);
        mw->show3DAct->blockSignals(false);
    }
}

// Help & Config
void RadiantShortcutsManager::trigger_help_dialog()
{
    if (mw && mw->aboutAct)
    {
        mw->aboutAct->trigger();
    }
}

void RadiantShortcutsManager::trigger_config_dialog()
{
    if (!mw) return;
    RadiantConfigDialog dlg(mw, mw);
    dlg.exec();
}

void RadiantShortcutsManager::trigger_deidentify()
{
    if (mw)
    {
        mw->toggle_deidentify();
    }
}

void RadiantShortcutsManager::trigger_dicom_metadata()
{
    if (mw)
    {
        mw->trigger_image_dicom_meta();
    }
}

void RadiantShortcutsManager::trigger_settings_tab()
{
    if (mw)
    {
        mw->toggle_settingswidget();
    }
}

// ROIs deletion
void RadiantShortcutsManager::delete_selected_roi()
{
    if (mw && mw->graphicswidget_m && mw->graphicswidget_m->graphicsview)
        mw->graphicswidget_m->graphicsview->delete_selected_measurement();
}

void RadiantShortcutsManager::delete_all_rois_image()
{
    if (mw && mw->graphicswidget_m && mw->graphicswidget_m->graphicsview)
        mw->graphicswidget_m->graphicsview->clear_measurements();
}

void RadiantShortcutsManager::delete_all_rois_series()
{
    delete_all_rois_image();
}

void RadiantShortcutsManager::delete_all_rois_all()
{
    delete_all_rois_image();
}

// MPR 3D
void RadiantShortcutsManager::mpr3d_toggle_mode() {}
void RadiantShortcutsManager::mpr3d_toggle_axes() {}
void RadiantShortcutsManager::mpr3d_center_axes() {}
void RadiantShortcutsManager::mpr3d_layout() {}
void RadiantShortcutsManager::mpr3d_slab_thickness() {}
void RadiantShortcutsManager::mpr3d_increase_slab() {}
void RadiantShortcutsManager::mpr3d_decrease_slab() {}
void RadiantShortcutsManager::mpr3d_rotate() {}
void RadiantShortcutsManager::mpr3d_roll() {}
void RadiantShortcutsManager::mpr3d_maximize() {}
void RadiantShortcutsManager::mpr3d_reset() {}
