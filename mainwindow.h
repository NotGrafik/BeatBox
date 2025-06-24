#pragma once

#include <QMainWindow>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QFileInfo>
#include "PadPage.h"
#include "SoundManager.h"
#include "HostSession.h"
#include "JoinSession.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);

private slots:
    void handleSoloMode();
    void handleHostMode();
    void handleJoinMode();
    void startNetworkSession();
    void attemptJoinSession();
    void playPadSound(int index);
    void uploadSound(int index, const QString& path);
    void hideModeButtons();

private:
    QPushButton *soloButton;
    QPushButton *hostButton;
    QPushButton *joinButton;
    QPushButton *startSessionButton;
    QLabel *sessionCodeLabel;
    QLineEdit *joinCodeInput;

    PadPage *padPage;
    SoundManager soundManager;

    HostSession *hostSession = nullptr;
    JoinSession *joinSession = nullptr;
};
