#pragma once

#include "JBThreadBase.h"

class JBThreadPCM2WavFile : public JBThreadBase
{
    Q_OBJECT
public:
    explicit JBThreadPCM2WavFile(QObject *parent = nullptr);
    
private:
    void run() final;

signals:

};

