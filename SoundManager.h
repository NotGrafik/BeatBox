#ifndef SOUNDMANAGER_H
#define SOUNDMANAGER_H

#include <QObject>
#include <QVector>
#include "Sound.h"

class SoundManager : public QObject {
    Q_OBJECT

public:
    explicit SoundManager(QObject* parent = nullptr);
    void importSound(const QString &filePath);
    void removeSound(int index);
    void playSound(int index);
    ~SoundManager();

    int getSoundCount() const;

private:
    QVector<Sound*> sounds;
};

#endif // SOUNDMANAGER_H
