#ifndef RADIANTEXPORTDIALOG_H
#define RADIANTEXPORTDIALOG_H

#include <QDialog>
#include <QRadioButton>
#include <QButtonGroup>
#include <QPushButton>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>

class MainWindow;
class Aliza;

enum class ExportScope {
    CurrentImage = 0,
    CurrentSeries = 1,
    AllOpenedSeries = 2
};

enum class ExportFormat {
    JPEG = 0,
    BMP = 1,
    DICOM = 2,
    WMV = 3,
    MP4 = 4
};

class RadiantExportDialog : public QDialog
{
    Q_OBJECT
public:
    explicit RadiantExportDialog(MainWindow * parentWindow, Aliza * alizaInstance, QWidget * parent = nullptr);
    ~RadiantExportDialog() override = default;

    ExportScope get_scope() const;
    ExportFormat get_format() const;

private slots:
    void on_export_clicked();
    void on_more_options_clicked();

private:
    MainWindow * mw{nullptr};
    Aliza      * aliza{nullptr};

    QRadioButton * radio_current_image{nullptr};
    QRadioButton * radio_current_series{nullptr};
    QRadioButton * radio_all_series{nullptr};

    QRadioButton * radio_jpeg{nullptr};
    QRadioButton * radio_bmp{nullptr};
    QRadioButton * radio_dicom{nullptr};
    QRadioButton * radio_wmv{nullptr};
    QRadioButton * radio_mp4{nullptr};

    QPushButton * btn_export{nullptr};
    QPushButton * btn_close{nullptr};
    QPushButton * btn_more_options{nullptr};
};

#endif // RADIANTEXPORTDIALOG_H
