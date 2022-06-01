//
//  JBThreadBase.h
//  JBFFTest
//
//  Created by 任金波 on 2022/4/16.
//

#pragma once

#include <QObject>
#include <QThread>

class JBThreadBase : public QThread
{
    Q_OBJECT
    
public:
    explicit JBThreadBase(QObject *parent = nullptr);
    virtual ~JBThreadBase();

    
signals:
    void timeChanged(int timestamp);

    void comandLineGenerate(const QString &line);
    void comandLinesGenerate(const QStringList &line);
};
