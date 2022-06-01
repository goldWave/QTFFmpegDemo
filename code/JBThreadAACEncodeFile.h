//
//  JBThreadAACEncodeFile.h
//  JBFFTest
//
//  Created by 任金波 on 2022/4/17.
//

#pragma once

#include "JBThreadBase.h"

struct JBAudioData;

class JBThreadAACEncodeFile : public JBThreadBase
{
    Q_OBJECT
public:
    explicit JBThreadAACEncodeFile(QObject *parent = nullptr);


private:
    void run() final;
    void aacEncoder(JBAudioData& pcmModel, const QString& outName);
};


