#pragma once

#include <QString>
#include <QSoundEffect>

class Sound {
public:
    QString name;
    QString filePath;
    QSoundEffect *player;

    Sound(const QString& name, const QString& path);
    void play();
};

