#include "SoundManager.h"
#include <QFileInfo>


void SoundManager::importSound(const QString &filePath) {
    QFileInfo info(filePath);
    Sound *s = new Sound(info.fileName(), filePath);
    sounds.append(s);
}

void SoundManager::removeSound(int index) {
    if (index >= 0 && index < sounds.size()) {
        delete sounds[index];
        sounds.removeAt(index);
    }
}

void SoundManager::playSound(int index) {
    if (index >= 0 && index < sounds.size()) {
        sounds[index]->play();
    }
}

