#pragma once

#include "JBThreadBase.h"

class JBThreadAudioPlayPCM : public JBThreadBase
{
    Q_OBJECT
public:
    explicit JBThreadAudioPlayPCM(QObject *parent = nullptr);

private:
    void run() final;

};

