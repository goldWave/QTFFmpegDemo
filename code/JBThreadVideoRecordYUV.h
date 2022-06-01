#pragma once

#include "JBThreadBase.h"

class JBThreadVideoRecordYUV : public JBThreadBase
{
public:
    explicit JBThreadVideoRecordYUV(QObject *parent = nullptr);
private:
    void run() final;
};

