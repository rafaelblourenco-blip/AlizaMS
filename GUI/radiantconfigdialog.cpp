#include "radiantconfigdialog.h"
#include "mainwindow.h"
#include "graphicswidget.h"
#include "graphicsview.h"
#include "studyviewwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QTableWidgetItem>
#include <QMessageBox>

static RadiantHudConfig s_hud_config;

RadiantHudConfig & RadiantConfigDialog::get_hud_config()
{
    return s_hud_config;
}

RadiantConfigDialog::RadiantConfigDialog(MainWindow * parentWindow, QWidget * parent)
    : QDialog(parent ? parent : parentWindow), mw(parentWindow)
{
    setWindowTitle(QString("Configurações do Sistema (RadiAnt / RadDoc Edition)"));
    resize(580, 440);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    QVBoxLayout * rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(14, 14, 14, 14);
    rootLayout->setSpacing(12);

    QTabWidget * tabWidget = new QTabWidget(this);

    // =========================================================================
    // TAB 1: Atributos DICOM no HUD (Sobreposições)
    // =========================================================================
    QWidget * tabHud = new QWidget();
    QVBoxLayout * hudLayout = new QVBoxLayout(tabHud);
    hudLayout->setContentsMargins(12, 12, 12, 12);
    hudLayout->setSpacing(10);

    QLabel * hudInfo = new QLabel(
        QString("Selecione quais atributos DICOM devem ser exibidos nos 4 cantos da tela (HUD):"), tabHud);
    hudInfo->setStyleSheet(QString("color: #38BDF8; font-weight: bold;"));
    hudLayout->addWidget(hudInfo);

    QGroupBox * grpHud = new QGroupBox(QString("Campos Visíveis"), tabHud);
    QGridLayout * hudGrid = new QGridLayout(grpHud);
    hudGrid->setContentsMargins(12, 12, 12, 12);
    hudGrid->setSpacing(8);

    chk_pat_name = new QCheckBox(QString("Nome do Paciente"), grpHud);
    chk_pat_name->setChecked(s_hud_config.show_patient_name);
    hudGrid->addWidget(chk_pat_name, 0, 0);

    chk_pat_id = new QCheckBox(QString("ID do Paciente"), grpHud);
    chk_pat_id->setChecked(s_hud_config.show_patient_id);
    hudGrid->addWidget(chk_pat_id, 0, 1);

    chk_study_date = new QCheckBox(QString("Data do Exame"), grpHud);
    chk_study_date->setChecked(s_hud_config.show_study_date);
    hudGrid->addWidget(chk_study_date, 1, 0);

    chk_series_desc = new QCheckBox(QString("Descrição da Série"), grpHud);
    chk_series_desc->setChecked(s_hud_config.show_series_desc);
    hudGrid->addWidget(chk_series_desc, 1, 1);

    chk_institution = new QCheckBox(QString("Instituição / Clínica"), grpHud);
    chk_institution->setChecked(s_hud_config.show_institution);
    hudGrid->addWidget(chk_institution, 2, 0);

    chk_modality = new QCheckBox(QString("Modalidade (CT, MR, CR...)"), grpHud);
    chk_modality->setChecked(s_hud_config.show_modality);
    hudGrid->addWidget(chk_modality, 2, 1);

    chk_tech_params = new QCheckBox(QString("Parâmetros Técnicos (kVp, mA, FOV)"), grpHud);
    chk_tech_params->setChecked(s_hud_config.show_tech_params);
    hudGrid->addWidget(chk_tech_params, 3, 0);

    chk_thickness_loc = new QCheckBox(QString("Espessura e Localização de Fatia"), grpHud);
    chk_thickness_loc->setChecked(s_hud_config.show_thickness_loc);
    hudGrid->addWidget(chk_thickness_loc, 3, 1);

    chk_zoom = new QCheckBox(QString("Fator de Zoom Atual"), grpHud);
    chk_zoom->setChecked(s_hud_config.show_zoom);
    hudGrid->addWidget(chk_zoom, 4, 0);

    chk_hu_probe = new QCheckBox(QString("Medição Dinâmica em HU sob Cursor"), grpHud);
    chk_hu_probe->setChecked(s_hud_config.show_hu_probe);
    hudGrid->addWidget(chk_hu_probe, 4, 1);

    hudLayout->addWidget(grpHud);
    hudLayout->addStretch(1);
    tabWidget->addTab(tabHud, QString("Atributos DICOM"));

    // =========================================================================
    // TAB 2: Posição do Painel de Miniaturas (Thumbnails)
    // =========================================================================
    QWidget * tabDock = new QWidget();
    QVBoxLayout * dockLayout = new QVBoxLayout(tabDock);
    dockLayout->setContentsMargins(12, 12, 12, 12);
    dockLayout->setSpacing(10);

    QLabel * dockInfo = new QLabel(
        QString("Configure o posicionamento do painel lateral de séries (miniaturas / thumbnails):"), tabDock);
    dockInfo->setStyleSheet(QString("color: #38BDF8; font-weight: bold;"));
    dockLayout->addWidget(dockInfo);

    QGroupBox * grpDockPos = new QGroupBox(QString("Posição do Painel"), tabDock);
    QVBoxLayout * dockVBox = new QVBoxLayout(grpDockPos);
    dockVBox->setSpacing(8);

    radio_dock_left = new QRadioButton(QString("À Esquerda (Padrão RadiAnt)"), grpDockPos);
    radio_dock_right = new QRadioButton(QString("À Direita"), grpDockPos);
    radio_dock_top = new QRadioButton(QString("Superior (Acima das Imagens)"), grpDockPos);
    radio_dock_bottom = new QRadioButton(QString("Inferior (Abaixo das Imagens)"), grpDockPos);

    QButtonGroup * dockGroup = new QButtonGroup(this);
    dockGroup->addButton(radio_dock_left, 0);
    dockGroup->addButton(radio_dock_right, 1);
    dockGroup->addButton(radio_dock_top, 2);
    dockGroup->addButton(radio_dock_bottom, 3);

    // Read current dock widget position from MainWindow
    if (mw && mw->dockWidget1)
    {
        const Qt::DockWidgetArea area = mw->dockWidgetArea(mw->dockWidget1);
        if (area == Qt::RightDockWidgetArea) radio_dock_right->setChecked(true);
        else if (area == Qt::TopDockWidgetArea) radio_dock_top->setChecked(true);
        else if (area == Qt::BottomDockWidgetArea) radio_dock_bottom->setChecked(true);
        else radio_dock_left->setChecked(true);
    }
    else
    {
        radio_dock_left->setChecked(true);
    }

    dockVBox->addWidget(radio_dock_left);
    dockVBox->addWidget(radio_dock_right);
    dockVBox->addWidget(radio_dock_top);
    dockVBox->addWidget(radio_dock_bottom);
    dockLayout->addWidget(grpDockPos);

    chk_dock_visible = new QCheckBox(QString("Exibir painel de miniaturas (desmarque para recolher / ocultar)"), tabDock);
    chk_dock_visible->setChecked(mw && mw->dockWidget1 && mw->dockWidget1->isVisible());
    dockLayout->addWidget(chk_dock_visible);

    dockLayout->addStretch(1);
    tabWidget->addTab(tabDock, QString("Miniaturas (Thumbnails)"));

    // =========================================================================
    // TAB 3: Presets de Janelamento (0 a 9)
    // =========================================================================
    QWidget * tabPresets = new QWidget();
    QVBoxLayout * presetLayout = new QVBoxLayout(tabPresets);
    presetLayout->setContentsMargins(12, 12, 12, 12);

    QLabel * presetInfo = new QLabel(
        QString("Predefinições de Janela (acessíveis via teclas numéricas 0 a 9):"), tabPresets);
    presetInfo->setStyleSheet(QString("color: #38BDF8; font-weight: bold;"));
    presetLayout->addWidget(presetInfo);

    QTableWidget * tablePresets = new QTableWidget(10, 4, tabPresets);
    tablePresets->setHorizontalHeaderLabels(QStringList() << QString("Tecla") << QString("Preset") << QString("Level (WL)") << QString("Width (WW)"));
    tablePresets->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    tablePresets->verticalHeader()->setVisible(false);
    tablePresets->setEditTriggers(QAbstractItemView::NoEditTriggers);

    auto addPresetRow = [&](int row, const QString & k, const QString & name, const QString & l, const QString & w) {
        tablePresets->setItem(row, 0, new QTableWidgetItem(k));
        tablePresets->setItem(row, 1, new QTableWidgetItem(name));
        tablePresets->setItem(row, 2, new QTableWidgetItem(l));
        tablePresets->setItem(row, 3, new QTableWidgetItem(w));
    };

    addPresetRow(0, "0", "Reset (Original DICOM)", "Auto", "Auto");
    addPresetRow(1, "1", "Alcance Total (Full Dynamic)", "Auto", "Auto");
    addPresetRow(2, "2", "Abdome TC", "40 HU", "350 HU");
    addPresetRow(3, "3", "Osso / Bone TC", "300 HU", "1500 HU");
    addPresetRow(4, "4", "Cérebro / Brain TC", "40 HU", "80 HU");
    addPresetRow(5, "5", "Pulmão / Lung TC", "-600 HU", "1500 HU");
    addPresetRow(6, "6", "Mediastino TC", "50 HU", "350 HU");
    addPresetRow(7, "7", "Pélvis TC", "40 HU", "400 HU");
    addPresetRow(8, "8", "Angio TC", "300 HU", "600 HU");
    addPresetRow(9, "9", "Customizado / PET", "2.5", "5.0");

    presetLayout->addWidget(tablePresets);
    tabWidget->addTab(tabPresets, QString("Presets WL / WW"));

    // =========================================================================
    // TAB 4: Atalhos de Teclado
    // =========================================================================
    QWidget * tabShortcuts = new QWidget();
    QVBoxLayout * scLayout = new QVBoxLayout(tabShortcuts);
    scLayout->setContentsMargins(12, 12, 12, 12);

    QLabel * scInfo = new QLabel(
        QString("Atalhos Oficiais RadiAnt configurados no sistema:"), tabShortcuts);
    scInfo->setStyleSheet(QString("color: #38BDF8; font-weight: bold;"));
    scLayout->addWidget(scInfo);

    QTableWidget * tableSc = new QTableWidget(12, 2, tabShortcuts);
    tableSc->setHorizontalHeaderLabels(QStringList() << QString("Tecla / Atalho") << QString("Ação"));
    tableSc->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    tableSc->verticalHeader()->setVisible(false);
    tableSc->setEditTriggers(QAbstractItemView::NoEditTriggers);

    auto addScRow = [&](int row, const QString & sc, const QString & desc) {
        tableSc->setItem(row, 0, new QTableWidgetItem(sc));
        tableSc->setItem(row, 1, new QTableWidgetItem(desc));
    };

    addScRow(0, "B / W / M / Z", "Ferramentas: Browse, Janelamento, Pan, Zoom");
    addScRow(1, "L / E / A / C / G", "Medidas: Régua, Elipse, Ângulo, Ângulo de Cobb, Polígono");
    addScRow(2, "Botão do Meio (Scroll)", "Ajuste Contínuo de Brilho e Contraste (WL / WW)");
    addScRow(3, "Botão Direito", "Zoom Suave Contínuo");
    addScRow(4, "Roda do Mouse (Wheel)", "Navegar fatias da série");
    addScRow(5, "Espaço", "Iniciar / Parar reprodução Cine");
    addScRow(6, "Ctrl + [ / Ctrl + ]", "Girar imagem 90° anti-horário / horário");
    addScRow(7, "Ctrl + Shift + [ / ]", "Espelhar imagem horizontal / vertical");
    addScRow(8, "F12 / Shift + F12", "Exibir/ocultar anotações HUD / Modo Anônimo");
    addScRow(9, "F11 / Ctrl + F11", "Inverter LUT (Negativo) / Janelamento manual");
    addScRow(10, "Ctrl + Alt + 1 / 2 / 3", "Reformatação MPR Coronal / Sagital / Axial");
    addScRow(11, "Ctrl + O / Ctrl + Shift + O", "Abrir arquivo DICOM / Abrir pasta DICOM");

    scLayout->addWidget(tableSc);
    tabWidget->addTab(tabShortcuts, QString("Atalhos de Teclado"));

    rootLayout->addWidget(tabWidget);

    // Buttons
    QHBoxLayout * bottomLayout = new QHBoxLayout();
    bottomLayout->addStretch(1);

    btn_apply = new QPushButton(QString("Aplicar"), this);
    connect(btn_apply, SIGNAL(clicked()), this, SLOT(on_apply_clicked()));
    bottomLayout->addWidget(btn_apply);

    btn_save = new QPushButton(QString("OK"), this);
    btn_save->setDefault(true);
    connect(btn_save, SIGNAL(clicked()), this, SLOT(on_save_clicked()));
    bottomLayout->addWidget(btn_save);

    btn_cancel = new QPushButton(QString("Cancelar"), this);
    connect(btn_cancel, SIGNAL(clicked()), this, SLOT(reject()));
    bottomLayout->addWidget(btn_cancel);

    rootLayout->addLayout(bottomLayout);
}

void RadiantConfigDialog::on_apply_clicked()
{
    // Update HUD config
    s_hud_config.show_patient_name = chk_pat_name->isChecked();
    s_hud_config.show_patient_id = chk_pat_id->isChecked();
    s_hud_config.show_study_date = chk_study_date->isChecked();
    s_hud_config.show_series_desc = chk_series_desc->isChecked();
    s_hud_config.show_institution = chk_institution->isChecked();
    s_hud_config.show_modality = chk_modality->isChecked();
    s_hud_config.show_tech_params = chk_tech_params->isChecked();
    s_hud_config.show_thickness_loc = chk_thickness_loc->isChecked();
    s_hud_config.show_zoom = chk_zoom->isChecked();
    s_hud_config.show_hu_probe = chk_hu_probe->isChecked();

    // Update Dock Widget
    if (mw && mw->dockWidget1)
    {
        mw->dockWidget1->setAllowedAreas(Qt::AllDockWidgetAreas);
        if (radio_dock_right->isChecked())
        {
            mw->addDockWidget(Qt::RightDockWidgetArea, mw->dockWidget1);
        }
        else if (radio_dock_top->isChecked())
        {
            mw->addDockWidget(Qt::TopDockWidgetArea, mw->dockWidget1);
        }
        else if (radio_dock_bottom->isChecked())
        {
            mw->addDockWidget(Qt::BottomDockWidgetArea, mw->dockWidget1);
        }
        else
        {
            mw->addDockWidget(Qt::LeftDockWidgetArea, mw->dockWidget1);
        }

        mw->dockWidget1->setVisible(chk_dock_visible->isChecked());
    }

    // Refresh viewports
    if (mw && mw->graphicswidget_m && mw->graphicswidget_m->graphicsview)
    {
        mw->graphicswidget_m->graphicsview->viewport()->update();
    }
}

void RadiantConfigDialog::on_save_clicked()
{
    on_apply_clicked();
    accept();
}
