//
//  JBThreadH264EncodeFile.h
//  JBFFTest
//
//  Created by 任金波 on 2022/4/17.
//

#pragma once

#include "JBThreadBase.h"
#include "JBCommonMethod.h"

class JBThreadH264EncodeFile : public JBThreadBase
{
    Q_OBJECT
public:
    explicit JBThreadH264EncodeFile(QObject *parent = nullptr);


private:
    void run() final;
    
    void h264Encoder(JBYUVData& yuvModel, const QString& outName);
};


