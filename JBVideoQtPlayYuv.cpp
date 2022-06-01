#include "JBVideoQtPlayYuv.h"
#include <QPainter>
#include <QDebug>
#include <QLabel>

extern "C" {
#include <libavutil/imgutils.h>
}

JBVideoQtPlayYuv::JBVideoQtPlayYuv(QWidget* parent)
	: QDialog{ parent }
{
	setMinimumSize({ 640, 480 });
	QVBoxLayout* verticalLayout_2;    
	verticalLayout_2 = new QVBoxLayout(this);
	verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
	verticalLayout_2->addWidget();


	


	m_srcYuvData.file = k_out_temp_dir + "movie_640x480_rgb888.yuv";
	m_srcYuvData.fps = 30;
	m_srcYuvData.width = 640;
	m_srcYuvData.height = 480;
	m_srcYuvData.pixelFormat = AV_PIX_FMT_YUV420P;

	m_myTimer = new QTimer(this);
	m_myTimer->setInterval(33);
	QObject::connect(m_myTimer, SIGNAL(timeout()), this, SLOT(updateMovie()));

	m_file.setFileName(m_srcYuvData.file);
	// 打开文件
	if (!m_file.open(QFile::ReadOnly)) {
		qDebug() << "open file failed: " << m_file.fileName();
		return;
	}

	m_dstRect = { 0, 0, m_srcYuvData.width, m_srcYuvData.height };

	m_pixelSize = av_image_get_buffer_size(m_srcYuvData.pixelFormat, m_srcYuvData.width, m_srcYuvData.height, 1);
	qDebug() << "m_pixelSize: " << m_pixelSize;
	m_myTimer->start();

}

void JBVideoQtPlayYuv::updateMovie() {

	static bool isEnter = false;
	if (isEnter)
	{
		return;
	}
	isEnter = true;
	char* data = new char[m_pixelSize];

	if (m_file.read(data, m_pixelSize) == m_pixelSize)
	{
		//freeCurrentImage();
		QImage* o = new QImage();
		m_img = new QImage((uchar*)data, m_srcYuvData.width, m_srcYuvData.height, QImage::Format_RGB888);
		
		update();
		m_myTimer->stop();
		
	}
	else {
		m_myTimer->stop();
	}

	delete[]data;

}
void JBVideoQtPlayYuv::freeCurrentImage() {
	//if (!m_img) return;
	//free(m_img->bits());
	//delete m_img;
	//m_img = nullptr;
}

JBVideoQtPlayYuv::~JBVideoQtPlayYuv()
{
	if (m_myTimer)
	{
		m_myTimer->stop();
		delete m_myTimer;
	}
	freeCurrentImage();
}

void JBVideoQtPlayYuv::paintEvent(QPaintEvent* event)
{
	if (!m_img)
	{
		return;
	}

	QPainter painter(this);
	qDebug() << "img: " << *m_img;
	qDebug() << "img: " << m_img;
	QImage tmp = (*m_img);
	qDebug() << "tmp: " << tmp;
	painter.drawImage(m_dstRect, tmp);

	//.drawImage()

}
