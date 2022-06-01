//
//  JBThreadPlayWav.h
//  JBFFTest
//
//  Created by 任金波 on 2022/4/17.
//

#pragma once

#include "JBThreadBase.h"

struct JBAudioData;
struct SDL_AudioSpec;

class JBThreadPlayWav : public JBThreadBase
{
    Q_OBJECT
public:
    explicit JBThreadPlayWav(QObject *parent = nullptr);

    SDL_AudioSpec* m_spec{nullptr};
    uint32_t m_totalSize = 0;

private:
    void run() final;



};


