#include "pacsdialog.h"
#include "mainwindow.h"
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileDialog>
#include <QFormLayout>
#include <QGridLayout>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QProcess>
#include <QProgressDialog>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>
#include <QTableWidget>
#include <QTimer>
#include <QVBoxLayout>

namespace {
QJsonObject runWorker(QWidget *parent, const QJsonObject &request)
{
    const QString base = QCoreApplication::applicationDirPath() + QStringLiteral("/pacs/");
    const QString python = base + QStringLiteral("python.exe"), worker = base + QStringLiteral("pacs_worker.py");
    if (!QFile::exists(python) || !QFile::exists(worker))
        return {{QStringLiteral("ok"), false}, {QStringLiteral("message"), QString::fromUtf8("Cliente PACS indisponível: componentes de rede não foram instalados.")}};
    QProcess process;
    process.setProgram(python);
    process.setArguments(QStringList() << worker);
    process.start();
    if (!process.waitForStarted(5000)) return {{QStringLiteral("ok"), false}, {QStringLiteral("message"), process.errorString()}};
    process.write(QJsonDocument(request).toJson(QJsonDocument::Compact));
    process.closeWriteChannel();
    QProgressDialog progress(QString::fromUtf8("Comunicando com o servidor DICOM..."), QString::fromUtf8("Cancelar"), 0, 0, parent);
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(0);
    QEventLoop loop;
    QTimer timeout; timeout.setSingleShot(true);
    QObject::connect(&process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), &loop, &QEventLoop::quit);
    QObject::connect(&progress, &QProgressDialog::canceled, &loop, &QEventLoop::quit);
    QObject::connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
    timeout.start(15 * 60 * 1000);
    if (process.state() != QProcess::NotRunning) loop.exec();
    if (process.state() != QProcess::NotRunning) {
        process.kill(); process.waitForFinished(5000);
        return {{QStringLiteral("ok"), false}, {QStringLiteral("message"), QString::fromUtf8("Operação cancelada ou expirada. Podem existir arquivos parciais na pasta de destino.")}};
    }
    QJsonParseError error;
    const QJsonDocument doc = QJsonDocument::fromJson(process.readAllStandardOutput(), &error);
    if (process.exitStatus() != QProcess::NormalExit || error.error != QJsonParseError::NoError || !doc.isObject())
        return {{QStringLiteral("ok"), false}, {QStringLiteral("message"), QString::fromUtf8("O cliente PACS terminou sem resposta válida. Verifique os componentes de rede.")}};
    return doc.object();
}
}

void openPacsDialog(MainWindow *parent)
{
    QDialog dialog(parent); dialog.setWindowTitle(QString::fromUtf8("Consulta e importação PACS")); dialog.resize(980, 600);
    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    QGridLayout *grid = new QGridLayout;
    QComboBox *servers = new QComboBox;
    QLineEdit *host = new QLineEdit, *remote = new QLineEdit, *local = new QLineEdit, *description = new QLineEdit;
    remote->setMaxLength(16); local->setMaxLength(16);
    QSpinBox *port = new QSpinBox, *listener = new QSpinBox;
    port->setRange(1,65535); port->setValue(104); listener->setRange(1,65535); listener->setValue(11112);
    local->setText(QStringLiteral("ALIZAMS"));
    QComboBox *syntax = new QComboBox;
    syntax->addItem(QStringLiteral("Explicit VR Little Endian"), QStringLiteral("1.2.840.10008.1.2.1"));
    syntax->addItem(QStringLiteral("Implicit VR Little Endian"), QStringLiteral("1.2.840.10008.1.2"));
    syntax->addItem(QStringLiteral("JPEG Baseline"), QStringLiteral("1.2.840.10008.1.2.4.50"));
    syntax->addItem(QStringLiteral("JPEG Lossless"), QStringLiteral("1.2.840.10008.1.2.4.70"));
    syntax->addItem(QStringLiteral("JPEG-LS Lossless"), QStringLiteral("1.2.840.10008.1.2.4.80"));
    syntax->addItem(QStringLiteral("JPEG 2000 Lossless"), QStringLiteral("1.2.840.10008.1.2.4.90"));
    QComboBox *method = new QComboBox; method->addItems(QStringList() << "C-GET" << "C-MOVE");
    QPushButton *save = new QPushButton(QString::fromUtf8("Salvar servidor")), *remove = new QPushButton(QString::fromUtf8("Remover servidor")), *echo = new QPushButton(QString::fromUtf8("Testar conexão"));
    grid->addWidget(new QLabel(QString::fromUtf8("Servidores salvos")),0,0); grid->addWidget(servers,0,1,1,3); grid->addWidget(save,0,4); grid->addWidget(remove,0,5);
    grid->addWidget(new QLabel(QString::fromUtf8("IP / nome")),1,0); grid->addWidget(host,1,1); grid->addWidget(new QLabel(QString::fromUtf8("Porta")),1,2); grid->addWidget(port,1,3); grid->addWidget(new QLabel(QString::fromUtf8("AE remoto")),1,4); grid->addWidget(remote,1,5);
    grid->addWidget(new QLabel(QString::fromUtf8("Descrição")),2,0); grid->addWidget(description,2,1); grid->addWidget(new QLabel(QString::fromUtf8("Meu AE Title")),2,2); grid->addWidget(local,2,3); grid->addWidget(new QLabel(QString::fromUtf8("Porta C-MOVE")),2,4); grid->addWidget(listener,2,5);
    grid->addWidget(new QLabel(QString::fromUtf8("Sintaxe preferida")),3,0); grid->addWidget(syntax,3,1); grid->addWidget(new QLabel(QString::fromUtf8("Recuperação")),3,2); grid->addWidget(method,3,3); grid->addWidget(echo,3,5); layout->addLayout(grid);
    QLabel *note = new QLabel(QString::fromUtf8("C-MOVE exige que o PACS conheça o AE local, IP e porta de escuta; a sintaxe preferida é negociada com alternativas não comprimidas.")); note->setWordWrap(true); layout->addWidget(note);
    QLineEdit *patient = new QLineEdit, *patientId = new QLineEdit, *date = new QLineEdit;
    date->setPlaceholderText(QStringLiteral("AAAAMMDD ou AAAAMMDD-AAAAMMDD"));
    QPushButton *search = new QPushButton(QString::fromUtf8("Consultar estudos"));
    QGridLayout *query = new QGridLayout;
    query->addWidget(new QLabel(QString::fromUtf8("Paciente")),0,0); query->addWidget(patient,0,1); query->addWidget(new QLabel(QString::fromUtf8("ID")),0,2); query->addWidget(patientId,0,3); query->addWidget(new QLabel(QString::fromUtf8("Data")),0,4); query->addWidget(date,0,5); query->addWidget(search,0,6); layout->addLayout(query);
    QTableWidget *table = new QTableWidget(0,6); table->setHorizontalHeaderLabels(QStringList() << QString::fromUtf8("Paciente") << "ID" << QString::fromUtf8("Data") << QString::fromUtf8("Descrição") << QString::fromUtf8("Modalidade") << "Accession");
    table->setSelectionBehavior(QAbstractItemView::SelectRows); table->setSelectionMode(QAbstractItemView::SingleSelection); table->setEditTriggers(QAbstractItemView::NoEditTriggers); table->horizontalHeader()->setStretchLastSection(true); layout->addWidget(table);
    QPushButton *download = new QPushButton(QString::fromUtf8("Importar estudo selecionado")); layout->addWidget(download);
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, QApplication::organizationName(), QApplication::applicationName());
    QJsonArray profiles = QJsonDocument::fromJson(settings.value(QStringLiteral("PACS/profiles")).toByteArray()).array();
    auto loadProfile = [&](int index) { if (index < 0 || index >= profiles.size()) return; const QJsonObject p=profiles.at(index).toObject(); host->setText(p.value("host").toString()); port->setValue(p.value("port").toInt(104)); remote->setText(p.value("remote_ae").toString()); local->setText(p.value("local_ae").toString("ALIZAMS")); listener->setValue(p.value("listener_port").toInt(11112)); description->setText(p.value("description").toString()); syntax->setCurrentIndex(qMax(0,syntax->findData(p.value("transfer_syntax").toString()))); method->setCurrentText(p.value("retrieve").toString("C-GET")); };
    auto refresh = [&]() { servers->clear(); for (const QJsonValue &v: profiles) { const QJsonObject p=v.toObject(); servers->addItem(p.value("description").toString().isEmpty()?p.value("host").toString():p.value("description").toString()); } };
    auto request = [&]() { return QJsonObject{{"host",host->text().trimmed()},{"port",port->value()},{"remote_ae",remote->text().trimmed()},{"local_ae",local->text().trimmed()},{"listener_port",listener->value()},{"description",description->text().trimmed()},{"transfer_syntax",syntax->currentData().toString()},{"retrieve",method->currentText()}}; };
    auto store = [&]() { settings.setValue(QStringLiteral("PACS/profiles"),QJsonDocument(profiles).toJson(QJsonDocument::Compact)); settings.sync(); };
    QObject::connect(servers, QOverload<int>::of(&QComboBox::currentIndexChanged), &dialog, [&](int i){ loadProfile(i); });
    refresh(); if (!profiles.isEmpty()) loadProfile(0);
    QObject::connect(save,&QPushButton::clicked,&dialog,[&]() { QJsonObject p=request(); if (host->text().trimmed().isEmpty() || remote->text().trimmed().isEmpty() || local->text().trimmed().isEmpty()) { QMessageBox::warning(&dialog,QString::fromUtf8("PACS"),QString::fromUtf8("Informe endereço e AE Titles local e remoto.")); return; } int found=-1; for (int i=0;i<profiles.size();++i) { QJsonObject old=profiles.at(i).toObject(); if(old.value("host")==p.value("host") && old.value("port")==p.value("port") && old.value("remote_ae")==p.value("remote_ae")) {found=i; break;} } if(found<0){ profiles.append(p); found=profiles.size()-1; } else profiles.replace(found,p); store(); refresh(); servers->setCurrentIndex(found); });
    QObject::connect(remove,&QPushButton::clicked,&dialog,[&]() { const int i=servers->currentIndex(); if(i>=0 && i<profiles.size()) {profiles.removeAt(i);store();refresh();} });
    auto run = [&](QJsonObject q) { QJsonObject result=runWorker(&dialog,q); if(!result.value("ok").toBool()) QMessageBox::warning(&dialog,QString::fromUtf8("PACS"),result.value("message").toString()); return result; };
    QObject::connect(echo,&QPushButton::clicked,&dialog,[&]() { QJsonObject q=request();q["action"]="echo"; QJsonObject result=run(q);if(result.value("ok").toBool())QMessageBox::information(&dialog,QString::fromUtf8("PACS"),result.value("message").toString()); });
    QObject::connect(search,&QPushButton::clicked,&dialog,[&]() { QJsonObject q=request();q["action"]="find";q["patient_name"]=patient->text();q["patient_id"]=patientId->text();q["study_date"]=date->text(); QJsonObject result=run(q);if(!result.value("ok").toBool())return;table->setRowCount(0);for(const QJsonValue &value:result.value("studies").toArray()){QJsonObject p=value.toObject();int row=table->rowCount();table->insertRow(row);const char *keys[]={"PatientName","PatientID","StudyDate","StudyDescription","ModalitiesInStudy","AccessionNumber"};for(int c=0;c<6;++c)table->setItem(row,c,new QTableWidgetItem(p.value(keys[c]).toString()));table->item(row,0)->setData(Qt::UserRole,p.value("StudyInstanceUID").toString());} });
    QObject::connect(download,&QPushButton::clicked,&dialog,[&]() {const int row=table->currentRow();if(row<0||!table->item(row,0)){QMessageBox::warning(&dialog,QString::fromUtf8("PACS"),QString::fromUtf8("Selecione um estudo."));return;}const QString uid=table->item(row,0)->data(Qt::UserRole).toString();const QString dest=QFileDialog::getExistingDirectory(&dialog,QString::fromUtf8("Escolha a pasta para salvar o exame"));if(dest.isEmpty())return;QJsonObject q=request();q["action"]="retrieve";q["study_uid"]=uid;q["destination"]=dest;QJsonObject result=run(q);if(result.value("ok").toBool()){const QString dir=result.value("directory").toString();QMessageBox::information(&dialog,QString::fromUtf8("PACS"),QString::fromUtf8("Importação concluída: %1 arquivos.\n%2").arg(result.value("received").toInt()).arg(dir));dialog.accept();parent->open_args(QStringList()<<dir);}});
    dialog.exec();
}
