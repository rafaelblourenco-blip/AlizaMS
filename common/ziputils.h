#ifndef ZIPUTILS_H
#define ZIPUTILS_H

#include <QString>

class QProgressDialog;

class ZipUtils
{
public:
	static bool isZipArchive(const QString & filePath);
	static QString extractZipToTemp(
		const QString & zipPath,
		QProgressDialog * pb = nullptr,
		QString * errorMsg = nullptr);
	static void cleanupTempCache();
	static QString findDicomdir(const QString & dirPath);

private:
	static void purgeMacMetadata(const QString & dirPath);
};

#endif // ZIPUTILS_H
