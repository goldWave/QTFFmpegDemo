#pragma once

#include "JBThreadBase.h"

class JBThreadAudioRecordWav : public JBThreadBase
{
    Q_OBJECT
public:
    explicit JBThreadAudioRecordWav(QObject *parent = nullptr);
private:
    void run() final;

signals:

};

