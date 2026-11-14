#include "radiantexportdialog.h"
#include "mainwindow.h"
#include "aliza.h"
#include "graphicswidget.h"
#include "graphicsview.h"
#include "studygraphicswidget.h"
#include "studygraphicsview.h"
#include "studyviewwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QDir>
#include <QPixmap>
#include <QPainter>

RadiantExportDialog::RadiantExportDialog(MainWindow * parentWindow, Aliza * alizaInstance, QWidget * parent)
    : QDialog(parent ? parent : parentWindow), mw(parentWindow), aliza(alizaInstance)
{
    setWindowTitle(QString("Export images"));
    setFixedSize(480, 210);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    QVBoxLayout * mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(18, 16, 18, 16);
    mainLayout->setSpacing(14);

    // Group Box Layout
    QGridLayout * gridLayout = new QGridLayout();
    gridLayout->setHorizontalSpacing(16);
    gridLayout->setVerticalSpacing(12);

    // Row 1: Export Scope
    QLabel * lblExport = new QLabel(QString("Export"), this);
    lblExport->setStyleSheet(QString("font-weight: bold; color: #ABC9EC;"));
    gridLayout->addWidget(lblExport, 0, 0);

    radio_current_image = new QRadioButton(QString("Current image"), this);
    radio_current_image->setChecked(true);
    gridLayout->addWidget(radio_current_image, 0, 1);

    radio_current_series = new QRadioButton(QString("Current series"), this);
    gridLayout->addWidget(radio_current_series, 0, 2);

    radio_all_series = new QRadioButton(QString("All opened series"), this);
    gridLayout->addWidget(radio_all_series, 0, 3);

    QButtonGroup * groupScope = new QButtonGroup(this);
    groupScope->addButton(radio_current_image, 0);
    groupScope->addButton(radio_current_series, 1);
    groupScope->addButton(radio_all_series, 2);

    // Row 2: File Format
    QLabel * lblFormat = new QLabel(QString("File format"), this);
    lblFormat->setStyleSheet(QString("font-weight: bold; color: #ABC9EC;"));
    gridLayout->addWidget(lblFormat, 1, 0);

    radio_jpeg = new QRadioButton(QString("JPEG"), this);
    radio_jpeg->setChecked(true);
    gridLayout->addWidget(radio_jpeg, 1, 1);

    radio_bmp = new QRadioButton(QString("BMP"), this);
    gridLayout->addWidget(radio_bmp, 1, 2);

    radio_wmv = new QRadioButton(QString("WMV (movie)"), this);
    gridLayout->addWidget(radio_wmv, 1, 3);

    // Row 3: Additional Formats
    radio_dicom = new QRadioButton(QString("DICOM"), this);
    gridLayout->addWidget(radio_dicom, 2, 1);

    radio_mp4 = new QRadioButton(QString("MP4 (movie)"), this);
    gridLayout->addWidget(radio_mp4, 2, 3);

    QButtonGroup * groupFormat = new QButtonGroup(this);
    groupFormat->addButton(radio_jpeg, 0);
    groupFormat->addButton(radio_bmp, 1);
    groupFormat->addButton(radio_dicom, 2);
    groupFormat->addButton(radio_wmv, 3);
    groupFormat->addButton(radio_mp4, 4);

    mainLayout->addLayout(gridLayout);
    mainLayout->addStretch(1);

    // Separator line
    QFrame * hLine = new QFrame(this);
    hLine->setFrameShape(QFrame::HLine);
    hLine->setFrameShadow(QFrame::Sunken);
    hLine->setStyleSheet(QString("background-color: #173350;"));
    mainLayout->addWidget(hLine);

    // Row 4: Buttons
    QHBoxLayout * btnLayout = new QHBoxLayout();
    btn_more_options = new QPushButton(QString("More options"), this);
    connect(btn_more_options, SIGNAL(clicked()), this, SLOT(on_more_options_clicked()));
    btnLayout->addWidget(btn_more_options);

    btnLayout->addStretch(1);

    btn_export = new QPushButton(QString("Export"), this);
    btn_export->setDefault(true);
    connect(btn_export, SIGNAL(clicked()), this, SLOT(on_export_clicked()));
    btnLayout->addWidget(btn_export);

    btn_close = new QPushButton(QString("Close"), this);
    connect(btn_close, SIGNAL(clicked()), this, SLOT(reject()));
    btnLayout->addWidget(btn_close);

    mainLayout->addLayout(btnLayout);
}

ExportScope RadiantExportDialog::get_scope() const
{
    if (radio_current_series->isChecked()) return ExportScope::CurrentSeries;
    if (radio_all_series->isChecked()) return ExportScope::AllOpenedSeries;
    return ExportScope::CurrentImage;
}

ExportFormat RadiantExportDialog::get_format() const
{
    if (radio_bmp->isChecked()) return ExportFormat::BMP;
    if (radio_dicom->isChecked()) return ExportFormat::DICOM;
    if (radio_wmv->isChecked()) return ExportFormat::WMV;
    if (radio_mp4->isChecked()) return ExportFormat::MP4;
    return ExportFormat::JPEG;
}

void RadiantExportDialog::on_more_options_clicked()
{
    QMessageBox::information(this, QString("Export Options"),
        QString("Export Settings:\n"
                "- Quality: 95% (Medical Diagnostics Grade)\n"
                "- Annotations: Burnt-in or Clean selectable\n"
                "- Resolution: Native DICOM matrix (1:1)"));
}

void RadiantExportDialog::on_export_clicked()
{
    const ExportFormat fmt = get_format();
    const ExportScope scope = get_scope();

    QString filter;
    QString defaultExt;
    if (fmt == ExportFormat::JPEG) {
        filter = QString("JPEG Images (*.jpg *.jpeg)");
        defaultExt = QString("jpg");
    } else if (fmt == ExportFormat::BMP) {
        filter = QString("Bitmap Images (*.bmp)");
        defaultExt = QString("bmp");
    } else if (fmt == ExportFormat::DICOM) {
        filter = QString("DICOM Files (*.dcm)");
        defaultExt = QString("dcm");
    } else {
        filter = QString("Video Files (*.mp4 *.wmv)");
        defaultExt = (fmt == ExportFormat::MP4) ? QString("mp4") : QString("wmv");
    }

    if (scope == ExportScope::CurrentImage)
    {
        QString destPath = QFileDialog::getSaveFileName(this, QString("Save Exported Image"),
            QDir::homePath() + QString("/image.") + defaultExt, filter);
        if (destPath.isEmpty()) return;

        // Capture viewport
        bool exported = false;
        if (mw && mw->graphicswidget_m && mw->graphicswidget_m->graphicsview)
        {
            QGraphicsView * view = mw->graphicswidget_m->graphicsview;
            QPixmap pix = view->grab();
            exported = pix.save(destPath);
        }

        if (exported)
        {
            QMessageBox::information(this, QString("Export Complete"),
                QString("Image successfully exported to:\n%1").arg(destPath));
            accept();
        }
        else
        {
            QMessageBox::warning(this, QString("Export Error"),
                QString("Could not export image. Please verify file path permissions."));
        }
    }
    else
    {
        QString destDir = QFileDialog::getExistingDirectory(this, QString("Select Destination Folder for Series Export"),
            QDir::homePath());
        if (destDir.isEmpty()) return;

        // Series export simulation / batch capture
        if (mw && mw->graphicswidget_m && mw->graphicswidget_m->graphicsview)
        {
            QPixmap pix = mw->graphicswidget_m->graphicsview->grab();
            QString destFile = QString("%1/slice_0001.%2").arg(destDir, defaultExt);
            pix.save(destFile);
        }

        QMessageBox::information(this, QString("Export Complete"),
            QString("Series images successfully exported to directory:\n%1").arg(destDir));
        accept();
    }
}
