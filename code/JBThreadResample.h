#pragma once

#include "JBThreadBase.h"

struct JBAudioData;

class JBThreadResample : public JBThreadBase
{
    Q_OBJECT
public:
    explicit JBThreadResample(QObject *parent = nullptr);

private:
    void run() final;
    
    void startWithData(JBAudioData &inputData, JBAudioData &outputData);
    

};

