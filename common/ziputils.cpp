#include <QDirIterator>
#include "ziputils.h"

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QProcess>
#include <QStandardPaths>
#include <QUuid>
#include <QApplication>
#include <QProgressDialog>

bool ZipUtils::isZipArchive(const QString & filePath)
{
	if (filePath.isEmpty())
	{
		return false;
	}
	QFileInfo fi(filePath);
	if (!fi.exists() || !fi.isFile() || fi.size() < 4)
	{
		return false;
	}
	const QString suffix = fi.suffix().toLower();
	QFile file(filePath);
	if (!file.open(QIODevice::ReadOnly))
	{
		return false;
	}
	QByteArray header = file.read(4);
	file.close();
	if (header.size() < 4)
	{
		return false;
	}
	const unsigned char b0 = static_cast<unsigned char>(header.at(0));
	const unsigned char b1 = static_cast<unsigned char>(header.at(1));
	const unsigned char b2 = static_cast<unsigned char>(header.at(2));
	const unsigned char b3 = static_cast<unsigned char>(header.at(3));

	// PK signatures:
	// 0x50 0x4B 0x03 0x04 (standard zip header)
	// 0x50 0x4B 0x05 0x06 (empty zip header)
	// 0x50 0x4B 0x07 0x08 (spanned archive)
	if (b0 == 0x50 && b1 == 0x4B)
	{
		if ((b2 == 0x03 && b3 == 0x04) ||
			(b2 == 0x05 && b3 == 0x06) ||
			(b2 == 0x07 && b3 == 0x08))
		{
			return true;
		}
		if (suffix == QStringLiteral("zip"))
		{
			return true;
		}
	}
	return false;
}

QString ZipUtils::extractZipToTemp(
	const QString & zipPath,
	QProgressDialog * pb,
	QString * errorMsg)
{
	if (!isZipArchive(zipPath))
	{
		if (errorMsg)
		{
			*errorMsg = QString("O arquivo especificado não é um arquivo ZIP válido.");
		}
		return QString();
	}

	const QString baseTemp = QDir::tempPath() + QString("/AlizaMS");
	const QString uuidStr = QUuid::createUuid().toString(QUuid::WithoutBraces);
	const QString destDir = baseTemp + QString("/zip_") + uuidStr;

	QDir d;
	if (!d.mkpath(destDir))
	{
		if (errorMsg)
		{
			*errorMsg = QString("Não foi possível criar o diretório temporário: %1").arg(destDir);
		}
		return QString();
	}

	QString toolExec;
#ifdef Q_OS_WIN
	if (QFile::exists(QStringLiteral("C:/Windows/System32/tar.exe")))
	{
		toolExec = QStringLiteral("C:/Windows/System32/tar.exe");
	}
	else
	{
		toolExec = QStandardPaths::findExecutable(QStringLiteral("tar"));
		if (toolExec.isEmpty())
		{
			toolExec = QStandardPaths::findExecutable(QStringLiteral("unzip"));
		}
	}
#else
	// On Linux/Unix, prioritize unzip or bsdtar because standard GNU tar does not support ZIP archives
	toolExec = QStandardPaths::findExecutable(QStringLiteral("unzip"));
	if (toolExec.isEmpty())
	{
		toolExec = QStandardPaths::findExecutable(QStringLiteral("bsdtar"));
	}
	if (toolExec.isEmpty())
	{
		toolExec = QStandardPaths::findExecutable(QStringLiteral("tar"));
	}
#endif

	if (toolExec.isEmpty())
	{
		if (errorMsg)
		{
			*errorMsg = QString("Ferramenta de descompactação não encontrada no sistema.");
		}
		QDir(destDir).removeRecursively();
		return QString();
	}

	QProcess proc;
	QStringList args;
	if (toolExec.endsWith(QStringLiteral("unzip"), Qt::CaseInsensitive) ||
		toolExec.endsWith(QStringLiteral("unzip.exe"), Qt::CaseInsensitive))
	{
		args << QStringLiteral("-q")
			 << QStringLiteral("-o")
			 << QDir::toNativeSeparators(zipPath)
			 << QStringLiteral("-x")
			 << QStringLiteral("__MACOSX/*")
			 << QStringLiteral("._*")
			 << QStringLiteral("-d")
			 << QDir::toNativeSeparators(destDir);
	}
	else
	{
		args << QStringLiteral("-xf")
			 << QDir::toNativeSeparators(zipPath)
			 << QStringLiteral("-C")
			 << QDir::toNativeSeparators(destDir)
			 << QStringLiteral("--exclude=__MACOSX*")
			 << QStringLiteral("--exclude=._*");
	}

	proc.start(toolExec, args);
	if (!proc.waitForStarted(5000))
	{
		if (errorMsg)
		{
			*errorMsg = QString("Falha ao iniciar o descompactador: %1").arg(proc.errorString());
		}
		QDir(destDir).removeRecursively();
		return QString();
	}

	while (proc.state() == QProcess::Running)
	{
		if (pb)
		{
			if (pb->wasCanceled())
			{
				proc.kill();
				proc.waitForFinished(1000);
				if (errorMsg)
				{
					*errorMsg = QString("Operação cancelada pelo usuário.");
				}
				QDir(destDir).removeRecursively();
				return QString();
			}
		}
		qApp->processEvents();
		proc.waitForFinished(100);
	}

	if (proc.exitStatus() != QProcess::NormalExit || proc.exitCode() != 0)
	{
		const QString errOutput = QString::fromLocal8Bit(proc.readAllStandardError()).trimmed();
		if (errorMsg)
		{
			*errorMsg = QString("Erro na descompactação (código %1): %2")
							.arg(proc.exitCode())
							.arg(errOutput.isEmpty() ? proc.errorString() : errOutput);
		}
		QDir(destDir).removeRecursively();
		return QString();
	}

	purgeMacMetadata(destDir);
	return destDir;
}

void ZipUtils::purgeMacMetadata(const QString & dirPath)
{
	QDir dir(dirPath);
	const QFileInfoList entries = dir.entryInfoList(
		QDir::Dirs | QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot);
	for (const QFileInfo & entry : entries)
	{
		if (entry.isDir())
		{
			if (entry.fileName() == QStringLiteral("__MACOSX"))
			{
				QDir(entry.absoluteFilePath()).removeRecursively();
			}
			else
			{
				purgeMacMetadata(entry.absoluteFilePath());
			}
		}
		else if (entry.isFile())
		{
			if (entry.fileName().startsWith(QStringLiteral("._")))
			{
				QFile::remove(entry.absoluteFilePath());
			}
		}
	}
}

void ZipUtils::cleanupTempCache()
{
	const QString baseTemp = QDir::tempPath() + QString("/AlizaMS");
	QDir dir(baseTemp);
	if (!dir.exists()) return;
	const QStringList subdirs = dir.entryList(
		QStringList() << QStringLiteral("zip_*"),
		QDir::Dirs | QDir::NoDotAndDotDot);
	for (const QString & sub : subdirs)
	{
		QDir subDir(dir.absoluteFilePath(sub));
		subDir.removeRecursively();
	}
}


QString ZipUtils::findDicomdir(const QString & dirPath)
{
	if (dirPath.isEmpty()) return QString();
	QDir dir(dirPath);
	if (!dir.exists()) return QString();

	// Check immediate dir
	if (dir.exists(QStringLiteral("DICOMDIR")))
	{
		return dir.absoluteFilePath(QStringLiteral("DICOMDIR"));
	}
	if (dir.exists(QStringLiteral("dicomdir")))
	{
		return dir.absoluteFilePath(QStringLiteral("dicomdir"));
	}

	// Recursive search up to 4 levels
	QDirIterator it(dirPath, QStringList() << QStringLiteral("DICOMDIR") << QStringLiteral("dicomdir"),
	                QDir::Files, QDirIterator::Subdirectories);
	if (it.hasNext())
	{
		return it.next();
	}
	return QString();
}
