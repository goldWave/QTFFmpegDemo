//
//  JBThreadH264DecodeFile.h
//  JBFFTest
//
//  Created by 任金波 on 2022/4/17.
//

#pragma once

#include "JBThreadBase.h"
#include "JBCommonMethod.h"

class JBThreadH264DecodeFile : public JBThreadBase
{
    Q_OBJECT
public:
    explicit JBThreadH264DecodeFile(QObject *parent = nullptr);


private:
    void run() final;
    void startDecoder(JBYUVData& outData, const QString& inName);
};


