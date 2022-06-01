//
//  JBThreadAACDecodeFile.h
//  JBFFTest
//
//  Created by 任金波 on 2022/4/17.
//

#pragma once

#include "JBThreadBase.h"
struct JBAudioData;

class JBThreadAACDecodeFile : public JBThreadBase
{
    Q_OBJECT
public:
    explicit JBThreadAACDecodeFile(QObject *parent = nullptr);


private:
    void run() final;

    void startDecoder(JBAudioData& outData, const QString& inName);

};


