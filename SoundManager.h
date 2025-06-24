#pragma once

#include <QList>
#include "Sound.h"

class SoundManager {
public:
    QList<Sound*> sounds;

    void importSound(const QString& filePath);
    void removeSound(int index);
    void playSound(int index);
};

