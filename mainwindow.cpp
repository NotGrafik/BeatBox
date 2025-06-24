#include "MainWindow.h"
#include <QVBoxLayout>
#include <QWidget>
#include <QDebug>
#include <QLineEdit>
#include <QLabel>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    QWidget *central = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(central);
    resize(800, 800);

    soloButton = new QPushButton("Solo Mode");
    hostButton = new QPushButton("Host Mode");
    joinButton = new QPushButton("Join Mode");
    startSessionButton = new QPushButton("Start Session");
    startSessionButton->hide();

    sessionCodeLabel = new QLabel("Session Code: ");
    sessionCodeLabel->hide();

    joinCodeInput = new QLineEdit();
    joinCodeInput->setPlaceholderText("Enter session code");
    joinCodeInput->hide();

    layout->addWidget(soloButton);
    layout->addWidget(hostButton);
    layout->addWidget(joinButton);
    layout->addWidget(sessionCodeLabel);
    layout->addWidget(joinCodeInput);
    layout->addWidget(startSessionButton);

    joinSubmitButton = new QPushButton("Rejoindre");
    joinSubmitButton->hide();
    backButtonJoin = new QPushButton("Retour");
    backButtonJoin->hide();

    layout->addWidget(joinSubmitButton);
    layout->addWidget(backButtonJoin);

    padPage = new PadPage;
    layout->addWidget(padPage);
    padPage->hide();

    connect(soloButton, &QPushButton::clicked, this, &MainWindow::handleSoloMode);
    connect(hostButton, &QPushButton::clicked, this, &MainWindow::handleHostMode);
    connect(joinButton, &QPushButton::clicked, this, &MainWindow::handleJoinMode);
    connect(startSessionButton, &QPushButton::clicked, this, &MainWindow::startNetworkSession);

    connect(padPage, &PadPage::padClicked, this, &MainWindow::playPadSound);
    connect(padPage, &PadPage::uploadSoundRequested, this, &MainWindow::uploadSound);

    connect(joinSubmitButton, &QPushButton::clicked, this, &MainWindow::attemptJoinSession);
    connect(backButtonJoin, &QPushButton::clicked, this, [this]() {
        joinCodeInput->hide();
        joinSubmitButton->hide();
        backButtonJoin->hide();
        showModeButtons();
    });

    setCentralWidget(central);
}

void MainWindow::handleSoloMode() {
    hideModeButtons();
    padPage->show();
}

void MainWindow::handleHostMode() {
    hideModeButtons();
    hostSession = new HostSession(this);
    hostSession->start();

    sessionCodeLabel->setText("Session Code: " + hostSession->getSessionCode());
    sessionCodeLabel->show();
    startSessionButton->show();

    connect(hostSession, &HostSession::sessionReady, this, []() {
        qDebug() << "Session ready with players.";
    });
}

void MainWindow::handleJoinMode() {
    hideModeButtons();
    connect(joinCodeInput, &QLineEdit::returnPressed, this, &MainWindow::attemptJoinSession);
    joinCodeInput->clear();
    joinCodeInput->show();
    joinSubmitButton->show();
    backButtonJoin->show();

}

void MainWindow::startNetworkSession() {
    if (hostSession) {
        hostSession->startSession();
        padPage->show();
        sessionCodeLabel->hide();
        startSessionButton->hide();
    }
}

void MainWindow::attemptJoinSession() {
    QString code = joinCodeInput->text().trimmed();
    if (!code.isEmpty()) {
        joinSession = new JoinSession(code, this);
        joinSession->start();
        connect(joinSession, &JoinSession::sessionStarted, this, [this]() {
            padPage->show();
            joinCodeInput->hide();
        });
    }
}

void MainWindow::playPadSound(int index) {
    qDebug() << "Play sound on pad" << index;
    soundManager.playSound(index);
}

void MainWindow::uploadSound(int index, const QString &path) {
    qDebug() << "Upload sound for pad" << index << ":" << path;
    padPage->setPadLabel(index, QFileInfo(path).fileName());
    soundManager.importSound(path);
}

void MainWindow::hideModeButtons() {
    soloButton->hide();
    hostButton->hide();
    joinButton->hide();
}


void MainWindow::showModeButtons() {
    soloButton->show();
    hostButton->show();
    joinButton->show();
}
