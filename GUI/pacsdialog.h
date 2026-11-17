#ifndef PACSDIALOG_H
#define PACSDIALOG_H

#include <QDialog>
#include <QJsonObject>
#include <QJsonArray>
#include <QStringList>
#include <QToolButton>

class MainWindow;
class QComboBox;
class QDateEdit;
class QLineEdit;
class QPushButton;
class QTableWidget;
class QLabel;
class QCheckBox;
class QSplitter;
class QFrame;
class QSpinBox;
class QToolButton;
class QListWidget;
class QAction;
class QMenu;
class QShortcut;

class PacsCacheManager
{
public:
    static QString getCacheRoot();
    static QString prepareStudyCacheDir(const QString & studyUid);
    static void cleanupCurrentCache();
    static void cleanupOtherCaches(const QString & exceptDir = QString());
    static void cleanupAllCaches();
    static QString getCurrentCacheDir();
    static void setCurrentCacheDir(const QString &dir);
private:
    static QString s_currentCacheDir;
};

// -------------------------------------------------------------------------
// ModalityCheckCombo: ComboBox com lista de checkboxes para modalidades
// -------------------------------------------------------------------------
class ModalityCheckCombo : public QToolButton
{
    Q_OBJECT
public:
    explicit ModalityCheckCombo(QWidget *parent = nullptr);
    QStringList selectedModalities() const;
    void clearSelection();

signals:
    void selectionChanged();

private slots:
    void showPopup();
    void onItemChanged();

private:
    QListWidget *m_list{nullptr};
    void updateLabel();
};

class PacsQueryRetrieveDialog : public QDialog
{
    Q_OBJECT
public:
    explicit PacsQueryRetrieveDialog(MainWindow *parent);
    ~PacsQueryRetrieveDialog() override = default;

    enum DownloadMode {
        ReplaceActive,       // Enter – substitui exame atual
        AddToActive,         // Shift+Enter – adiciona à janela ativa (abre junto)
        DownloadNewWindow,   // Ctrl+Enter – novo viewport/tab
        SeparateWindows      // Ctrl+Shift+Enter – cada estudo numa janela separada
    };

private slots:
    void onSearchClicked();
    void onClearClicked();
    void onDownloadClicked();
    void onStudySelected();
    void onStudyDoubleClicked(int row, int col);
    void onDatePresetChanged(int index);
    void onSearchFieldChanged(int index);
    void toggleAdvancedFilters();
    void toggleServerSettings();
    void onEchoServer();
    void onSaveServer();
    void onRemoveServer();
    void onServerSelected(int index);
    void onStudiesContextMenu(const QPoint &pos);
    void onFindPatientStudies();

    // Atalhos por modo
    void downloadReplace();
    void downloadAdd();
    void downloadNewWindow();
    void downloadSeparate();

private:
    void setupUi();
    void loadProfiles();
    void saveProfiles();
    void refreshServerCombo();
    void updateDateControls();
    QJsonObject getActiveServerConfig() const;
    QJsonObject runWorkerSync(const QJsonObject &request,
                              const std::function<void(const QJsonObject&)> &progressCb = nullptr);
    void populateStudiesTable(const QJsonArray &studies);
    void populateSeriesTable(const QJsonArray &series);
    QString formatDateString(const QString &rawDate, const QString &rawTime = QString()) const;
    void downloadSelected(DownloadMode mode);
    QList<int> selectedStudyRows() const;

    MainWindow *m_mainWindow{nullptr};

    // Header Controls
    QToolButton        *m_settingsBtn{nullptr};
    QComboBox          *m_serverCombo{nullptr};
    ModalityCheckCombo *m_modalityCombo{nullptr};
    QComboBox          *m_datePresetCombo{nullptr};
    QDateEdit          *m_startDateEdit{nullptr};
    QDateEdit          *m_endDateEdit{nullptr};
    QPushButton        *m_clearDateBtn{nullptr};

    // Quick Search Row
    QComboBox          *m_searchFieldCombo{nullptr};
    QLineEdit          *m_searchLineEdit{nullptr};
    QPushButton        *m_searchBtn{nullptr};
    QPushButton        *m_clearBtn{nullptr};
    QToolButton        *m_advancedToggleBtn{nullptr};

    // Expandable Advanced Filters Panel
    QFrame             *m_advancedPanel{nullptr};
    QLineEdit          *m_advPatientName{nullptr};
    QLineEdit          *m_advPatientId{nullptr};
    QLineEdit          *m_advBirthDate{nullptr};
    QLineEdit          *m_advStudyDesc{nullptr};
    QLineEdit          *m_advAccession{nullptr};
    QLineEdit          *m_advExamId{nullptr};
    QLineEdit          *m_advReferring{nullptr};

    // Expandable Server Config Panel
    QFrame             *m_serverPanel{nullptr};
    QLineEdit          *m_cfgHost{nullptr};
    QSpinBox           *m_cfgPort{nullptr};
    QLineEdit          *m_cfgRemoteAe{nullptr};
    QLineEdit          *m_cfgLocalAe{nullptr};
    QSpinBox           *m_cfgListenerPort{nullptr};
    QLineEdit          *m_cfgDescription{nullptr};
    QComboBox          *m_cfgSyntax{nullptr};
    QComboBox          *m_cfgMethod{nullptr};
    QPushButton        *m_cfgEchoBtn{nullptr};
    QPushButton        *m_cfgSaveBtn{nullptr};
    QPushButton        *m_cfgRemoveBtn{nullptr};

    // Splitter & Tables
    QSplitter          *m_splitter{nullptr};
    QTableWidget       *m_studiesTable{nullptr};
    QTableWidget       *m_seriesTable{nullptr};

    // Bottom Bar
    QLabel             *m_statusLabel{nullptr};
    QCheckBox          *m_cacheCheckBox{nullptr};
    QPushButton        *m_downloadBtn{nullptr};

    // Context menu actions
    QAction            *m_actDownload{nullptr};
    QAction            *m_actAddToActive{nullptr};
    QAction            *m_actNewWindow{nullptr};
    QAction            *m_actSeparate{nullptr};
    QAction            *m_actImport{nullptr};
    QAction            *m_actFindPatient{nullptr};

    QJsonArray m_profiles;
    QJsonArray m_currentStudies;
};

void openPacsDialog(MainWindow *parent);

#endif // PACSDIALOG_H
