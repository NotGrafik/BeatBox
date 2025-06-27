#include "Sound.h"
#include <QSoundEffect>
#include <QDebug>

Sound::Sound(const QString& name, const QString& path) : name(name), filePath(path) {
    player = new QSoundEffect;
    player->setSource(QUrl::fromLocalFile(path));
}

void Sound::play() {
    qDebug() << "Trying to play:" << filePath;
    if (!player->isLoaded()) {
        qDebug() << "Sound not loaded.";
    }
    player->setVolume(1.0); 
    player->play();
}
