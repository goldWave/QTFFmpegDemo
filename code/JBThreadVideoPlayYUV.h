#pragma once

#include <QObject>
#include "JBCommonMethod.h"
#include <qtimer.h>
#include <qfile.h>


class JBThreadVideoPlayYUV : public QObject {
	Q_OBJECT

public:
	explicit JBThreadVideoPlayYUV(QObject* parent = nullptr);
	void play();
	void stop();

private:
    QTimer *m_myTimer;
    //渲染上下文
    SDL_Renderer* m_renderer = nullptr;

    SDL_Texture* m_texture = nullptr;
    QFile m_file;
    
    private slots:
    void updateMovie();
};

