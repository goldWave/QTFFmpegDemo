#pragma once

#include "JBThreadBase.h"

class JBThreadAudioRecordPCM : public JBThreadBase
{
public:
    explicit JBThreadAudioRecordPCM(QObject *parent = nullptr);
private:
    void run() final;
};

