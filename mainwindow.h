#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QFileInfo>
#include <QStatusBar>
#include "NetworkManager.h"
#include "PadPage.h"
#include "SoundManager.h"

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
    void uploadSound(int index, const QString &path);
    
    // Slots pour NetworkManager
    void onSessionCreated(const QString &code);
    void onClientJoined(const QString &name);
    void onJoinedSession();
    void onSessionStarted();
    void onConnectionError(const QString &error);

private:
    void hideModeButtons();
    void showModeButtons();
    void showGameInterface();
    void hideJoinInterface();

    // UI Elements
    QPushButton *soloButton;
    QPushButton *hostButton;
    QPushButton *joinButton;
    QPushButton *startSessionButton;
    QPushButton *joinSubmitButton;
    QPushButton *backButtonJoin;
    QLabel *sessionCodeLabel;
    QLineEdit *joinCodeInput;
    PadPage *padPage;
    
    // Network and Game
    NetworkManager *networkManager;
    SoundManager soundManager;
    
    // État
    bool isNetworkMode;
};

#endif