#include "pacsdialog.h"
#include "mainwindow.h"
#include <QApplication>
#include <QComboBox>
#include <QDateEdit>
#include <QDateTime>
#include <QDialog>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileDialog>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QProcess>
#include <QProgressDialog>
#include <QPushButton>
#include <QSettings>
#include <QShortcut>
#include <QSpinBox>
#include <QSplitter>
#include <QStandardPaths>
#include <QTableWidget>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>
#include <QAction>
#include <QScrollBar>
#include <QCalendarWidget>

// =========================================================================
// PacsCacheManager: Gestão do Cache Volátil
// =========================================================================

QString PacsCacheManager::s_currentCacheDir = QString();

QString PacsCacheManager::getCacheRoot()
{
    const QString root = QDir::tempPath() + QStringLiteral("/AlizaMS/pacs_cache");
    QDir().mkpath(root);
    return root;
}

QString PacsCacheManager::prepareStudyCacheDir(const QString & studyUid)
{
    cleanupCurrentCache();
    const QString root = getCacheRoot();
    const QString safeUid = studyUid.isEmpty() ? QStringLiteral("study") : studyUid.right(16);
    const QString dirName = QStringLiteral("pacs_%1_%2").arg(safeUid).arg(QDateTime::currentMSecsSinceEpoch());
    const QString fullPath = root + QStringLiteral("/") + dirName;
    QDir().mkpath(fullPath);
    s_currentCacheDir = fullPath;
    return fullPath;
}

void PacsCacheManager::cleanupCurrentCache()
{
    if (!s_currentCacheDir.isEmpty() && QDir(s_currentCacheDir).exists())
    {
        QDir(s_currentCacheDir).removeRecursively();
        s_currentCacheDir.clear();
    }
}

void PacsCacheManager::cleanupOtherCaches(const QString & exceptDir)
{
    const QString root = getCacheRoot();
    QDir dir(root);
    if (!dir.exists()) return;
    const QStringList subdirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString & sub : subdirs)
    {
        const QString subPath = dir.absoluteFilePath(sub);
        if (!exceptDir.isEmpty() && (subPath == exceptDir || exceptDir.startsWith(subPath)))
        {
            continue;
        }
        QDir(subPath).removeRecursively();
    }
    if (s_currentCacheDir != exceptDir)
    {
        s_currentCacheDir = exceptDir;
    }
}

void PacsCacheManager::cleanupAllCaches()
{
    cleanupCurrentCache();
    const QString root = getCacheRoot();
    QDir dir(root);
    if (dir.exists())
    {
        dir.removeRecursively();
    }
}

QString PacsCacheManager::getCurrentCacheDir()
{
    return s_currentCacheDir;
}

void PacsCacheManager::setCurrentCacheDir(const QString &dir)
{
    s_currentCacheDir = dir;
}

// =========================================================================
// ModalityCheckCombo: Botão com popup de checkboxes de modalidades
// =========================================================================

ModalityCheckCombo::ModalityCheckCombo(QWidget *parent)
    : QToolButton(parent)
{
    setPopupMode(QToolButton::InstantPopup);
    setToolButtonStyle(Qt::ToolButtonTextOnly);
    setStyleSheet(QStringLiteral(
        "QToolButton { min-width:110px; padding: 2px 18px 2px 6px; "
        "border: 1px solid #555; border-radius: 3px; text-align: left; }"
        "QToolButton::menu-indicator { image: none; }"
        "QToolButton:hover { border-color: #0284C7; }"));

    m_list = new QListWidget(this);
    m_list->setWindowFlags(Qt::Popup);
    m_list->setFrameShape(QFrame::StyledPanel);
    m_list->setMinimumWidth(160);
    m_list->setMaximumHeight(360);

    // "Todas as modalidades" como item especial (sem checkbox, header)
    QListWidgetItem *allItem = new QListWidgetItem(QString::fromUtf8("Todas as modalidades"), m_list);
    allItem->setCheckState(Qt::Checked);
    allItem->setData(Qt::UserRole, QStringLiteral("ALL"));

    QListWidgetItem *sep = new QListWidgetItem(QString::fromUtf8("─────────────────"), m_list);
    sep->setFlags(Qt::NoItemFlags);

    const QStringList modalities = {
        "CR","DX","CT","MR","XA","MG","US","PT","NM","RF","RG","PX","ES","XC","GM","SC","OT","SR"
    };
    for (const QString &m : modalities) {
        QListWidgetItem *item = new QListWidgetItem(m, m_list);
        item->setCheckState(Qt::Unchecked);
        item->setData(Qt::UserRole, m);
    }

    connect(m_list, &QListWidget::itemChanged, this, &ModalityCheckCombo::onItemChanged);
    connect(this, &QToolButton::clicked, this, &ModalityCheckCombo::showPopup);

    updateLabel();
}

void ModalityCheckCombo::showPopup()
{
    const QPoint globalPos = mapToGlobal(QPoint(0, height()));
    m_list->move(globalPos);
    m_list->resize(qMax(width(), m_list->minimumWidth()), m_list->sizeHintForRow(0) * qMin(m_list->count(), 20) + 4);
    m_list->show();
    m_list->raise();
}

void ModalityCheckCombo::onItemChanged()
{
    updateLabel();
    emit selectionChanged();
}

void ModalityCheckCombo::updateLabel()
{
    const QStringList sel = selectedModalities();
    if (sel.isEmpty()) {
        setText(QString::fromUtf8("Todas as modalidades ▾"));
    } else if (sel.size() == 1) {
        setText(sel.first() + QStringLiteral(" ▾"));
    } else {
        setText(sel.join(QStringLiteral(",")) + QStringLiteral(" ▾"));
    }
}

QStringList ModalityCheckCombo::selectedModalities() const
{
    QStringList result;
    for (int i = 0; i < m_list->count(); ++i) {
        const QListWidgetItem *item = m_list->item(i);
        if (!item) continue;
        const QString data = item->data(Qt::UserRole).toString();
        if (data == QStringLiteral("ALL") || data.isEmpty()) continue;
        if (item->checkState() == Qt::Checked)
            result << data;
    }
    return result;
}

void ModalityCheckCombo::clearSelection()
{
    QSignalBlocker blocker(m_list);
    for (int i = 0; i < m_list->count(); ++i) {
        QListWidgetItem *item = m_list->item(i);
        if (!item) continue;
        const QString data = item->data(Qt::UserRole).toString();
        if (data == QStringLiteral("ALL"))
            item->setCheckState(Qt::Checked);
        else if (!data.isEmpty())
            item->setCheckState(Qt::Unchecked);
    }
    updateLabel();
}

// =========================================================================
// PacsQueryRetrieveDialog: Janela de Pesquisa e Download PACS (RadiAnt UX)
// =========================================================================

PacsQueryRetrieveDialog::PacsQueryRetrieveDialog(MainWindow *parent)
    : QDialog(parent), m_mainWindow(parent)
{
    setupUi();
    loadProfiles();
    updateDateControls();
}

void PacsQueryRetrieveDialog::setupUi()
{
    setWindowTitle(QString::fromUtf8("Pesquisar e baixar exames dos servidores PACS"));
    resize(1120, 720);
    setMinimumSize(880, 560);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(8);

    // -------------------------------------------------------------------------
    // Linha 1: Controles de Topo (Servidor, Modalidade, Presets de Data)
    // -------------------------------------------------------------------------
    QHBoxLayout *topRow = new QHBoxLayout;
    topRow->setSpacing(6);

    m_settingsBtn = new QToolButton(this);
    m_settingsBtn->setText(QString::fromUtf8("⚙"));
    m_settingsBtn->setToolTip(QString::fromUtf8("Configurar servidores DICOM / PACS"));
    m_settingsBtn->setFixedSize(28, 26);
    topRow->addWidget(m_settingsBtn);

    m_serverCombo = new QComboBox(this);
    m_serverCombo->setMinimumWidth(180);
    topRow->addWidget(m_serverCombo);

    // Modalidade com checkboxes (estilo RadiAnt)
    m_modalityCombo = new ModalityCheckCombo(this);
    topRow->addWidget(m_modalityCombo);

    // Preset de datas no estilo RadiAnt (All dates primeiro, como no screenshot)
    m_datePresetCombo = new QComboBox(this);
    m_datePresetCombo->setMinimumWidth(160);
    m_datePresetCombo->addItem(QString::fromUtf8("Todas as datas"),        QStringLiteral("ALL"));
    m_datePresetCombo->insertSeparator(1);
    m_datePresetCombo->addItem(QString::fromUtf8("Hoje"),                  QStringLiteral("TODAY"));
    m_datePresetCombo->addItem(QString::fromUtf8("Ontem"),                 QStringLiteral("YESTERDAY"));
    m_datePresetCombo->addItem(QString::fromUtf8("Última semana"),         QStringLiteral("LAST_WEEK"));
    m_datePresetCombo->addItem(QString::fromUtf8("Último mês"),            QStringLiteral("LAST_MONTH"));
    m_datePresetCombo->addItem(QString::fromUtf8("Último ano"),            QStringLiteral("LAST_YEAR"));
    m_datePresetCombo->insertSeparator(8);
    m_datePresetCombo->addItem(QString::fromUtf8("Data personalizada"),    QStringLiteral("CUSTOM_DATE"));
    m_datePresetCombo->addItem(QString::fromUtf8("Período personalizado"), QStringLiteral("CUSTOM_RANGE"));
    // Seleciona "Hoje" por padrão (índice 2 após separadores)
    m_datePresetCombo->setCurrentIndex(2);
    topRow->addWidget(m_datePresetCombo);

    m_startDateEdit = new QDateEdit(this);
    m_startDateEdit->setCalendarPopup(true);
    m_startDateEdit->setDisplayFormat(QStringLiteral("dd/MM/yyyy"));
    m_startDateEdit->setDate(QDate::currentDate());
    // Calendário em PT-BR
    if (m_startDateEdit->calendarWidget()) {
        m_startDateEdit->calendarWidget()->setLocale(QLocale(QLocale::Portuguese, QLocale::Brazil));
    }
    topRow->addWidget(m_startDateEdit);

    m_endDateEdit = new QDateEdit(this);
    m_endDateEdit->setCalendarPopup(true);
    m_endDateEdit->setDisplayFormat(QStringLiteral("dd/MM/yyyy"));
    m_endDateEdit->setDate(QDate::currentDate());
    if (m_endDateEdit->calendarWidget()) {
        m_endDateEdit->calendarWidget()->setLocale(QLocale(QLocale::Portuguese, QLocale::Brazil));
    }
    topRow->addWidget(m_endDateEdit);

    m_clearDateBtn = new QPushButton(QString::fromUtf8("Limpar data"), this);
    m_clearDateBtn->setFixedHeight(26);
    topRow->addWidget(m_clearDateBtn);

    topRow->addStretch(1);
    mainLayout->addLayout(topRow);

    // -------------------------------------------------------------------------
    // Linha 2: Busca Rápida por Campo (RadiAnt style)
    // -------------------------------------------------------------------------
    QHBoxLayout *searchRow = new QHBoxLayout;
    searchRow->setSpacing(6);

    m_searchFieldCombo = new QComboBox(this);
    m_searchFieldCombo->setMinimumWidth(150);
    m_searchFieldCombo->addItem(QString::fromUtf8("Nome do paciente"),    QStringLiteral("patient_name"));
    m_searchFieldCombo->addItem(QString::fromUtf8("ID do paciente"),      QStringLiteral("patient_id"));
    m_searchFieldCombo->addItem(QString::fromUtf8("Data de nascimento"),  QStringLiteral("patient_birth_date"));
    m_searchFieldCombo->addItem(QString::fromUtf8("Accession number"),    QStringLiteral("accession_number"));
    m_searchFieldCombo->addItem(QString::fromUtf8("Descrição do estudo"), QStringLiteral("study_description"));
    m_searchFieldCombo->addItem(QString::fromUtf8("Exam ID"),             QStringLiteral("exam_id"));
    searchRow->addWidget(m_searchFieldCombo);

    m_searchLineEdit = new QLineEdit(this);
    m_searchLineEdit->setPlaceholderText(QString::fromUtf8("Digite o nome do paciente..."));
    m_searchLineEdit->setClearButtonEnabled(true);
    searchRow->addWidget(m_searchLineEdit, 1);

    m_searchBtn = new QPushButton(QString::fromUtf8("Pesquisar"), this);
    m_searchBtn->setDefault(true);
    m_searchBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background-color: #0284C7; color: white; font-weight: bold; "
        "padding: 4px 14px; border-radius: 4px; } "
        "QPushButton:hover { background-color: #0369A1; }"));
    searchRow->addWidget(m_searchBtn);

    m_clearBtn = new QPushButton(QString::fromUtf8("Limpar"), this);
    searchRow->addWidget(m_clearBtn);

    m_advancedToggleBtn = new QToolButton(this);
    m_advancedToggleBtn->setText(QString::fromUtf8("Filtros avançados ▾"));
    searchRow->addWidget(m_advancedToggleBtn);

    mainLayout->addLayout(searchRow);

    // -------------------------------------------------------------------------
    // Painel Expansível de Filtros Avançados
    // -------------------------------------------------------------------------
    m_advancedPanel = new QFrame(this);
    m_advancedPanel->setFrameShape(QFrame::StyledPanel);
    m_advancedPanel->setStyleSheet(QStringLiteral(
        "QFrame { background-color: #0A192F; border: 1px solid #1E3A5F; border-radius: 4px; padding: 4px; }"));
    m_advancedPanel->setVisible(false);

    QGridLayout *advGrid = new QGridLayout(m_advancedPanel);
    advGrid->setContentsMargins(8, 6, 8, 6);
    advGrid->setHorizontalSpacing(10);
    advGrid->setVerticalSpacing(6);

    m_advPatientName = new QLineEdit(m_advancedPanel);
    m_advPatientId   = new QLineEdit(m_advancedPanel);
    m_advBirthDate   = new QLineEdit(m_advancedPanel);
    m_advBirthDate->setPlaceholderText(QStringLiteral("AAAAMMDD ou DD/MM/AAAA"));
    m_advStudyDesc   = new QLineEdit(m_advancedPanel);
    m_advAccession   = new QLineEdit(m_advancedPanel);
    m_advExamId      = new QLineEdit(m_advancedPanel);
    m_advReferring   = new QLineEdit(m_advancedPanel);

    advGrid->addWidget(new QLabel(QString::fromUtf8("Nome Paciente:"), m_advancedPanel), 0, 0);
    advGrid->addWidget(m_advPatientName, 0, 1);
    advGrid->addWidget(new QLabel(QString::fromUtf8("Patient ID:"), m_advancedPanel), 0, 2);
    advGrid->addWidget(m_advPatientId, 0, 3);
    advGrid->addWidget(new QLabel(QString::fromUtf8("Data Nasc.:"), m_advancedPanel), 0, 4);
    advGrid->addWidget(m_advBirthDate, 0, 5);

    advGrid->addWidget(new QLabel(QString::fromUtf8("Descrição:"), m_advancedPanel), 1, 0);
    advGrid->addWidget(m_advStudyDesc, 1, 1);
    advGrid->addWidget(new QLabel(QString::fromUtf8("Accession:"), m_advancedPanel), 1, 2);
    advGrid->addWidget(m_advAccession, 1, 3);
    advGrid->addWidget(new QLabel(QString::fromUtf8("Exam ID:"), m_advancedPanel), 1, 4);
    advGrid->addWidget(m_advExamId, 1, 5);

    advGrid->addWidget(new QLabel(QString::fromUtf8("Médico:"), m_advancedPanel), 2, 0);
    advGrid->addWidget(m_advReferring, 2, 1);

    mainLayout->addWidget(m_advancedPanel);

    // -------------------------------------------------------------------------
    // Painel Expansível de Configurações de Servidores PACS
    // -------------------------------------------------------------------------
    m_serverPanel = new QFrame(this);
    m_serverPanel->setFrameShape(QFrame::StyledPanel);
    m_serverPanel->setStyleSheet(QStringLiteral(
        "QFrame { background-color: #0A192F; border: 1px solid #1E3A5F; border-radius: 4px; }"));
    m_serverPanel->setVisible(false);

    QGridLayout *cfgGrid = new QGridLayout(m_serverPanel);
    cfgGrid->setContentsMargins(8, 8, 8, 8);
    cfgGrid->setHorizontalSpacing(8);
    cfgGrid->setVerticalSpacing(6);

    m_cfgHost = new QLineEdit(m_serverPanel);
    m_cfgPort = new QSpinBox(m_serverPanel);
    m_cfgPort->setRange(1, 65535); m_cfgPort->setValue(11112);
    m_cfgRemoteAe = new QLineEdit(m_serverPanel); m_cfgRemoteAe->setMaxLength(16);
    m_cfgLocalAe  = new QLineEdit(m_serverPanel); m_cfgLocalAe->setMaxLength(16);
    m_cfgLocalAe->setText(QStringLiteral("111011851"));
    m_cfgListenerPort = new QSpinBox(m_serverPanel);
    m_cfgListenerPort->setRange(1, 65535); m_cfgListenerPort->setValue(11112);
    m_cfgDescription = new QLineEdit(m_serverPanel);

    m_cfgSyntax = new QComboBox(m_serverPanel);
    m_cfgSyntax->addItem(QStringLiteral("Explicit VR Little Endian"),  QStringLiteral("1.2.840.10008.1.2.1"));
    m_cfgSyntax->addItem(QStringLiteral("Implicit VR Little Endian"),  QStringLiteral("1.2.840.10008.1.2"));
    m_cfgSyntax->addItem(QStringLiteral("JPEG Baseline"),              QStringLiteral("1.2.840.10008.1.2.4.50"));
    m_cfgSyntax->addItem(QStringLiteral("JPEG Lossless"),              QStringLiteral("1.2.840.10008.1.2.4.70"));
    m_cfgSyntax->addItem(QStringLiteral("JPEG-LS Lossless"),           QStringLiteral("1.2.840.10008.1.2.4.80"));
    m_cfgSyntax->addItem(QStringLiteral("JPEG 2000 Lossless"),         QStringLiteral("1.2.840.10008.1.2.4.90"));

    m_cfgMethod = new QComboBox(m_serverPanel);
    m_cfgMethod->addItems(QStringList() << QStringLiteral("C-GET") << QStringLiteral("C-MOVE"));

    m_cfgEchoBtn   = new QPushButton(QString::fromUtf8("Testar conexão (C-ECHO)"), m_serverPanel);
    m_cfgSaveBtn   = new QPushButton(QString::fromUtf8("Salvar servidor"),          m_serverPanel);
    m_cfgRemoveBtn = new QPushButton(QString::fromUtf8("Remover servidor"),          m_serverPanel);

    cfgGrid->addWidget(new QLabel(QString::fromUtf8("IP / Host:"), m_serverPanel), 0, 0);
    cfgGrid->addWidget(m_cfgHost, 0, 1);
    cfgGrid->addWidget(new QLabel(QString::fromUtf8("Porta:"), m_serverPanel), 0, 2);
    cfgGrid->addWidget(m_cfgPort, 0, 3);
    cfgGrid->addWidget(new QLabel(QString::fromUtf8("AE Remoto:"), m_serverPanel), 0, 4);
    cfgGrid->addWidget(m_cfgRemoteAe, 0, 5);

    cfgGrid->addWidget(new QLabel(QString::fromUtf8("Descrição:"), m_serverPanel), 1, 0);
    cfgGrid->addWidget(m_cfgDescription, 1, 1);
    cfgGrid->addWidget(new QLabel(QString::fromUtf8("Meu AE Title:"), m_serverPanel), 1, 2);
    cfgGrid->addWidget(m_cfgLocalAe, 1, 3);
    cfgGrid->addWidget(new QLabel(QString::fromUtf8("Porta C-MOVE:"), m_serverPanel), 1, 4);
    cfgGrid->addWidget(m_cfgListenerPort, 1, 5);

    cfgGrid->addWidget(new QLabel(QString::fromUtf8("Sintaxe:"), m_serverPanel), 2, 0);
    cfgGrid->addWidget(m_cfgSyntax, 2, 1);
    cfgGrid->addWidget(new QLabel(QString::fromUtf8("Método:"), m_serverPanel), 2, 2);
    cfgGrid->addWidget(m_cfgMethod, 2, 3);

    QHBoxLayout *cfgBtns = new QHBoxLayout;
    cfgBtns->addWidget(m_cfgEchoBtn);
    cfgBtns->addWidget(m_cfgSaveBtn);
    cfgBtns->addWidget(m_cfgRemoveBtn);
    cfgGrid->addLayout(cfgBtns, 2, 4, 1, 2);

    mainLayout->addWidget(m_serverPanel);

    // -------------------------------------------------------------------------
    // Splitter Central com as Duas Tabelas (Estudos / Séries)
    // -------------------------------------------------------------------------
    m_splitter = new QSplitter(Qt::Vertical, this);

    // Tabela Superior: Estudos
    QWidget *studiesContainer = new QWidget(this);
    QVBoxLayout *studiesLayout = new QVBoxLayout(studiesContainer);
    studiesLayout->setContentsMargins(0, 0, 0, 0);
    studiesLayout->setSpacing(2);

    m_studiesTable = new QTableWidget(0, 9, studiesContainer);
    QStringList studyHeaders = {
        QString::fromUtf8("Data do estudo"),
        QString::fromUtf8("Nome do paciente"),
        QString::fromUtf8("Data de nascimento"),
        QString::fromUtf8("Patient ID"),
        QString::fromUtf8("Modalidade"),
        QString::fromUtf8("Descrição do estudo"),
        QString::fromUtf8("Accession number"),
        QString::fromUtf8("Exam ID"),
        QString::fromUtf8("Médico solicitante")
    };
    m_studiesTable->setHorizontalHeaderLabels(studyHeaders);
    m_studiesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    // Multi-seleção: Ctrl+Click e Shift+Click selecionam múltiplos estudos
    m_studiesTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_studiesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_studiesTable->setAlternatingRowColors(true);
    m_studiesTable->horizontalHeader()->setStretchLastSection(true);
    m_studiesTable->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_studiesTable->verticalHeader()->setVisible(false);
    // Habilitar menu de contexto personalizado
    m_studiesTable->setContextMenuPolicy(Qt::CustomContextMenu);
    studiesLayout->addWidget(m_studiesTable);

    // Tabela Inferior: Séries do Estudo
    QWidget *seriesContainer = new QWidget(this);
    QVBoxLayout *seriesLayout = new QVBoxLayout(seriesContainer);
    seriesLayout->setContentsMargins(0, 0, 0, 0);
    seriesLayout->setSpacing(2);

    m_seriesTable = new QTableWidget(0, 6, seriesContainer);
    QStringList seriesHeaders = {
        QStringLiteral("#"),
        QString::fromUtf8("Data"),
        QString::fromUtf8("Número"),
        QString::fromUtf8("Modalidade"),
        QString::fromUtf8("Descrição"),
        QString::fromUtf8("# Imagens")
    };
    m_seriesTable->setHorizontalHeaderLabels(seriesHeaders);
    m_seriesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_seriesTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_seriesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_seriesTable->setAlternatingRowColors(true);
    m_seriesTable->horizontalHeader()->setStretchLastSection(true);
    m_seriesTable->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_seriesTable->verticalHeader()->setVisible(false);
    seriesLayout->addWidget(m_seriesTable);

    m_splitter->addWidget(studiesContainer);
    m_splitter->addWidget(seriesContainer);
    m_splitter->setStretchFactor(0, 3);
    m_splitter->setStretchFactor(1, 2);
    mainLayout->addWidget(m_splitter, 1);

    // -------------------------------------------------------------------------
    // Barra Inferior: Status, Checkbox de Cache Volátil e Download
    // -------------------------------------------------------------------------
    QHBoxLayout *bottomRow = new QHBoxLayout;
    bottomRow->setSpacing(12);

    m_statusLabel = new QLabel(QString::fromUtf8("0 estudos encontrados"), this);
    m_statusLabel->setStyleSheet(QStringLiteral("QLabel { color: #ABC9EC; font-size: 11px; }"));
    bottomRow->addWidget(m_statusLabel);

    bottomRow->addStretch(1);

    m_cacheCheckBox = new QCheckBox(
        QString::fromUtf8("Armazenar em cache volátil (limpar automaticamente ao fechar/trocar de exame)"), this);
    m_cacheCheckBox->setChecked(true); // Regra: padrão cache volátil
    m_cacheCheckBox->setToolTip(QString::fromUtf8(
        "Quando ativado, os exames são mantidos temporariamente em cache e removidos "
        "automaticamente ao abrir outro exame ou encerrar a aplicação."));
    bottomRow->addWidget(m_cacheCheckBox);

    m_downloadBtn = new QPushButton(QString::fromUtf8("Download / Abrir"), this);
    m_downloadBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background-color: #0284C7; color: white; font-weight: bold; "
        "padding: 6px 18px; border-radius: 4px; } "
        "QPushButton:hover { background-color: #0369A1; }"));
    bottomRow->addWidget(m_downloadBtn);

    mainLayout->addLayout(bottomRow);

    // -------------------------------------------------------------------------
    // Ações do Menu de Contexto (estilo RadiAnt exato)
    // -------------------------------------------------------------------------
    m_actDownload    = new QAction(QString::fromUtf8("Download"),                       this);
    m_actAddToActive = new QAction(QString::fromUtf8("Add to active window"),           this);
    m_actNewWindow   = new QAction(QString::fromUtf8("Download to new window"),         this);
    m_actSeparate    = new QAction(QString::fromUtf8("Download to separate windows"),   this);
    m_actImport      = new QAction(QString::fromUtf8("Import to database"),             this);
    m_actFindPatient = new QAction(QString::fromUtf8("Find all studies of this patient"), this);

    // Atalhos de teclado (iguais ao RadiAnt)
    m_actDownload->setShortcut(QKeySequence(Qt::Key_Return));
    m_actAddToActive->setShortcut(QKeySequence(Qt::SHIFT | Qt::Key_Return));
    m_actNewWindow->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Return));
    m_actSeparate->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Return));
    m_actFindPatient->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_P));

    // Adiciona as ações à tabela para que os atalhos funcionem dentro do dialog
    m_studiesTable->addAction(m_actDownload);
    m_studiesTable->addAction(m_actAddToActive);
    m_studiesTable->addAction(m_actNewWindow);
    m_studiesTable->addAction(m_actSeparate);
    m_studiesTable->addAction(m_actFindPatient);

    // "Import to database" desabilitado por padrão (exames não são salvos por padrão)
    m_actImport->setEnabled(false);
    m_actImport->setToolTip(QString::fromUtf8("Funcionalidade não disponível no modo cache volátil"));

    // -------------------------------------------------------------------------
    // Conexões de Sinais
    // -------------------------------------------------------------------------
    connect(m_settingsBtn,      &QToolButton::clicked,                  this, &PacsQueryRetrieveDialog::toggleServerSettings);
    connect(m_advancedToggleBtn,&QToolButton::clicked,                  this, &PacsQueryRetrieveDialog::toggleAdvancedFilters);
    connect(m_searchBtn,        &QPushButton::clicked,                  this, &PacsQueryRetrieveDialog::onSearchClicked);
    connect(m_searchLineEdit,   &QLineEdit::returnPressed,              this, &PacsQueryRetrieveDialog::onSearchClicked);
    connect(m_clearBtn,         &QPushButton::clicked,                  this, &PacsQueryRetrieveDialog::onClearClicked);
    connect(m_clearDateBtn,     &QPushButton::clicked,                  this, [this]() {
        m_datePresetCombo->setCurrentIndex(0); // "Todas as datas"
    });
    connect(m_downloadBtn,      &QPushButton::clicked,                  this, &PacsQueryRetrieveDialog::onDownloadClicked);
    connect(m_datePresetCombo,  QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PacsQueryRetrieveDialog::onDatePresetChanged);
    connect(m_searchFieldCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PacsQueryRetrieveDialog::onSearchFieldChanged);
    connect(m_serverCombo,      QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PacsQueryRetrieveDialog::onServerSelected);

    connect(m_studiesTable, &QTableWidget::itemSelectionChanged, this, &PacsQueryRetrieveDialog::onStudySelected);
    connect(m_studiesTable, &QTableWidget::cellDoubleClicked,    this, &PacsQueryRetrieveDialog::onStudyDoubleClicked);
    connect(m_studiesTable, &QTableWidget::customContextMenuRequested,
            this, &PacsQueryRetrieveDialog::onStudiesContextMenu);

    connect(m_cfgEchoBtn,   &QPushButton::clicked, this, &PacsQueryRetrieveDialog::onEchoServer);
    connect(m_cfgSaveBtn,   &QPushButton::clicked, this, &PacsQueryRetrieveDialog::onSaveServer);
    connect(m_cfgRemoveBtn, &QPushButton::clicked, this, &PacsQueryRetrieveDialog::onRemoveServer);

    // Ações do menu de contexto
    connect(m_actDownload,    &QAction::triggered, this, &PacsQueryRetrieveDialog::downloadReplace);
    connect(m_actAddToActive, &QAction::triggered, this, &PacsQueryRetrieveDialog::downloadAdd);
    connect(m_actNewWindow,   &QAction::triggered, this, &PacsQueryRetrieveDialog::downloadNewWindow);
    connect(m_actSeparate,    &QAction::triggered, this, &PacsQueryRetrieveDialog::downloadSeparate);
    connect(m_actFindPatient, &QAction::triggered, this, &PacsQueryRetrieveDialog::onFindPatientStudies);
}

void PacsQueryRetrieveDialog::loadProfiles()
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope,
                       QApplication::organizationName(), QApplication::applicationName());
    m_profiles = QJsonDocument::fromJson(
        settings.value(QStringLiteral("PACS/profiles")).toByteArray()).array();
    refreshServerCombo();
}

void PacsQueryRetrieveDialog::saveProfiles()
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope,
                       QApplication::organizationName(), QApplication::applicationName());
    settings.setValue(QStringLiteral("PACS/profiles"),
                      QJsonDocument(m_profiles).toJson(QJsonDocument::Compact));
    settings.sync();
}

void PacsQueryRetrieveDialog::refreshServerCombo()
{
    m_serverCombo->clear();
    for (const QJsonValue &v : m_profiles) {
        const QJsonObject p = v.toObject();
        const QString desc = p.value(QStringLiteral("description")).toString();
        m_serverCombo->addItem(desc.isEmpty() ? p.value(QStringLiteral("host")).toString() : desc);
    }
    if (!m_profiles.isEmpty()) {
        onServerSelected(0);
    }
}

void PacsQueryRetrieveDialog::onServerSelected(int index)
{
    if (index < 0 || index >= m_profiles.size()) return;
    const QJsonObject p = m_profiles.at(index).toObject();
    m_cfgHost->setText(p.value(QStringLiteral("host")).toString());
    m_cfgPort->setValue(p.value(QStringLiteral("port")).toInt(11112));
    m_cfgRemoteAe->setText(p.value(QStringLiteral("remote_ae")).toString());
    m_cfgLocalAe->setText(p.value(QStringLiteral("local_ae")).toString(QStringLiteral("111011851")));
    m_cfgListenerPort->setValue(p.value(QStringLiteral("listener_port")).toInt(11112));
    m_cfgDescription->setText(p.value(QStringLiteral("description")).toString());
    m_cfgSyntax->setCurrentIndex(qMax(0, m_cfgSyntax->findData(
        p.value(QStringLiteral("transfer_syntax")).toString())));
    m_cfgMethod->setCurrentText(p.value(QStringLiteral("retrieve")).toString(QStringLiteral("C-GET")));
}

QJsonObject PacsQueryRetrieveDialog::getActiveServerConfig() const
{
    return QJsonObject{
        {QStringLiteral("host"),            m_cfgHost->text().trimmed()},
        {QStringLiteral("port"),            m_cfgPort->value()},
        {QStringLiteral("remote_ae"),       m_cfgRemoteAe->text().trimmed()},
        {QStringLiteral("local_ae"),        m_cfgLocalAe->text().trimmed()},
        {QStringLiteral("listener_port"),   m_cfgListenerPort->value()},
        {QStringLiteral("description"),     m_cfgDescription->text().trimmed()},
        {QStringLiteral("transfer_syntax"), m_cfgSyntax->currentData().toString()},
        {QStringLiteral("retrieve"),        m_cfgMethod->currentText()}
    };
}

void PacsQueryRetrieveDialog::toggleServerSettings()
{
    m_serverPanel->setVisible(!m_serverPanel->isVisible());
}

void PacsQueryRetrieveDialog::toggleAdvancedFilters()
{
    const bool show = !m_advancedPanel->isVisible();
    m_advancedPanel->setVisible(show);
    m_advancedToggleBtn->setText(show
        ? QString::fromUtf8("Filtros avançados ▴")
        : QString::fromUtf8("Filtros avançados ▾"));
}

void PacsQueryRetrieveDialog::onDatePresetChanged(int index)
{
    Q_UNUSED(index);
    updateDateControls();
}

void PacsQueryRetrieveDialog::updateDateControls()
{
    const QString preset = m_datePresetCombo->currentData().toString();
    const QDate today = QDate::currentDate();

    if (preset == QStringLiteral("ALL")) {
        m_startDateEdit->setEnabled(false);
        m_endDateEdit->setEnabled(false);
        m_clearDateBtn->setEnabled(false);
    } else if (preset == QStringLiteral("TODAY")) {
        m_startDateEdit->setEnabled(true);
        m_endDateEdit->setEnabled(false);
        m_startDateEdit->setDate(today);
        m_endDateEdit->setDate(today);
        m_clearDateBtn->setEnabled(true);
    } else if (preset == QStringLiteral("YESTERDAY")) {
        m_startDateEdit->setEnabled(true);
        m_endDateEdit->setEnabled(false);
        m_startDateEdit->setDate(today.addDays(-1));
        m_endDateEdit->setDate(today.addDays(-1));
        m_clearDateBtn->setEnabled(true);
    } else if (preset == QStringLiteral("LAST_WEEK")) {
        m_startDateEdit->setEnabled(true);
        m_endDateEdit->setEnabled(true);
        m_startDateEdit->setDate(today.addDays(-7));
        m_endDateEdit->setDate(today);
        m_clearDateBtn->setEnabled(true);
    } else if (preset == QStringLiteral("LAST_MONTH")) {
        m_startDateEdit->setEnabled(true);
        m_endDateEdit->setEnabled(true);
        m_startDateEdit->setDate(today.addMonths(-1));
        m_endDateEdit->setDate(today);
        m_clearDateBtn->setEnabled(true);
    } else if (preset == QStringLiteral("LAST_YEAR")) {
        m_startDateEdit->setEnabled(true);
        m_endDateEdit->setEnabled(true);
        m_startDateEdit->setDate(today.addYears(-1));
        m_endDateEdit->setDate(today);
        m_clearDateBtn->setEnabled(true);
    } else if (preset == QStringLiteral("CUSTOM_DATE")) {
        m_startDateEdit->setEnabled(true);
        m_endDateEdit->setEnabled(false);
        m_endDateEdit->setDate(m_startDateEdit->date());
        m_clearDateBtn->setEnabled(true);
    } else if (preset == QStringLiteral("CUSTOM_RANGE")) {
        m_startDateEdit->setEnabled(true);
        m_endDateEdit->setEnabled(true);
        m_clearDateBtn->setEnabled(true);
    }
}

void PacsQueryRetrieveDialog::onSearchFieldChanged(int index)
{
    Q_UNUSED(index);
    const QString field = m_searchFieldCombo->currentData().toString();
    if (field == QStringLiteral("patient_name"))
        m_searchLineEdit->setPlaceholderText(QString::fromUtf8("Digite o nome do paciente..."));
    else if (field == QStringLiteral("patient_id"))
        m_searchLineEdit->setPlaceholderText(QString::fromUtf8("Digite o ID / Prontuário do paciente..."));
    else if (field == QStringLiteral("patient_birth_date"))
        m_searchLineEdit->setPlaceholderText(QString::fromUtf8("Digite a data de nascimento (DD/MM/AAAA)..."));
    else if (field == QStringLiteral("accession_number"))
        m_searchLineEdit->setPlaceholderText(QString::fromUtf8("Digite o Accession Number..."));
    else if (field == QStringLiteral("study_description"))
        m_searchLineEdit->setPlaceholderText(QString::fromUtf8("Digite a descrição do estudo..."));
    else if (field == QStringLiteral("exam_id"))
        m_searchLineEdit->setPlaceholderText(QString::fromUtf8("Digite o Exam ID..."));
}

void PacsQueryRetrieveDialog::onEchoServer()
{
    QJsonObject req = getActiveServerConfig();
    req[QStringLiteral("action")] = QStringLiteral("echo");
    const QJsonObject res = runWorkerSync(req);
    if (res.value(QStringLiteral("ok")).toBool()) {
        QMessageBox::information(this, QStringLiteral("PACS"), res.value(QStringLiteral("message")).toString());
    } else {
        QMessageBox::warning(this, QStringLiteral("PACS"), res.value(QStringLiteral("message")).toString());
    }
}

void PacsQueryRetrieveDialog::onSaveServer()
{
    QJsonObject p = getActiveServerConfig();
    if (p.value(QStringLiteral("host")).toString().isEmpty() ||
        p.value(QStringLiteral("remote_ae")).toString().isEmpty() ||
        p.value(QStringLiteral("local_ae")).toString().isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("PACS"),
            QString::fromUtf8("Informe endereço e AE Titles local e remoto."));
        return;
    }
    int found = -1;
    for (int i = 0; i < m_profiles.size(); ++i) {
        const QJsonObject old = m_profiles.at(i).toObject();
        if (old.value(QStringLiteral("host"))      == p.value(QStringLiteral("host")) &&
            old.value(QStringLiteral("port"))      == p.value(QStringLiteral("port")) &&
            old.value(QStringLiteral("remote_ae")) == p.value(QStringLiteral("remote_ae"))) {
            found = i; break;
        }
    }
    if (found < 0) {
        m_profiles.append(p);
        found = m_profiles.size() - 1;
    } else {
        m_profiles.replace(found, p);
    }
    saveProfiles();
    refreshServerCombo();
    m_serverCombo->setCurrentIndex(found);
    QMessageBox::information(this, QStringLiteral("PACS"),
        QString::fromUtf8("Configurações do servidor salvas com sucesso."));
}

void PacsQueryRetrieveDialog::onRemoveServer()
{
    const int idx = m_serverCombo->currentIndex();
    if (idx >= 0 && idx < m_profiles.size()) {
        m_profiles.removeAt(idx);
        saveProfiles();
        refreshServerCombo();
    }
}

void PacsQueryRetrieveDialog::onClearClicked()
{
    m_searchLineEdit->clear();
    m_advPatientName->clear();
    m_advPatientId->clear();
    m_advBirthDate->clear();
    m_advStudyDesc->clear();
    m_advAccession->clear();
    m_advExamId->clear();
    m_advReferring->clear();
    m_modalityCombo->clearSelection();
    m_datePresetCombo->setCurrentIndex(2); // Hoje
    m_studiesTable->setRowCount(0);
    m_seriesTable->setRowCount(0);
    m_statusLabel->setText(QString::fromUtf8("0 estudos encontrados"));
}

QJsonObject PacsQueryRetrieveDialog::runWorkerSync(
    const QJsonObject &request,
    const std::function<void(const QJsonObject&)> &progressCb)
{
    QString base   = QCoreApplication::applicationDirPath() + QStringLiteral("/pacs/");
    QString worker = base + QStringLiteral("pacs_worker.py");
    if (!QFile::exists(worker)) {
        const QString alt = QCoreApplication::applicationDirPath() + QStringLiteral("/pacs_worker.py");
        if (QFile::exists(alt)) worker = alt;
    }
    QString python = base + QStringLiteral("python.exe");
    if (!QFile::exists(python)) {
        QString sys = QStandardPaths::findExecutable(QStringLiteral("python"));
        if (sys.isEmpty()) sys = QStandardPaths::findExecutable(QStringLiteral("python3"));
        if (!sys.isEmpty()) python = sys;
    }
    if (!QFile::exists(python) || !QFile::exists(worker)) {
        return {{QStringLiteral("ok"), false},
                {QStringLiteral("message"),
                 QString::fromUtf8("Cliente PACS indisponível: componentes de rede não foram instalados.")}};
    }

    QProcess process;
    process.setProgram(python);
    process.setArguments(QStringList() << worker);
    process.start();
    if (!process.waitForStarted(5000)) {
        return {{QStringLiteral("ok"), false}, {QStringLiteral("message"), process.errorString()}};
    }
    process.write(QJsonDocument(request).toJson(QJsonDocument::Compact) + "\n");
    process.closeWriteChannel();

    QProgressDialog progress(this);
    progress.setWindowTitle(QStringLiteral("PACS"));
    progress.setLabelText(QString::fromUtf8("Comunicando com o servidor DICOM..."));
    progress.setCancelButtonText(QString::fromUtf8("Cancelar"));
    progress.setRange(0, 0);
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(300);

    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);
    QObject::connect(&process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                     &loop, &QEventLoop::quit);
    QObject::connect(&progress, &QProgressDialog::canceled, &loop, &QEventLoop::quit);
    QObject::connect(&timeout,  &QTimer::timeout,           &loop, &QEventLoop::quit);

    QJsonObject lastFinalResult;

    auto readLines = [&]() {
        while (process.canReadLine()) {
            const QByteArray line = process.readLine().trimmed();
            if (line.isEmpty()) continue;
            QJsonParseError err;
            const QJsonDocument doc = QJsonDocument::fromJson(line, &err);
            if (err.error == QJsonParseError::NoError && doc.isObject()) {
                const QJsonObject obj = doc.object();
                if (obj.contains(QStringLiteral("event"))) {
                    if (progressCb) progressCb(obj);
                    if (obj.value(QStringLiteral("event")).toString() == QStringLiteral("file_received")) {
                        const int count = obj.value(QStringLiteral("count")).toInt();
                        progress.setLabelText(
                            QString::fromUtf8("Baixando imagens do PACS: %1 fatias recebidas...").arg(count));
                    }
                } else if (obj.contains(QStringLiteral("ok"))) {
                    lastFinalResult = obj;
                }
            }
        }
    };

    QObject::connect(&process, &QProcess::readyReadStandardOutput, &loop, readLines);

    timeout.start(15 * 60 * 1000); // 15 minutos
    if (process.state() != QProcess::NotRunning) loop.exec();
    readLines();

    if (process.state() != QProcess::NotRunning) {
        process.kill();
        process.waitForFinished(3000);
        return {{QStringLiteral("ok"), false},
                {QStringLiteral("message"), QString::fromUtf8("Operação cancelada ou expirada.")}};
    }

    if (!lastFinalResult.isEmpty()) return lastFinalResult;

    // Fallback: ler restante do stdout
    const QByteArray remaining = process.readAllStandardOutput();
    const QList<QByteArray> lines = remaining.split('\n');
    for (int i = lines.size() - 1; i >= 0; --i) {
        const QByteArray l = lines.at(i).trimmed();
        if (l.isEmpty()) continue;
        QJsonParseError err;
        const QJsonDocument doc = QJsonDocument::fromJson(l, &err);
        if (err.error == QJsonParseError::NoError && doc.isObject())
            return doc.object();
    }

    return {{QStringLiteral("ok"), false},
            {QStringLiteral("message"), QString::fromUtf8("O cliente PACS terminou sem resposta válida.")}};
}

QString PacsQueryRetrieveDialog::formatDateString(const QString &rawDate, const QString &rawTime) const
{
    if (rawDate.length() == 8) {
        const QString y = rawDate.mid(0, 4);
        const QString m = rawDate.mid(4, 2);
        const QString d = rawDate.mid(6, 2);
        QString formatted = QStringLiteral("%1/%2/%3").arg(d, m, y);
        if (rawTime.length() >= 6) {
            const QString hh = rawTime.mid(0, 2);
            const QString mm = rawTime.mid(2, 2);
            const QString ss = rawTime.mid(4, 2);
            formatted += QStringLiteral(" %1:%2:%3").arg(hh, mm, ss);
        }
        return formatted;
    }
    return rawDate;
}

void PacsQueryRetrieveDialog::onSearchClicked()
{
    QJsonObject req = getActiveServerConfig();
    req[QStringLiteral("action")] = QStringLiteral("find");

    // Resolver data do estudo
    const QString datePreset = m_datePresetCombo->currentData().toString();
    if (datePreset != QStringLiteral("ALL")) {
        if (datePreset == QStringLiteral("TODAY")     ||
            datePreset == QStringLiteral("YESTERDAY") ||
            datePreset == QStringLiteral("CUSTOM_DATE")) {
            req[QStringLiteral("study_date")] =
                m_startDateEdit->date().toString(QStringLiteral("yyyyMMdd"));
        } else {
            req[QStringLiteral("study_date")] =
                QStringLiteral("%1-%2")
                    .arg(m_startDateEdit->date().toString(QStringLiteral("yyyyMMdd")))
                    .arg(m_endDateEdit->date().toString(QStringLiteral("yyyyMMdd")));
        }
    }

    // Modalidades selecionadas (pode ser múltipla)
    const QStringList selectedMods = m_modalityCombo->selectedModalities();
    if (!selectedMods.isEmpty()) {
        if (selectedMods.size() == 1) {
            req[QStringLiteral("modality")] = selectedMods.first();
        } else {
            // Múltiplas: envia como lista (pacs_worker.py filtra client-side)
            QJsonArray modArray;
            for (const QString &mod : selectedMods)
                modArray.append(mod);
            req[QStringLiteral("modalities")] = modArray;
        }
    }

    // Campo de busca rápida
    const QString quickField = m_searchFieldCombo->currentData().toString();
    const QString quickValue = m_searchLineEdit->text().trimmed();
    if (!quickValue.isEmpty()) {
        req[quickField] = quickValue;
    }

    // Filtros avançados (se preenchidos)
    if (!m_advPatientName->text().trimmed().isEmpty())
        req[QStringLiteral("patient_name")] = m_advPatientName->text().trimmed();
    if (!m_advPatientId->text().trimmed().isEmpty())
        req[QStringLiteral("patient_id")] = m_advPatientId->text().trimmed();
    if (!m_advBirthDate->text().trimmed().isEmpty()) {
        QString b = m_advBirthDate->text().trimmed().remove('/');
        req[QStringLiteral("patient_birth_date")] = b;
    }
    if (!m_advStudyDesc->text().trimmed().isEmpty())
        req[QStringLiteral("study_description")] = m_advStudyDesc->text().trimmed();
    if (!m_advAccession->text().trimmed().isEmpty())
        req[QStringLiteral("accession_number")] = m_advAccession->text().trimmed();
    if (!m_advExamId->text().trimmed().isEmpty())
        req[QStringLiteral("exam_id")] = m_advExamId->text().trimmed();
    if (!m_advReferring->text().trimmed().isEmpty())
        req[QStringLiteral("referring_physician")] = m_advReferring->text().trimmed();

    m_statusLabel->setText(QString::fromUtf8("Consultando servidor PACS..."));
    qApp->processEvents();

    const QJsonObject res = runWorkerSync(req);
    if (!res.value(QStringLiteral("ok")).toBool()) {
        m_statusLabel->setText(QString::fromUtf8("Falha na consulta"));
        QMessageBox::warning(this, QStringLiteral("PACS"), res.value(QStringLiteral("message")).toString());
        return;
    }

    m_currentStudies = res.value(QStringLiteral("studies")).toArray();
    populateStudiesTable(m_currentStudies);
}

void PacsQueryRetrieveDialog::populateStudiesTable(const QJsonArray &studies)
{
    m_studiesTable->setRowCount(0);
    m_seriesTable->setRowCount(0);

    for (const QJsonValue &val : studies) {
        const QJsonObject s = val.toObject();
        const int row = m_studiesTable->rowCount();
        m_studiesTable->insertRow(row);

        const QString studyDate = formatDateString(
            s.value(QStringLiteral("StudyDate")).toString(),
            s.value(QStringLiteral("StudyTime")).toString());
        QString patientName = s.value(QStringLiteral("PatientName")).toString();
        patientName.replace('^', ' ');
        patientName = patientName.trimmed();

        const QString birthDate  = formatDateString(s.value(QStringLiteral("PatientBirthDate")).toString());
        const QString patientId  = s.value(QStringLiteral("PatientID")).toString();
        const QString modality   = s.value(QStringLiteral("ModalitiesInStudy")).toString();
        const QString studyDesc  = s.value(QStringLiteral("StudyDescription")).toString();
        const QString accession  = s.value(QStringLiteral("AccessionNumber")).toString();
        const QString examId     = s.value(QStringLiteral("StudyID")).toString();
        const QString referring  = s.value(QStringLiteral("ReferringPhysicianName")).toString()
                                     .replace('^', ' ').trimmed();

        m_studiesTable->setItem(row, 0, new QTableWidgetItem(studyDate));
        m_studiesTable->setItem(row, 1, new QTableWidgetItem(patientName));
        m_studiesTable->setItem(row, 2, new QTableWidgetItem(birthDate));
        m_studiesTable->setItem(row, 3, new QTableWidgetItem(patientId));
        m_studiesTable->setItem(row, 4, new QTableWidgetItem(modality));
        m_studiesTable->setItem(row, 5, new QTableWidgetItem(studyDesc));
        m_studiesTable->setItem(row, 6, new QTableWidgetItem(accession));
        m_studiesTable->setItem(row, 7, new QTableWidgetItem(examId));
        m_studiesTable->setItem(row, 8, new QTableWidgetItem(referring));

        // Armazenar StudyInstanceUID no UserRole da coluna 0
        m_studiesTable->item(row, 0)->setData(Qt::UserRole,
            s.value(QStringLiteral("StudyInstanceUID")).toString());
        // Armazenar PatientID para "Find all studies of this patient"
        m_studiesTable->item(row, 0)->setData(Qt::UserRole + 1, patientId);
    }

    m_statusLabel->setText(
        QString::fromUtf8("%1 estudo(s) encontrado(s)").arg(studies.size()));
    if (studies.size() > 0) {
        m_studiesTable->selectRow(0);
    }
}

void PacsQueryRetrieveDialog::onStudySelected()
{
    // Carrega séries do estudo atualmente destacado (último clicado)
    const int row = m_studiesTable->currentRow();
    if (row < 0 || !m_studiesTable->item(row, 0)) {
        m_seriesTable->setRowCount(0);
        return;
    }

    const QString studyUid = m_studiesTable->item(row, 0)->data(Qt::UserRole).toString();
    if (studyUid.isEmpty()) return;

    QJsonObject req = getActiveServerConfig();
    req[QStringLiteral("action")]    = QStringLiteral("find_series");
    req[QStringLiteral("study_uid")] = studyUid;

    const QJsonObject res = runWorkerSync(req);
    if (res.value(QStringLiteral("ok")).toBool()) {
        populateSeriesTable(res.value(QStringLiteral("series")).toArray());
    }
}

void PacsQueryRetrieveDialog::populateSeriesTable(const QJsonArray &series)
{
    m_seriesTable->setRowCount(0);
    for (int i = 0; i < series.size(); ++i) {
        const QJsonObject s = series.at(i).toObject();
        const int row = m_seriesTable->rowCount();
        m_seriesTable->insertRow(row);

        const QString idx    = QString::number(i + 1);
        const QString sDate  = formatDateString(
            s.value(QStringLiteral("SeriesDate")).toString(),
            s.value(QStringLiteral("SeriesTime")).toString());
        const QString sNum   = s.value(QStringLiteral("SeriesNumber")).toString();
        const QString sMod   = s.value(QStringLiteral("Modality")).toString();
        const QString sDesc  = s.value(QStringLiteral("SeriesDescription")).toString();
        const QString sCount = s.value(QStringLiteral("NumberOfSeriesRelatedInstances")).toString();

        m_seriesTable->setItem(row, 0, new QTableWidgetItem(idx));
        m_seriesTable->setItem(row, 1, new QTableWidgetItem(sDate));
        m_seriesTable->setItem(row, 2, new QTableWidgetItem(sNum));
        m_seriesTable->setItem(row, 3, new QTableWidgetItem(sMod));
        m_seriesTable->setItem(row, 4, new QTableWidgetItem(sDesc));
        m_seriesTable->setItem(row, 5, new QTableWidgetItem(sCount));
        m_seriesTable->item(row, 0)->setData(Qt::UserRole,
            s.value(QStringLiteral("SeriesInstanceUID")).toString());
    }
}

void PacsQueryRetrieveDialog::onStudyDoubleClicked(int row, int col)
{
    Q_UNUSED(row);
    Q_UNUSED(col);
    downloadSelected(ReplaceActive);
}

// =========================================================================
// Menu de Contexto (estilo RadiAnt)
// =========================================================================

void PacsQueryRetrieveDialog::onStudiesContextMenu(const QPoint &pos)
{
    if (m_studiesTable->rowCount() == 0) return;
    const QList<int> rows = selectedStudyRows();
    if (rows.isEmpty()) return;

    QMenu menu(this);
    menu.addAction(m_actDownload);
    menu.addSeparator();
    menu.addAction(m_actAddToActive);
    menu.addAction(m_actNewWindow);
    menu.addAction(m_actSeparate);
    menu.addSeparator();
    menu.addAction(m_actImport);
    menu.addSeparator();

    // "Find all studies of this patient" – disponível apenas com 1 estudo selecionado
    m_actFindPatient->setEnabled(rows.size() == 1);
    menu.addAction(m_actFindPatient);

    menu.exec(m_studiesTable->viewport()->mapToGlobal(pos));
}

void PacsQueryRetrieveDialog::onFindPatientStudies()
{
    const QList<int> rows = selectedStudyRows();
    if (rows.isEmpty()) return;

    const QString patientId = m_studiesTable->item(rows.first(), 0)
                                  ->data(Qt::UserRole + 1).toString();
    if (patientId.isEmpty()) return;

    // Popula campo de Patient ID nos filtros avançados e busca sem filtro de data
    m_advancedPanel->setVisible(true);
    m_advancedToggleBtn->setText(QString::fromUtf8("Filtros avançados ▴"));
    m_advPatientId->setText(patientId);

    // Busca todas as datas deste paciente
    const int savedIdx = m_datePresetCombo->currentIndex();
    m_datePresetCombo->setCurrentIndex(0); // "Todas as datas"
    Q_UNUSED(savedIdx);

    onSearchClicked();
}

// =========================================================================
// Lógica de Download por Modo
// =========================================================================

QList<int> PacsQueryRetrieveDialog::selectedStudyRows() const
{
    QList<int> rows;
    const QList<QTableWidgetItem*> selected = m_studiesTable->selectedItems();
    QSet<int> seen;
    for (const QTableWidgetItem *item : selected) {
        const int r = item->row();
        if (!seen.contains(r)) {
            seen.insert(r);
            rows << r;
        }
    }
    std::sort(rows.begin(), rows.end());
    return rows;
}

void PacsQueryRetrieveDialog::downloadSelected(DownloadMode mode)
{
    const QList<int> rows = selectedStudyRows();
    if (rows.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("PACS"),
            QString::fromUtf8("Selecione um ou mais estudos na tabela para baixar."));
        return;
    }

    // Para ReplaceActive: um download de cada vez (substitui o exame atual)
    // Para AddToActive: baixa todos e abre juntos na mesma janela
    // Para DownloadNewWindow / SeparateWindows: cada estudo em janela separada

    QStringList downloadedDirs;

    for (int row : rows) {
        const QTableWidgetItem *item0 = m_studiesTable->item(row, 0);
        if (!item0) continue;
        const QString studyUid = item0->data(Qt::UserRole).toString();
        if (studyUid.isEmpty()) continue;

        QString targetDir;
        if (m_cacheCheckBox->isChecked()) {
            // Cache volátil: cria diretório único por estudo
            const QString safeUid = studyUid.right(16);
            const QString root    = PacsCacheManager::getCacheRoot();
            const QString dirName = QStringLiteral("pacs_%1_%2")
                                        .arg(safeUid)
                                        .arg(QDateTime::currentMSecsSinceEpoch());
            targetDir = root + QStringLiteral("/") + dirName;
            QDir().mkpath(targetDir);
            PacsCacheManager::setCurrentCacheDir(targetDir);
        } else {
            targetDir = QFileDialog::getExistingDirectory(
                this, QString::fromUtf8("Escolha a pasta para salvar o exame permanentemente"));
            if (targetDir.isEmpty()) return;
        }

        QJsonObject req = getActiveServerConfig();
        req[QStringLiteral("action")]          = QStringLiteral("retrieve");
        req[QStringLiteral("study_uid")]       = studyUid;
        req[QStringLiteral("destination")]     = targetDir;
        req[QStringLiteral("stream_progress")] = true;

        const QJsonObject res = runWorkerSync(req);
        if (!res.value(QStringLiteral("ok")).toBool()) {
            QMessageBox::warning(this, QStringLiteral("PACS"),
                res.value(QStringLiteral("message")).toString());
            continue;
        }

        const QString downloadedDir  = res.value(QStringLiteral("directory")).toString();
        const int     receivedCount  = res.value(QStringLiteral("received")).toInt();

        if (!downloadedDir.isEmpty() && receivedCount > 0) {
            downloadedDirs << downloadedDir;
        }
    }

    if (downloadedDirs.isEmpty()) return;

    accept(); // Fecha o diálogo

    if (!m_mainWindow) return;

    switch (mode) {
    case ReplaceActive:
        // Limpa outros caches, abre o primeiro (ou único) estudo
        PacsCacheManager::cleanupOtherCaches(downloadedDirs.first());
        m_mainWindow->open_args(QStringList() << downloadedDirs.first());
        break;

    case AddToActive:
        // Abre todos os estudos sem limpar o cache entre eles
        // O primeiro substitui, os seguintes são adicionados à sessão
        PacsCacheManager::cleanupOtherCaches(downloadedDirs.first());
        for (const QString &dir : downloadedDirs) {
            m_mainWindow->open_args(QStringList() << dir);
        }
        break;

    case DownloadNewWindow:
        // Abre o primeiro em nova janela (tab)
        // O Aliza não tem suporte nativo a múltiplas janelas independentes;
        // usamos open_args que abre no viewer principal
        PacsCacheManager::cleanupOtherCaches(downloadedDirs.first());
        m_mainWindow->open_args(QStringList() << downloadedDirs.first());
        break;

    case SeparateWindows:
        // Abre cada estudo sequencialmente substituindo o anterior
        // (comportamento de múltiplas janelas limitado pela arquitetura do Aliza)
        for (const QString &dir : downloadedDirs) {
            PacsCacheManager::cleanupOtherCaches(dir);
            m_mainWindow->open_args(QStringList() << dir);
        }
        break;
    }
}

void PacsQueryRetrieveDialog::onDownloadClicked()
{
    downloadSelected(ReplaceActive);
}

void PacsQueryRetrieveDialog::downloadReplace()
{
    downloadSelected(ReplaceActive);
}

void PacsQueryRetrieveDialog::downloadAdd()
{
    downloadSelected(AddToActive);
}

void PacsQueryRetrieveDialog::downloadNewWindow()
{
    downloadSelected(DownloadNewWindow);
}

void PacsQueryRetrieveDialog::downloadSeparate()
{
    downloadSelected(SeparateWindows);
}

// =========================================================================
// Função de entrada pública
// =========================================================================

void openPacsDialog(MainWindow *parent)
{
    PacsQueryRetrieveDialog dlg(parent);
    dlg.exec();
}
