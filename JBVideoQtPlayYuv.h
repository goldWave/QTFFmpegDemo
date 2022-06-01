#pragma once

#include <QDialog>
#include <qtimer.h>
#include <qfile.h>
#include <qimage.h>
#include "JBCommonMethod.h"
#include <QLabel>

class JBVideoQtPlayYuv : public QDialog
{
	Q_OBJECT
public:
	explicit JBVideoQtPlayYuv(QWidget* parent = nullptr);
	~JBVideoQtPlayYuv();
private:
	QTimer* m_myTimer{ nullptr };
	QFile m_file;
	int m_pixelSize = 0;
	QImage* m_img{nullptr};
	QRect m_dstRect;

	JBYUVData m_srcYuvData;

	QLabel *m_tmpLabel;

private slots:
	void updateMovie();
	void freeCurrentImage();

protected:
	void paintEvent(QPaintEvent* event) override;
};
